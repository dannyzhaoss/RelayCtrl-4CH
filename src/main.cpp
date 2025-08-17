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
#include "config.h"
#include "relay_controller.h"

// 全局对象
ESP8266WebServer server(HTTP_PORT);
ESP8266HTTPUpdateServer httpUpdater;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
SoftwareSerial rs485Serial(RS485_RX, RS485_TX);
ModbusMaster modbus;

// TCP服务器对象指针 - 动态创建以支持配置的端口号
WiFiServer* modbusServer = nullptr;
WiFiServer* rawTcpServer = nullptr;

// 全局变量
bool relayStates[4] = {false, false, false, false};
String deviceId = DEFAULT_DEVICE_ID;
String mqttServer = MQTT_BROKER;
int mqttPort = MQTT_PORT;
unsigned long lastMqttReconnect = 0;
unsigned long lastHeartbeat = 0;
String dynamicAPName;  // 动态生成的AP名称
String dynamicMqttClientId;  // 动态生成的MQTT客户端ID

RelayConfig config;

// 函数声明
void printDebugHeartbeat();

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== " PROJECT_NAME " 4-Channel Relay Controller ===");
  Serial.println("Version: " FIRMWARE_VERSION);
  Serial.println("Device: " DEVICE_TYPE);
  
  // 生成动态AP名称（包含MAC后四位）
  String mac = WiFi.macAddress();
  String macSuffix = mac.substring(12, 14) + mac.substring(15, 17);  // 获取后四位，跳过冒号
  dynamicAPName = PROJECT_NAME "-" + macSuffix;
  dynamicMqttClientId = PROJECT_NAME "-" + macSuffix;
  Serial.print("Dynamic AP Name: ");
  Serial.println(dynamicAPName);
  Serial.print("Dynamic MQTT Client ID: ");
  Serial.println(dynamicMqttClientId);
  
  // 初始化EEPROM
  EEPROM.begin(EEPROM_SIZE);
  loadConfig();
  
  // 更新设备ID为动态生成的值
  strcpy(config.deviceId, dynamicMqttClientId.c_str());
  saveConfig();
  
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
  if (MDNS.begin("relayctrl")) {
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
  
  // Modbus处理 - 每10ms处理，提高响应速度
  if (currentTime - lastModbusCheck > 10) {
    lastModbusCheck = currentTime;
    handleModbus();
  }
  
  // 心跳发送 - 每10秒，仅在MQTT启用时发送
  if (config.mqttEnabled && currentTime - lastHeartbeat > 10000) {
    lastHeartbeat = currentTime;
    sendHeartbeat();
  }
  
  // 串口调试心跳 - 每30秒输出系统状态
  static unsigned long lastDebugHeartbeat = 0;
  if (currentTime - lastDebugHeartbeat > 30000) {
    lastDebugHeartbeat = currentTime;
    printDebugHeartbeat();
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
  
  // RS485方向控制引脚初始化
  pinMode(RS485_DE, OUTPUT);
  digitalWrite(RS485_DE, LOW);  // 初始设为接收模式
  Serial.println("RS485 DE/RE pin initialized (GPIO2)");
  
  // RS485串口初始化
  rs485Serial.begin(config.modbusBaudRate);
  Serial.print("RS485 serial: ");
  Serial.print(config.modbusBaudRate);
  Serial.println(" baud");
}

void initWiFi() {
  Serial.println("Initializing WiFi...");
  
  // 先扫描可用网络进行调试
  Serial.println("Scanning WiFi networks...");
  int n = WiFi.scanNetworks();
  Serial.print("Found ");
  Serial.print(n);
  Serial.println(" networks:");
  
  for (int i = 0; i < n; i++) {
    Serial.print("  ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" (");
    Serial.print(WiFi.RSSI(i));
    Serial.print("dBm)");
    Serial.println(WiFi.encryptionType(i) == ENC_TYPE_NONE ? " [Open]" : " [Encrypted]");
  }
  Serial.println();
  
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
    Serial.print("Please connect to ");
    Serial.print(dynamicAPName);
    Serial.println(" hotspot and configure WiFi");
    wifiManager.autoConnect(dynamicAPName.c_str(), AP_PASSWORD);
  }
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
  
  modbus.begin(config.modbusSlaveId, rs485Serial);
  Serial.print("Modbus slave ID: ");
  Serial.println(config.modbusSlaveId);
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
  Serial.println("Loading configuration from EEPROM...");
  
  // 检查配置有效性标志
  if (EEPROM.read(CONFIG_VALID_ADDR) != 0xAA) {
    Serial.println("No valid config found in EEPROM, using defaults");
    setDefaultConfig();
    saveConfig(); // 保存默认配置到EEPROM
    return;
  }
  
  // 读取WiFi配置
  for (int i = 0; i < 32; i++) {
    config.ssid[i] = EEPROM.read(WIFI_SSID_ADDR + i);
  }
  for (int i = 0; i < 64; i++) {
    config.password[i] = EEPROM.read(WIFI_PASS_ADDR + i);
  }
  
  // 读取MQTT配置
  for (int i = 0; i < 64; i++) {
    config.mqttServer[i] = EEPROM.read(MQTT_SERVER_ADDR + i);
  }
  config.mqttPort = EEPROM.read(MQTT_PORT_ADDR) | (EEPROM.read(MQTT_PORT_ADDR + 1) << 8);
  
  for (int i = 0; i < 64; i++) {
    config.mqttTopic[i] = EEPROM.read(MQTT_TOPIC_ADDR + i);
  }
  for (int i = 0; i < 32; i++) {
    config.mqttUsername[i] = EEPROM.read(MQTT_USERNAME_ADDR + i);
  }
  for (int i = 0; i < 32; i++) {
    config.mqttPassword[i] = EEPROM.read(MQTT_PASSWORD_ADDR + i);
  }
  
  // 读取设备ID
  for (int i = 0; i < 32; i++) {
    config.deviceId[i] = EEPROM.read(DEVICE_ID_ADDR + i);
  }
  
  // 读取Web认证配置
  for (int i = 0; i < 16; i++) {
    config.webUsername[i] = EEPROM.read(WEB_USERNAME_ADDR + i);
  }
  for (int i = 0; i < 16; i++) {
    config.webPassword[i] = EEPROM.read(WEB_PASSWORD_ADDR + i);
  }
  
  // 读取端口配置
  config.rawTcpPort = EEPROM.read(RAW_TCP_PORT_ADDR) | (EEPROM.read(RAW_TCP_PORT_ADDR + 1) << 8);
  config.modbusTcpPort = EEPROM.read(MODBUS_TCP_PORT_ADDR) | (EEPROM.read(MODBUS_TCP_PORT_ADDR + 1) << 8);
  
  // 读取Modbus配置
  config.modbusSlaveId = EEPROM.read(MODBUS_SLAVE_ID_ADDR);
  config.modbusBaudRate = EEPROM.read(MODBUS_BAUD_RATE_ADDR) | 
                         (EEPROM.read(MODBUS_BAUD_RATE_ADDR + 1) << 8) |
                         (EEPROM.read(MODBUS_BAUD_RATE_ADDR + 2) << 16) |
                         (EEPROM.read(MODBUS_BAUD_RATE_ADDR + 3) << 24);
  
  // 读取启用状态标志
  config.mqttEnabled = EEPROM.read(MQTT_ENABLED_ADDR) == 1;
  config.tcpEnabled = EEPROM.read(TCP_ENABLED_ADDR) == 1;
  config.modbusTcpEnabled = EEPROM.read(MODBUS_TCP_ENABLED_ADDR) == 1;
  config.webAuthEnabled = EEPROM.read(WEB_AUTH_ENABLED_ADDR) == 1;
  
  config.valid = true;
  
  Serial.println("Configuration loaded from EEPROM successfully");
  Serial.println("=== Loaded Configuration ===");
  Serial.printf("WiFi SSID: %s\n", config.ssid);
  Serial.printf("Device ID: %s\n", config.deviceId);
  Serial.printf("MQTT Server: %s:%d\n", config.mqttServer, config.mqttPort);
  Serial.printf("Modbus Slave ID: %d, Baud Rate: %d\n", config.modbusSlaveId, config.modbusBaudRate);
  Serial.printf("Ports - Raw TCP: %d, Modbus TCP: %d\n", config.rawTcpPort, config.modbusTcpPort);
  Serial.printf("Services - MQTT: %s, TCP: %s, Modbus TCP: %s, Web Auth: %s\n",
                config.mqttEnabled ? "ON" : "OFF",
                config.tcpEnabled ? "ON" : "OFF", 
                config.modbusTcpEnabled ? "ON" : "OFF",
                config.webAuthEnabled ? "ON" : "OFF");
  Serial.println("============================");
  
  config.valid = true;
  
  Serial.println("Configuration loaded successfully");
}

void setDefaultConfig() {
  strcpy(config.ssid, DEFAULT_SSID);
  strcpy(config.password, DEFAULT_PASSWORD);
  strcpy(config.mqttServer, MQTT_BROKER);
  config.mqttPort = MQTT_PORT;
  strcpy(config.mqttTopic, MQTT_TOPIC_BASE);
  strcpy(config.mqttUsername, MQTT_USERNAME);
  strcpy(config.mqttPassword, MQTT_PASSWORD);
  strcpy(config.deviceId, DEFAULT_DEVICE_ID);
  strcpy(config.webUsername, "admin");    // 默认Web用户名
  strcpy(config.webPassword, "admin");    // 默认Web密码
  config.rawTcpPort = RAW_TCP_PORT;       // 默认原始TCP端口
  config.modbusTcpPort = MODBUS_TCP_PORT; // 默认Modbus TCP端口
  config.modbusSlaveId = MODBUS_SLAVE_ID; // 默认Modbus从站地址
  config.modbusBaudRate = MODBUS_BAUD;    // 默认Modbus波特率
  config.mqttEnabled = false;      // 默认关闭MQTT - 用户按需开启
  config.tcpEnabled = false;       // 默认关闭TCP - 用户按需开启
  config.modbusTcpEnabled = false; // 默认关闭Modbus TCP - 用户按需开启
  config.webAuthEnabled = true;    // 默认开启Web认证 - 提高安全性
  config.valid = true;
  
  saveConfig();
}

void saveConfig() {
  Serial.println("Saving configuration to EEPROM...");
  
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
  
  for (int i = 0; i < 64; i++) {
    EEPROM.write(MQTT_TOPIC_ADDR + i, config.mqttTopic[i]);
  }
  for (int i = 0; i < 32; i++) {
    EEPROM.write(MQTT_USERNAME_ADDR + i, config.mqttUsername[i]);
  }
  for (int i = 0; i < 32; i++) {
    EEPROM.write(MQTT_PASSWORD_ADDR + i, config.mqttPassword[i]);
  }
  
  // 写入设备ID
  for (int i = 0; i < 32; i++) {
    EEPROM.write(DEVICE_ID_ADDR + i, config.deviceId[i]);
  }
  
  // 写入Web认证配置
  for (int i = 0; i < 16; i++) {
    EEPROM.write(WEB_USERNAME_ADDR + i, config.webUsername[i]);
  }
  for (int i = 0; i < 16; i++) {
    EEPROM.write(WEB_PASSWORD_ADDR + i, config.webPassword[i]);
  }
  
  // 写入端口配置
  EEPROM.write(RAW_TCP_PORT_ADDR, config.rawTcpPort & 0xFF);
  EEPROM.write(RAW_TCP_PORT_ADDR + 1, (config.rawTcpPort >> 8) & 0xFF);
  EEPROM.write(MODBUS_TCP_PORT_ADDR, config.modbusTcpPort & 0xFF);
  EEPROM.write(MODBUS_TCP_PORT_ADDR + 1, (config.modbusTcpPort >> 8) & 0xFF);
  
  // 写入Modbus配置
  EEPROM.write(MODBUS_SLAVE_ID_ADDR, config.modbusSlaveId);
  EEPROM.write(MODBUS_BAUD_RATE_ADDR, config.modbusBaudRate & 0xFF);
  EEPROM.write(MODBUS_BAUD_RATE_ADDR + 1, (config.modbusBaudRate >> 8) & 0xFF);
  EEPROM.write(MODBUS_BAUD_RATE_ADDR + 2, (config.modbusBaudRate >> 16) & 0xFF);
  EEPROM.write(MODBUS_BAUD_RATE_ADDR + 3, (config.modbusBaudRate >> 24) & 0xFF);
  
  // 写入启用状态标志
  EEPROM.write(MQTT_ENABLED_ADDR, config.mqttEnabled ? 1 : 0);
  EEPROM.write(TCP_ENABLED_ADDR, config.tcpEnabled ? 1 : 0);
  EEPROM.write(MODBUS_TCP_ENABLED_ADDR, config.modbusTcpEnabled ? 1 : 0);
  EEPROM.write(WEB_AUTH_ENABLED_ADDR, config.webAuthEnabled ? 1 : 0);
  
  // 写入有效性标志
  EEPROM.write(CONFIG_VALID_ADDR, 0xAA);
  
  EEPROM.commit();
  Serial.println("Configuration saved to EEPROM successfully");
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
  if (rawTcpServer) {
    rawTcpServer->begin();
  }
}

void stopTCP() {
  Serial.println("Stopping raw TCP server...");
  if (rawTcpServer) {
    rawTcpServer->stop();
  }
}

void startModbusTCP() {
  Serial.println("Starting Modbus TCP server...");
  if (modbusServer) {
    modbusServer->begin();
  }
}

void stopModbusTCP() {
  Serial.println("Stopping Modbus TCP server...");
  if (modbusServer) {
    modbusServer->stop();
  }
}

// 简单认证检查函数
bool checkAuthentication() {
  if (!config.webAuthEnabled) {
    return true;  // 如果认证未启用，直接通过
  }
  
  if (!server.authenticate(config.webUsername, config.webPassword)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

// 串口调试心跳函数
void printDebugHeartbeat() {
  Serial.println();
  Serial.println("=== System Status Heartbeat ===");
  Serial.print("Uptime: ");
  Serial.print(millis() / 1000);
  Serial.println(" seconds");
  
  // WiFi状态
  Serial.print("WiFi: ");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected (");
    Serial.print(WiFi.SSID());
    Serial.print(", ");
    Serial.print(WiFi.localIP());
    Serial.print(", ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm)");
  } else {
    Serial.println("Disconnected");
  }
  
  // 继电器状态
  Serial.print("Relay Status: ");
  for (int i = 0; i < 4; i++) {
    Serial.print("JDQ");
    Serial.print(i);
    Serial.print("=");
    Serial.print(relayStates[i] ? "ON" : "OFF");
    if (i < 3) Serial.print(", ");
  }
  Serial.println();
  
  // Modbus配置
  Serial.print("Modbus: SlaveID=");
  Serial.print(config.modbusSlaveId);
  Serial.print(", RTU_Baud=");
  Serial.print(config.modbusBaudRate);
  Serial.print(", TCP_Port=");
  Serial.print(config.modbusTcpPort);
  Serial.print(" (");
  Serial.print(config.modbusTcpEnabled ? "Enabled" : "Disabled");
  Serial.println(")");
  
  // 内存使用
  Serial.print("Memory: Free=");
  Serial.print(ESP.getFreeHeap());
  Serial.print(" bytes, Usage=");
  Serial.print(100 - (ESP.getFreeHeap() * 100 / 81920));
  Serial.println("%");
  
  // 服务状态
  Serial.print("Services: Web=Running");
  if (config.mqttEnabled) {
    Serial.print(", MQTT=");
    Serial.print(mqttClient.connected() ? "Connected" : "Disconnected");
  }
  if (config.tcpEnabled) {
    Serial.print(", TCP=Running");
  }
  if (config.modbusTcpEnabled) {
    Serial.print(", ModbusTCP=Running");
  }
  Serial.println();
  Serial.println("===============================");
}
