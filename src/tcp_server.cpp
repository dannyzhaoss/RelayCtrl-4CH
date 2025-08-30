#include "config.h"
#include "relay_controller.h"
#include <WiFiServer.h>
#include <WiFiClient.h>

// TCP服务器处理 - 支持原始TCP控制和Modbus TCP

// TCP服务器配置 - 降低并发连接数以节省内存
#define MAX_CLIENTS 2  // 从4减少到2，节省内存

// 使用全局声明的服务器对象（在main.cpp中定义）
WiFiClient clients[MAX_CLIENTS];
WiFiClient rawClients[MAX_CLIENTS];

void initTcpServers() {
  Serial.println("Initializing TCP servers...");
  
  // 创建TCP服务器对象使用配置中的端口号
  if (modbusServer) {
    modbusServer->stop();
    delete modbusServer;
  }
  if (rawTcpServer) {
    rawTcpServer->stop();
    delete rawTcpServer;
  }
  
  modbusServer = new WiFiServer(config.modbusTcpPort);
  rawTcpServer = new WiFiServer(config.rawTcpPort);
  
  modbusServer->begin();
  rawTcpServer->begin();
  
  Serial.print("Modbus TCP server started on port ");
  Serial.println(config.modbusTcpPort);
  Serial.print("Raw TCP server started on port ");
  Serial.println(config.rawTcpPort);
}

void restartTcpServers() {
  Serial.println("Restarting TCP servers with new configuration...");
  initTcpServers();
}

void handleTcpClients() {
  handleModbusTcpClients();
  handleRawTcpClients();
}

void handleModbusTcpClients() {
  if (!modbusServer) return;
  
  // 检查新的客户端连接
  WiFiClient newClient = modbusServer->accept();
  if (newClient) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (!clients[i] || !clients[i].connected()) {
        clients[i] = newClient;
        Serial.print("New Modbus TCP client connected: ");
        Serial.println(newClient.remoteIP());
        break;
      }
    }
  }
  
  // 处理现有客户端的数据
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i] && clients[i].connected()) {
      if (clients[i].available()) {
        handleModbusTcpRequest(clients[i]);
      }
    } else if (clients[i]) {
      clients[i].stop();
    }
  }
}

void handleModbusTcpRequest(WiFiClient& client) {
  static uint8_t buffer[64]; // 减少缓冲区大小 - Modbus基本请求通常不超过64字节
  int bytesRead = 0;
  
  // 读取Modbus TCP ADU - 限制最大读取长度
  while (client.available() && bytesRead < sizeof(buffer)) {
    buffer[bytesRead++] = client.read();
    
    // 检查是否收到完整的MBAP头部
    if (bytesRead >= 6) {
      uint16_t length = (buffer[4] << 8) | buffer[5];
      
      // 防止长度字段异常大
      if (length > 250 || length < 1) {
        Serial.println("Invalid TCP frame length");
        return;
      }
      
      if (bytesRead >= length + 6) {
        // 收到完整的Modbus TCP帧
        processModbusTcpFrame(client, buffer, bytesRead);
        return;
      }
    }
  }
}

