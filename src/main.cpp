#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ModbusMaster.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include "relay_controller.h"

// 继电器引脚定义 (JDQ0-3)
#define RELAY1_PIN 12  // D6 - JDQ0
#define RELAY2_PIN 13  // D7 - JDQ1  
#define RELAY3_PIN 14  // D5 - JDQ2
#define RELAY4_PIN 16  // D0 - JDQ3

// 串口引脚定义
#define DEBUG_RX 3   // RXD0
#define DEBUG_TX 1   // TXD0
#define RS485_RX 4  // D2 - RS485 RX (GPIO4)
#define RS485_TX 5  // D1 - RS485 TX (GPIO5)
// RS485模块自动控制方向，无需DE/RE管脚

// 网络配置
#define DEFAULT_SSID "SSKJ-4"
#define DEFAULT_PASSWORD "xszn486020zcs"
#define AP_NAME "ESP8266-RelayCtrl"
#define AP_PASSWORD "12345678"

// MQTT配置
#define MQTT_BROKER "192.168.1.100"
#define MQTT_PORT 1883
#define MQTT_CLIENT_ID "ESP8266_RelayCtrl"
#define MQTT_TOPIC_BASE "relay/"

// Modbus配置
#define MODBUS_SLAVE_ID 1
#define MODBUS_BAUD 9600

// EEPROM地址定义
#define EEPROM_SIZE 512
#define WIFI_SSID_ADDR 0
#define WIFI_PASS_ADDR 64
#define MQTT_SERVER_ADDR 128
#define MQTT_PORT_ADDR 192
#define DEVICE_ID_ADDR 196

// 全局对象
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
SoftwareSerial rs485Serial(RS485_RX, RS485_TX);
ModbusMaster modbus;

// TCP服务器对象
WiFiServer modbusServer(502);  // Modbus TCP端口
WiFiServer rawTcpServer(8080); // 原始TCP端口

// 全局变量
bool relayStates[4] = {false, false, false, false};
String deviceId = "ESP8266_001";
String mqttServer = MQTT_BROKER;
int mqttPort = MQTT_PORT;
unsigned long lastMqttReconnect = 0;
unsigned long lastHeartbeat = 0;

RelayConfig config;

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP8266 4-Channel Relay Controller ===");
  Serial.println("Version: 1.0.0");
  Serial.println("Device: JDQ0-3 Relay Control");
  
  // 初始化EEPROM
  EEPROM.begin(EEPROM_SIZE);
  loadConfig();
  
  // 初始化继电器引脚
  initRelays();
  
  // 初始化串口通信
  initSerialPorts();
  
  // 初始化WiFi
  initWiFi();
  
  // 初始化网络服务
  initWebServer();
  initMQTT();
  initModbus();
  initTcpServers();
  
  // 初始化mDNS
  if (MDNS.begin("esp8266-relay")) {
    Serial.println("mDNS responder started");
    MDNS.addService("http", "tcp", 80);
  }
  
  Serial.println("System initialized successfully!");
  printSystemInfo();
}

void loop() {
  // 优先处理Web服务器请求
  server.handleClient();
  
  // 静态变量用于控制其他任务的执行频率
  static unsigned long lastMqttCheck = 0;
  static unsigned long lastTcpCheck = 0;
  static unsigned long lastSerialCheck = 0;
  static unsigned long lastModbusCheck = 0;
  static unsigned long lastHeartbeat = 0;
  
  unsigned long currentTime = millis();
  
  // MQTT处理 - 降低频率到每500ms，仅在启用时处理
  if (config.mqttEnabled && currentTime - lastMqttCheck > 500) {
    lastMqttCheck = currentTime;
    if (!mqttClient.connected()) {
      reconnectMQTT();
    }
    mqttClient.loop();
  }
  
  // TCP服务器处理 - 每100ms，仅在启用时处理
  if ((config.tcpEnabled || config.modbusTcpEnabled) && currentTime - lastTcpCheck > 100) {
    lastTcpCheck = currentTime;
    handleTcpClients();
  }
  
  // 串口命令处理 - 每50ms
  if (currentTime - lastSerialCheck > 50) {
    lastSerialCheck = currentTime;
    handleSerialCommands();
  }
  
  // Modbus处理 - 每50ms
  if (currentTime - lastModbusCheck > 50) {
    lastModbusCheck = currentTime;
    handleModbus();
  }
  
  // 心跳发送 - 每10秒，仅在MQTT启用时发送
  if (config.mqttEnabled && currentTime - lastHeartbeat > 10000) {
    lastHeartbeat = currentTime;
    sendHeartbeat();
  }
  
  // mDNS更新 - 每次都执行，但很轻量
  MDNS.update();
  
  // 短暂延迟，但优先响应Web请求
  yield(); // 让出CPU给WiFi栈
}

