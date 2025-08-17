#ifndef RELAY_CONTROLLER_H
#define RELAY_CONTROLLER_H

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <ModbusMaster.h>
#include <SoftwareSerial.h>
#include <WiFiServer.h>
#include <WiFiClient.h>

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

// 配置结构体
struct RelayConfig {
  char ssid[32];
  char password[64];
  char mqttServer[64];
  int mqttPort;
  char deviceId[32];
  bool mqttEnabled;     // MQTT协议启用状态
  bool tcpEnabled;      // 原始TCP协议启用状态
  bool modbusTcpEnabled; // Modbus TCP协议启用状态
  bool valid;
};

// 全局变量声明
extern bool relayStates[4];
extern RelayConfig config;
extern PubSubClient mqttClient;
extern ESP8266WebServer server;
extern SoftwareSerial rs485Serial;
extern ModbusMaster modbus;
extern unsigned long lastMqttReconnect;
extern unsigned long lastHeartbeat;

// TCP服务器对象
extern WiFiServer modbusServer;
extern WiFiServer rawTcpServer;

// Web服务器处理函数
void handleRoot();
void handleStatus();
void handleRelayControl();
void handleGetConfig();
void handleSetConfig();
void handleRestart();
void handleProtocolControl(); // 新增：协议控制处理

// MQTT处理函数
void reconnectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void handleMqttControl(String message);
void handleMqttConfig(String message);
void publishRelayState(int relay, bool state);
void publishSystemStatus();
void publishOnlineStatus();
void sendHeartbeat();
int getRelayPin(int relay);

// Modbus和串口处理函数
void rs485Write(uint8_t* data, int length);
void handleModbus();
void processModbusRequest();
bool validateModbusFrame(uint8_t* buffer, int length);
void processModbusFrame(uint8_t* buffer, int length);
void handleReadCoils(uint8_t* buffer, int length);
void handleWriteSingleCoil(uint8_t* buffer, int length);
void handleWriteMultipleCoils(uint8_t* buffer, int length);
void handleReadHoldingRegisters(uint8_t* buffer, int length);
void handleWriteSingleRegister(uint8_t* buffer, int length);
void sendModbusError(uint8_t slaveId, uint8_t functionCode, uint8_t exceptionCode);
uint16_t calculateCRC(uint8_t* buffer, int length);
void handleSerialCommands();
void handleSerialRelayCommand(String command);
void handleSerialConfigCommand(String command);
void setConfigParameter(String param, String value);
void printCurrentConfig();
void printSystemStatus();
void printSerialHelp();

// TCP服务器处理函数
void initTcpServers();
void handleTcpClients();
void handleModbusTcpClients();
void handleModbusTcpRequest(WiFiClient& client);
void processModbusTcpFrame(WiFiClient& client, uint8_t* buffer, int length);
int processReadCoilsTcp(uint8_t* pdu, int length, uint8_t* response);
int processReadHoldingRegistersTcp(uint8_t* pdu, int length, uint8_t* response);
int processWriteSingleCoilTcp(uint8_t* pdu, int length, uint8_t* response);
int processWriteSingleRegisterTcp(uint8_t* pdu, int length, uint8_t* response);
int processWriteMultipleCoilsTcp(uint8_t* pdu, int length, uint8_t* response);
void handleRawTcpClients();
void handleRawTcpCommand(WiFiClient& client);
void handleTcpRelayCommand(WiFiClient& client, String command);
void sendTcpStatus(WiFiClient& client);
void sendTcpHelp(WiFiClient& client);

// 主程序函数
void setRelay(int relay, bool state);
void loadConfig();
void setDefaultConfig();
void saveConfig();
void printSystemInfo();
void initRelays();
void initSerialPorts();
void initWiFi();
void initWebServer();
void initMQTT();
void initModbus();

// 协议控制函数
void enableMQTT(bool enable);
void enableTCP(bool enable);
void enableModbusTCP(bool enable);
void stopMQTT();
void stopTCP();
void stopModbusTCP();
void startMQTT();
void startTCP();
void startModbusTCP();

#endif