void processModbusTcpFrame(WiFiClient& client, uint8_t* buffer, int length) {
  // Modbus TCP ADU格式:
  // [Transaction ID (2 bytes)][Protocol ID (2 bytes)][Length (2 bytes)][Unit ID (1 byte)][PDU]
  
  uint16_t transactionId = (buffer[0] << 8) | buffer[1];
  uint16_t protocolId = (buffer[2] << 8) | buffer[3];
  uint16_t frameLength = (buffer[4] << 8) | buffer[5];
  uint8_t unitId = buffer[6];
  
  Serial.print("Modbus TCP - Transaction: ");
  Serial.print(transactionId);
  Serial.print(", Protocol: ");
  Serial.print(protocolId);
  Serial.print(", Length: ");
  Serial.print(frameLength);
  Serial.print(", Unit: ");
  Serial.print(unitId);
  Serial.print(", Config Unit: ");
  Serial.println(config.modbusSlaveId);
  
  if (protocolId != 0) {
    Serial.println("Invalid protocol ID");
    return;
  }
  
  // 临时禁用Unit ID检查进行测试
  /*
  if (unitId != config.modbusSlaveId) {
    Serial.println("Unit ID mismatch, ignoring frame");
    return;
  }
  */
  Serial.println("Processing TCP frame (Unit ID check disabled for testing)");
  
  // 提取PDU部分
  uint8_t* pdu = buffer + 7;
  int pduLength = frameLength - 1;
  
  uint8_t functionCode = pdu[0];
  uint8_t response[260];
  int responseLength = 0;
  
  // 复制MBAP头部到响应
  response[0] = buffer[0]; // Transaction ID High
  response[1] = buffer[1]; // Transaction ID Low
  response[2] = 0;         // Protocol ID High
  response[3] = 0;         // Protocol ID Low
  response[4] = 0;         // Length High (待填充)
  response[5] = 0;         // Length Low (待填充)
  response[6] = unitId;    // Unit ID
  
  responseLength = 7;
  
  switch (functionCode) {
    case 0x01: // Read Coils
      responseLength += processReadCoilsTcp(pdu, pduLength, response + 7);
      break;
    case 0x03: // Read Holding Registers
      responseLength += processReadHoldingRegistersTcp(pdu, pduLength, response + 7);
      break;
    case 0x05: // Write Single Coil
      responseLength += processWriteSingleCoilTcp(pdu, pduLength, response + 7);
      break;
    case 0x06: // Write Single Register
      responseLength += processWriteSingleRegisterTcp(pdu, pduLength, response + 7);
      break;
    case 0x0F: // Write Multiple Coils
      responseLength += processWriteMultipleCoilsTcp(pdu, pduLength, response + 7);
      break;
    default:
      // 发送异常响应
      response[7] = functionCode | 0x80;
      response[8] = 0x01; // 非法功能码
      responseLength = 9;
      break;
  }
  
  // 填充长度字段
  uint16_t pduResponseLength = responseLength - 6;
  response[4] = (pduResponseLength >> 8) & 0xFF;
  response[5] = pduResponseLength & 0xFF;
  
  // 发送响应
  client.write(response, responseLength);
  
  Serial.print("Modbus TCP response sent, length: ");
  Serial.println(responseLength);
}

int processReadCoilsTcp(uint8_t* pdu, int length, uint8_t* response) {
  uint16_t startAddress = (pdu[1] << 8) | pdu[2];
  uint16_t quantity = (pdu[3] << 8) | pdu[4];
  
  if (startAddress >= 4 || startAddress + quantity > 4) {
    response[0] = pdu[0] | 0x80; // 错误响应
    response[1] = 0x02; // 非法数据地址
    return 2;
  }
  
  response[0] = pdu[0]; // Function code
  response[1] = 1;      // Byte count
  response[2] = 0;      // Coil status
  
  for (int i = 0; i < quantity && i < 4; i++) {
    if (relayStates[startAddress + i]) {
      response[2] |= (1 << i);
    }
  }
  
  return 3;
}

int processReadHoldingRegistersTcp(uint8_t* pdu, int length, uint8_t* response) {
  uint16_t startAddress = (pdu[1] << 8) | pdu[2];
  uint16_t quantity = (pdu[3] << 8) | pdu[4];
  
  if (startAddress + quantity > 8) {
    response[0] = pdu[0] | 0x80;
    response[1] = 0x02;
    return 2;
  }
  
  response[0] = pdu[0];
  response[1] = quantity * 2;
  
  int responseIndex = 2;
  
  for (int i = 0; i < quantity; i++) {
    uint16_t value = 0;
    int regAddress = startAddress + i;
    
    if (regAddress < 4) {
      value = relayStates[regAddress] ? 1 : 0;
    } else if (regAddress == 4) {
      value = WiFi.status() == WL_CONNECTED ? 1 : 0;
      if (mqttClient.connected()) value |= 0x02;
    } else if (regAddress == 5) {
      value = abs(WiFi.RSSI());
    } else if (regAddress == 6) {
      value = (millis() / 1000) & 0xFFFF;
    } else if (regAddress == 7) {
      value = ((millis() / 1000) >> 16) & 0xFFFF;
    }
    
    response[responseIndex++] = (value >> 8) & 0xFF;
    response[responseIndex++] = value & 0xFF;
  }
  
  return responseIndex;
}

