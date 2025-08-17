#ifndef RELAY_CONTROLLER_H
#define RELAY_CONTROLLER_H

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <ModbusMaster.h>
#include <SoftwareSerial.h>
#include <WiFiServer.h>
#include <WiFiClient.h>
#include "config.h"  // 引入统一配置文件

// 配置结构体
struct RelayConfig {
  char ssid[32];
  char password[64];
  char mqttServer[64];
  int mqttPort;
  char mqttTopic[64];      // MQTT主题前缀
  char mqttUsername[32];   // MQTT用户名
  char mqttPassword[32];   // MQTT密码
  char deviceId[32];
  char webUsername[16];    // Web认证用户名
  char webPassword[16];    // Web认证密码
  int rawTcpPort;          // 原始TCP端口号
  int modbusTcpPort;       // Modbus TCP端口号
  bool mqttEnabled;        // MQTT协议启用状态
  bool tcpEnabled;         // 原始TCP协议启用状态
  bool modbusTcpEnabled;   // Modbus TCP协议启用状态
  bool webAuthEnabled;     // Web认证启用状态
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
extern String dynamicMqttClientId;

// TCP服务器对象指针
extern WiFiServer* modbusServer;
extern WiFiServer* rawTcpServer;

// Web服务器处理函数
void handleRoot();
void handleConfigPage();  // 新增：配置页面
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
void restartTcpServers(); // 新增：重启TCP服务器
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
bool checkAuthentication();  // 新增：检查Web认证
void stopMQTT();
void stopTCP();
void stopModbusTCP();
void startMQTT();
void startTCP();
void startModbusTCP();

#endif