void initRelays() {
  Serial.println("Initializing relays...");
  
  // 继电器低电平吸合，所以HIGH=关闭，先设置为关闭状态避免上电瞬间动作
  pinMode(RELAY1_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, HIGH);  // 立即关闭（高电平）
  
  pinMode(RELAY2_PIN, OUTPUT);
  digitalWrite(RELAY2_PIN, HIGH);  // 立即关闭（高电平）
  
  pinMode(RELAY3_PIN, OUTPUT);
  digitalWrite(RELAY3_PIN, HIGH);  // 立即关闭（高电平）
  
  pinMode(RELAY4_PIN, OUTPUT);
  digitalWrite(RELAY4_PIN, HIGH);  // 立即关闭（高电平）
  
  // 初始状态为关闭（上电保持关闭）
  for (int i = 0; i < 4; i++) {
    setRelay(i, false);
  }
  
  Serial.println("Relays initialized - All OFF");
}

void initSerialPorts() {
  Serial.println("Initializing serial ports...");
  
  // 调试串口已在setup()中初始化
  Serial.println("Debug serial: 115200 baud");
  
  // RS485串口初始化（自动方向控制）
  rs485Serial.begin(MODBUS_BAUD);
  Serial.println("RS485 serial: 9600 baud");
}

void initWiFi() {
  Serial.println("Initializing WiFi...");
  
  WiFiManager wifiManager;
  wifiManager.setAPStaticIPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  
  // 首先尝试连接默认WiFi (SSKJ-4)
  WiFi.begin(DEFAULT_SSID, DEFAULT_PASSWORD);
  Serial.print("Connecting to default WiFi: ");
  Serial.println(DEFAULT_SSID);
  Serial.print("WiFi password: ");
  Serial.println(DEFAULT_PASSWORD);
  
  // 等待连接，超时后启动配网
  int timeout = 30; // 增加超时时间到30秒
  while (WiFi.status() != WL_CONNECTED && timeout > 0) {
    delay(1000);
    Serial.print(".");
    timeout--;
    
    // 每10秒显示一次WiFi状态
    if (timeout % 10 == 0) {
      Serial.print("\nWiFi Status: ");
      Serial.print(WiFi.status());
      Serial.print(" (");
      switch(WiFi.status()) {
        case WL_IDLE_STATUS: Serial.print("IDLE"); break;
        case WL_NO_SSID_AVAIL: Serial.print("NO_SSID_AVAIL"); break;
        case WL_SCAN_COMPLETED: Serial.print("SCAN_COMPLETED"); break;
        case WL_CONNECTED: Serial.print("CONNECTED"); break;
        case WL_CONNECT_FAILED: Serial.print("CONNECT_FAILED"); break;
        case WL_CONNECTION_LOST: Serial.print("CONNECTION_LOST"); break;
        case WL_DISCONNECTED: Serial.print("DISCONNECTED"); break;
        default: Serial.print("UNKNOWN"); break;
      }
      Serial.println(")");
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC address: ");
    Serial.println(WiFi.macAddress());
    Serial.print("Signal strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    return; // 连接成功，退出函数
  }
  
  // 如果默认WiFi连接失败，尝试已保存的配置
  if (strlen(config.ssid) > 0 && strcmp(config.ssid, DEFAULT_SSID) != 0) {
    Serial.print("\nTrying saved WiFi: ");
    Serial.println(config.ssid);
    WiFi.begin(config.ssid, config.password);
    
    timeout = 20;
    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
      delay(1000);
      Serial.print(".");
      timeout--;
    }
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nDefault WiFi connection failed!");
    Serial.print("Final WiFi Status: ");
    Serial.print(WiFi.status());
    Serial.print(" (");
    switch(WiFi.status()) {
      case WL_NO_SSID_AVAIL: Serial.print("NO_SSID_AVAIL - Network not found"); break;
      case WL_CONNECT_FAILED: Serial.print("CONNECT_FAILED - Wrong password"); break;
      case WL_CONNECTION_LOST: Serial.print("CONNECTION_LOST"); break;
      case WL_DISCONNECTED: Serial.print("DISCONNECTED"); break;
      default: Serial.print("OTHER"); break;
    }
    Serial.println(")");
    
    Serial.println("Starting AP mode...");
    Serial.println("Please connect to ESP8266-RelayCtrl hotspot and configure WiFi");
    wifiManager.autoConnect(AP_NAME, AP_PASSWORD);
  }
}

void initWebServer() {
  Serial.println("Initializing web server...");
  
  // 主页
  server.on("/", handleRoot);
  
  // API端点
  server.on("/api/status", handleStatus);
  server.on("/api/relay", HTTP_POST, handleRelayControl);
  server.on("/api/config", HTTP_GET, handleGetConfig);
  server.on("/api/config", HTTP_POST, handleSetConfig);
  server.on("/api/restart", HTTP_POST, handleRestart);
  server.on("/api/protocol", HTTP_POST, handleProtocolControl); // 新增协议控制
  
  // OTA更新
  httpUpdater.setup(&server, "/update", "admin", "admin");
  
  server.begin();
  Serial.println("Web server started on port 80");
}

void initMQTT() {
  Serial.println("Initializing MQTT...");
  
  mqttClient.setServer(config.mqttServer, config.mqttPort);
  mqttClient.setCallback(mqttCallback);
  
  Serial.print("MQTT server: ");
  Serial.print(config.mqttServer);
  Serial.print(":");
  Serial.println(config.mqttPort);
}

void initModbus() {
  Serial.println("Initializing Modbus...");
  
  modbus.begin(MODBUS_SLAVE_ID, rs485Serial);
  Serial.print("Modbus slave ID: ");
  Serial.println(MODBUS_SLAVE_ID);
}

void setRelay(int relay, bool state) {
  if (relay < 0 || relay > 3) return;
  
  relayStates[relay] = state;
  
  int pin;
  switch (relay) {
    case 0: pin = RELAY1_PIN; break;
    case 1: pin = RELAY2_PIN; break;
    case 2: pin = RELAY3_PIN; break;
    case 3: pin = RELAY4_PIN; break;
  }
  
  // 继电器低电平吸合（LOW=ON, HIGH=OFF）
  digitalWrite(pin, state ? LOW : HIGH);
  
  Serial.print("Relay ");
  Serial.print(relay + 1);
  Serial.print(" (JDQ");
  Serial.print(relay);
  Serial.print("): ");
  Serial.println(state ? "ON" : "OFF");
  
  // 发送MQTT状态更新
  publishRelayState(relay, state);
}

void loadConfig() {
  Serial.println("Loading configuration...");
  
  // 检查配置有效性标志
  if (EEPROM.read(EEPROM_SIZE - 1) != 0xAA) {
    Serial.println("No valid config found, using defaults");
    setDefaultConfig();
    return;
  }
  
  // 读取配置
  for (int i = 0; i < 32; i++) {
    config.ssid[i] = EEPROM.read(WIFI_SSID_ADDR + i);
  }
  for (int i = 0; i < 64; i++) {
    config.password[i] = EEPROM.read(WIFI_PASS_ADDR + i);
  }
  for (int i = 0; i < 64; i++) {
    config.mqttServer[i] = EEPROM.read(MQTT_SERVER_ADDR + i);
  }
  
  config.mqttPort = EEPROM.read(MQTT_PORT_ADDR) | (EEPROM.read(MQTT_PORT_ADDR + 1) << 8);
  
  for (int i = 0; i < 32; i++) {
    config.deviceId[i] = EEPROM.read(DEVICE_ID_ADDR + i);
  }
  
  config.valid = true;
  
  Serial.println("Configuration loaded successfully");
}

void setDefaultConfig() {
  strcpy(config.ssid, DEFAULT_SSID);
  strcpy(config.password, DEFAULT_PASSWORD);
  strcpy(config.mqttServer, MQTT_BROKER);
  config.mqttPort = MQTT_PORT;
  strcpy(config.deviceId, "ESP8266_001");
  config.mqttEnabled = true;      // 默认启用MQTT
  config.tcpEnabled = true;       // 默认启用TCP
  config.modbusTcpEnabled = true; // 默认启用Modbus TCP
  config.valid = true;
  
  saveConfig();
}

void saveConfig() {
  Serial.println("Saving configuration...");
  
  // 写入WiFi配置
  for (int i = 0; i < 32; i++) {
    EEPROM.write(WIFI_SSID_ADDR + i, config.ssid[i]);
  }
  for (int i = 0; i < 64; i++) {
    EEPROM.write(WIFI_PASS_ADDR + i, config.password[i]);
  }
  
  // 写入MQTT配置
  for (int i = 0; i < 64; i++) {
    EEPROM.write(MQTT_SERVER_ADDR + i, config.mqttServer[i]);
  }
  EEPROM.write(MQTT_PORT_ADDR, config.mqttPort & 0xFF);
  EEPROM.write(MQTT_PORT_ADDR + 1, (config.mqttPort >> 8) & 0xFF);
  
  // 写入设备ID
  for (int i = 0; i < 32; i++) {
    EEPROM.write(DEVICE_ID_ADDR + i, config.deviceId[i]);
  }
  
  // 写入有效性标志
  EEPROM.write(EEPROM_SIZE - 1, 0xAA);
  
  EEPROM.commit();
  Serial.println("Configuration saved");
}

void printSystemInfo() {
  Serial.println("\n=== System Information ===");
  Serial.print("Device ID: ");
  Serial.println(config.deviceId);
  Serial.print("WiFi SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("MQTT Server: ");
  Serial.print(config.mqttServer);
  Serial.print(":");
  Serial.println(config.mqttPort);
  Serial.println("========================\n");
}

// 协议控制函数实现
void enableMQTT(bool enable) {
  config.mqttEnabled = enable;
  if (enable) {
    startMQTT();
  } else {
    stopMQTT();
  }
  saveConfig();
}

void enableTCP(bool enable) {
  config.tcpEnabled = enable;
  if (enable) {
    startTCP();
  } else {
    stopTCP();
  }
  saveConfig();
}

void enableModbusTCP(bool enable) {
  config.modbusTcpEnabled = enable;
  if (enable) {
    startModbusTCP();
  } else {
    stopModbusTCP();
  }
  saveConfig();
}

void startMQTT() {
  Serial.println("Starting MQTT...");
  mqttClient.setServer(config.mqttServer, config.mqttPort);
  mqttClient.setCallback(mqttCallback);
}

void stopMQTT() {
  Serial.println("Stopping MQTT...");
  if (mqttClient.connected()) {
    mqttClient.disconnect();
  }
}

void startTCP() {
  Serial.println("Starting raw TCP server...");
  rawTcpServer.begin();
}

void stopTCP() {
  Serial.println("Stopping raw TCP server...");
  rawTcpServer.stop();
}

void startModbusTCP() {
  Serial.println("Starting Modbus TCP server...");
  modbusServer.begin();
}

void stopModbusTCP() {
  Serial.println("Stopping Modbus TCP server...");
  modbusServer.stop();
}