int processWriteSingleCoilTcp(uint8_t* pdu, int length, uint8_t* response) {
  uint16_t address = (pdu[1] << 8) | pdu[2];
  uint16_t value = (pdu[3] << 8) | pdu[4];
  
  Serial.print("TCP Write Single Coil - Address: ");
  Serial.print(address);
  Serial.print(", Value: 0x");
  Serial.println(value, HEX);
  
  if (address >= 4) {
    Serial.println("Invalid address");
    response[0] = pdu[0] | 0x80;
    response[1] = 0x02;
    return 2;
  }
  
  bool state = (value == 0xFF00);
  Serial.print("Setting relay ");
  Serial.print(address);
  Serial.print(" to ");
  Serial.println(state ? "ON" : "OFF");
  
  setRelay(address, state);
  
  // 回送原始请求
  memcpy(response, pdu, 5);
  Serial.println("TCP Write Single Coil response sent");
  return 5;
}

int processWriteSingleRegisterTcp(uint8_t* pdu, int length, uint8_t* response) {
  uint16_t address = (pdu[1] << 8) | pdu[2];
  uint16_t value = (pdu[3] << 8) | pdu[4];
  
  if (address < 4) {
    setRelay(address, value != 0);
    memcpy(response, pdu, 5);
    return 5;
  } else {
    response[0] = pdu[0] | 0x80;
    response[1] = 0x02;
    return 2;
  }
}

int processWriteMultipleCoilsTcp(uint8_t* pdu, int length, uint8_t* response) {
  uint16_t startAddress = (pdu[1] << 8) | pdu[2];
  uint16_t quantity = (pdu[3] << 8) | pdu[4];
  uint8_t byteCount = pdu[5];
  
  if (startAddress >= 4 || startAddress + quantity > 4) {
    response[0] = pdu[0] | 0x80;
    response[1] = 0x02;
    return 2;
  }
  
  uint8_t coilData = pdu[6];
  
  for (int i = 0; i < quantity && i < 4; i++) {
    bool state = (coilData & (1 << i)) != 0;
    setRelay(startAddress + i, state);
  }
  
  response[0] = pdu[0];
  response[1] = pdu[1];
  response[2] = pdu[2];
  response[3] = pdu[3];
  response[4] = pdu[4];
  
  return 5;
}

void handleRawTcpClients() {
  if (!rawTcpServer) return;
  
  // 检查新的客户端连接
  WiFiClient newClient = rawTcpServer->accept();
  if (newClient) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (!rawClients[i] || !rawClients[i].connected()) {
        rawClients[i] = newClient;
        Serial.print("New Raw TCP client connected: ");
        Serial.println(newClient.remoteIP());
        
        // 发送欢迎消息
        newClient.println("ESP8266 Relay Controller v1.0");
        newClient.println("Commands: relay <1-4> <on/off>, status, help");
        newClient.print("> ");
        break;
      }
    }
  }
  
  // 处理现有客户端的数据
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (rawClients[i] && rawClients[i].connected()) {
      if (rawClients[i].available()) {
        handleRawTcpCommand(rawClients[i]);
      }
    } else if (rawClients[i]) {
      // 清理断开的连接
      Serial.print("Cleaning up disconnected TCP client slot: ");
      Serial.println(i);
      rawClients[i].stop();
      rawClients[i] = WiFiClient(); // 重置客户端对象
    }
  }
}

void handleRawTcpCommand(WiFiClient& client) {
  String command = "";
  
  // 逐字符读取，避免缓冲区问题
  while (client.available()) {
    char c = client.read();
    if (c == '\n' || c == '\r') {
      break;
    }
    if (c >= 32 && c <= 126) { // 只接受可打印ASCII字符
      command += c;
    }
  }
  
  command.trim();
  
  if (command.length() == 0) {
    client.print("> ");
    return;
  }
  
  Serial.print("TCP command from ");
  Serial.print(client.remoteIP());
  Serial.print(": ");
  Serial.println(command);
  
  // 添加小延时避免快速命令冲突
  delay(10);
  
  if (command.startsWith("relay")) {
    handleTcpRelayCommand(client, command);
  } else if (command == "status") {
    sendTcpStatus(client);
  } else if (command == "help") {
    sendTcpHelp(client);
  } else if (command == "quit" || command == "exit") {
    client.println("Goodbye!");
    client.flush();
    delay(100); // 给客户端时间接收消息
    client.stop();
    Serial.print("TCP client disconnected: ");
    Serial.println(client.remoteIP());
    return;
  } else {
    client.println("Unknown command. Type 'help' for available commands.");
  }
  
  client.flush(); // 确保数据发送完成
  client.print("> ");
}

void handleTcpRelayCommand(WiFiClient& client, String command) {
  int spaceIndex1 = command.indexOf(' ');
  int spaceIndex2 = command.indexOf(' ', spaceIndex1 + 1);
  
  if (spaceIndex1 > 0 && spaceIndex2 > 0) {
    int relay = command.substring(spaceIndex1 + 1, spaceIndex2).toInt();
    String state = command.substring(spaceIndex2 + 1);
    state.trim();
    state.toLowerCase(); // 统一转为小写
    
    if (relay >= 1 && relay <= 4) {
      bool relayState = (state == "on" || state == "1" || state == "true");
      
      // 立即设置继电器状态
      setRelay(relay - 1, relayState);
      
      // 确认状态变更
      bool currentState = relayStates[relay - 1];
      
      client.print("Relay ");
      client.print(relay);
      client.print(" (JDQ");
      client.print(relay - 1);
      client.print("): ");
      client.println(currentState ? "ON" : "OFF");
      
      // 如果状态不匹配，重试一次
      if (currentState != relayState) {
        delay(50);
        setRelay(relay - 1, relayState);
        client.print("Retry - Relay ");
        client.print(relay);
        client.print(": ");
        client.println(relayStates[relay - 1] ? "ON" : "OFF");
      }
    } else {
      client.println("Invalid relay number (1-4)");
    }
  } else {
    client.println("Usage: relay <1-4> <on/off>");
    client.println("Example: relay 1 on");
  }
}

void sendTcpStatus(WiFiClient& client) {
  client.println("=== System Status ===");
  client.print("Device ID: ");
  client.println(config.deviceId);
  client.print("WiFi: ");
  client.print(WiFi.SSID());
  client.print(" (");
  client.print(WiFi.RSSI());
  client.println(" dBm)");
  client.print("IP: ");
  client.println(WiFi.localIP());
  client.print("MQTT: ");
  client.println(mqttClient.connected() ? "Connected" : "Disconnected");
  client.print("Uptime: ");
  client.print(millis() / 1000);
  client.println(" seconds");
  
  client.println("Relay States:");
  for (int i = 0; i < 4; i++) {
    client.print("  JDQ");
    client.print(i);
    client.print(" (Relay ");
    client.print(i + 1);
    client.print("): ");
    client.println(relayStates[i] ? "ON" : "OFF");
  }
  client.println("===================");
}

void sendTcpHelp(WiFiClient& client) {
  client.println("=== Available Commands ===");
  client.println("relay <1-4> <on/off>  - Control relay");
  client.println("status               - Show system status");
  client.println("help                 - Show this help");
  client.println("quit/exit            - Disconnect");
  client.println("=========================");
}
