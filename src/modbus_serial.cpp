#include "config.h"
#include "relay_controller.h"
#include <ArduinoJson.h>

// RS485写入函数 - 无需方向控制
void rs485Write(uint8_t* data, int length) {
  rs485Serial.write(data, length);
  rs485Serial.flush(); // 等待发送完成
}

// Modbus和串口处理函数

void handleModbus() {
  // 检查是否有Modbus请求
  if (rs485Serial.available()) {
    processModbusRequest();
  }
}

void processModbusRequest() {
  static uint8_t buffer[256];
  static int bufferIndex = 0;
  static unsigned long lastReceive = 0;
  
  while (rs485Serial.available() && bufferIndex < 256) {
    buffer[bufferIndex++] = rs485Serial.read();
    lastReceive = millis();
    
    // 打印接收到的每个字节用于调试
    Serial.print("RX byte[");
    Serial.print(bufferIndex-1);
    Serial.print("]: 0x");
    Serial.println(buffer[bufferIndex-1], HEX);
    
    // 简单的帧检测 - 检查是否收到完整的Modbus帧
    if (bufferIndex >= 8) { // 最小Modbus RTU帧长度
      Serial.print("Checking frame, length: ");
      Serial.print(bufferIndex);
      Serial.print(", slave ID: ");
      Serial.print(buffer[0]);
      Serial.print(", config slave ID: ");
      Serial.println(config.modbusSlaveId);
      
      if (validateModbusFrame(buffer, bufferIndex)) {
        Serial.println("Frame validated, processing...");
        processModbusFrame(buffer, bufferIndex);
        bufferIndex = 0;
        return; // 立即返回，不等待更多数据
      } else {
        Serial.println("Frame validation failed");
        // 重要修复：验证失败时清空缓冲区
        bufferIndex = 0;
      }
    }
  }
  
  // 减少超时时间，提高响应速度
  if (bufferIndex > 0 && millis() - lastReceive > 50) { // 从100ms减少到50ms
    Serial.print("Modbus frame timeout, clearing buffer. Length: ");
    Serial.println(bufferIndex);
    for (int i = 0; i < bufferIndex; i++) {
      Serial.print("0x");
      Serial.print(buffer[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    bufferIndex = 0; // 重置缓冲区
  }
}

bool validateModbusFrame(uint8_t* buffer, int length) {
  if (length < 8) return false;
  
  uint8_t slaveId = buffer[0];
  if (slaveId != config.modbusSlaveId) return false;
  
  // 简单的CRC检查（这里简化处理）
  return true;
}

void processModbusFrame(uint8_t* buffer, int length) {
  uint8_t slaveId = buffer[0];
  uint8_t functionCode = buffer[1];
  
  Serial.print("Modbus request - Slave: ");
  Serial.print(slaveId);
  Serial.print(", Function: ");
  Serial.println(functionCode);
  
  switch (functionCode) {
    case 0x01: // Read Coils (读线圈)
      handleReadCoils(buffer, length);
      break;
    case 0x05: // Write Single Coil (写单个线圈)
      handleWriteSingleCoil(buffer, length);
      break;
    case 0x0F: // Write Multiple Coils (写多个线圈)
      handleWriteMultipleCoils(buffer, length);
      break;
    case 0x03: // Read Holding Registers (读保持寄存器)
      handleReadHoldingRegisters(buffer, length);
      break;
    case 0x06: // Write Single Register (写单个寄存器)
      handleWriteSingleRegister(buffer, length);
      break;
    default:
      sendModbusError(slaveId, functionCode, 0x01); // 非法功能码
      break;
  }
}

void handleReadCoils(uint8_t* buffer, int length) {
  uint16_t startAddress = (buffer[2] << 8) | buffer[3];
  uint16_t quantity = (buffer[4] << 8) | buffer[5];
  
  if (startAddress >= 4 || startAddress + quantity > 4) {
    sendModbusError(buffer[0], buffer[1], 0x02); // 非法数据地址
    return;
  }
  
  uint8_t response[6];
  response[0] = buffer[0]; // Slave ID
  response[1] = buffer[1]; // Function Code
  response[2] = 1; // Byte count
  response[3] = 0; // Coil status
  
  // 设置继电器状态位
  for (int i = 0; i < quantity && i < 4; i++) {
    if (relayStates[startAddress + i]) {
      response[3] |= (1 << i);
    }
  }
  
  // 计算并添加CRC（简化处理）
  uint16_t crc = calculateCRC(response, 4);
  response[4] = crc & 0xFF;
  response[5] = (crc >> 8) & 0xFF;
  
  rs485Write(response, 6);
  
  Serial.print("Modbus Read Coils response sent, status: 0x");
  Serial.println(response[3], HEX);
}

void handleWriteSingleCoil(uint8_t* buffer, int length) {
  uint16_t address = (buffer[2] << 8) | buffer[3];
  uint16_t value = (buffer[4] << 8) | buffer[5];
  
  if (address >= 4) {
    sendModbusError(buffer[0], buffer[1], 0x02); // 非法数据地址
    return;
  }
  
  bool state = (value == 0xFF00);
  
  Serial.print("Relay ");
  Serial.print(address + 1);
  Serial.print(" (JDQ");
  Serial.print(address);
  Serial.print("): ");
  Serial.println(state ? "ON" : "OFF");
  
  // 立即设置继电器状态，无延时
  setRelay(address, state);
  
  Serial.print("Modbus Write Single Coil - Address: ");
  Serial.print(address);
  Serial.print(", State: ");
  Serial.println(state ? "ON" : "OFF");
  
  // 立即回送原始请求作为响应
  rs485Write(buffer, length);
}

void handleWriteMultipleCoils(uint8_t* buffer, int length) {
  uint16_t startAddress = (buffer[2] << 8) | buffer[3];
  uint16_t quantity = (buffer[4] << 8) | buffer[5];
  uint8_t byteCount = buffer[6];
  
  if (startAddress >= 4 || startAddress + quantity > 4) {
    sendModbusError(buffer[0], buffer[1], 0x02); // 非法数据地址
    return;
  }
  
  uint8_t coilData = buffer[7];
  
  // 设置继电器状态
  for (int i = 0; i < quantity && i < 4; i++) {
    bool state = (coilData & (1 << i)) != 0;
    setRelay(startAddress + i, state);
  }
  
  // 发送响应
  uint8_t response[8];
  response[0] = buffer[0]; // Slave ID
  response[1] = buffer[1]; // Function Code
  response[2] = buffer[2]; // Start Address High
  response[3] = buffer[3]; // Start Address Low
  response[4] = buffer[4]; // Quantity High
  response[5] = buffer[5]; // Quantity Low
  
  uint16_t crc = calculateCRC(response, 6);
  response[6] = crc & 0xFF;
  response[7] = (crc >> 8) & 0xFF;
  
  rs485Write(response, 8);
  
  Serial.print("Modbus Write Multiple Coils - Start: ");
  Serial.print(startAddress);
  Serial.print(", Quantity: ");
  Serial.println(quantity);
}

void handleReadHoldingRegisters(uint8_t* buffer, int length) {
  uint16_t startAddress = (buffer[2] << 8) | buffer[3];
  uint16_t quantity = (buffer[4] << 8) | buffer[5];
  
  // 寄存器映射：
  // 0-3: 继电器状态
  // 4: 设备状态
  // 5: WiFi信号强度
  // 6-7: 运行时间(32位)
  
  if (startAddress + quantity > 8) {
    sendModbusError(buffer[0], buffer[1], 0x02); // 非法数据地址
    return;
  }
  
  uint8_t response[5 + quantity * 2 + 2]; // Header + data + CRC
  response[0] = buffer[0]; // Slave ID
  response[1] = buffer[1]; // Function Code
  response[2] = quantity * 2; // Byte count
  
  int responseIndex = 3;
  
  for (int i = 0; i < quantity; i++) {
    uint16_t value = 0;
    int regAddress = startAddress + i;
    
    if (regAddress < 4) {
      // 继电器状态寄存器
      value = relayStates[regAddress] ? 1 : 0;
    } else if (regAddress == 4) {
      // 设备状态寄存器
      value = WiFi.status() == WL_CONNECTED ? 1 : 0;
      if (mqttClient.connected()) value |= 0x02;
    } else if (regAddress == 5) {
      // WiFi信号强度
      value = abs(WiFi.RSSI());
    } else if (regAddress == 6) {
      // 运行时间低16位
      value = (millis() / 1000) & 0xFFFF;
    } else if (regAddress == 7) {
      // 运行时间高16位
      value = ((millis() / 1000) >> 16) & 0xFFFF;
    }
    
    response[responseIndex++] = (value >> 8) & 0xFF; // High byte
    response[responseIndex++] = value & 0xFF; // Low byte
  }
  
  // 计算并添加CRC
  uint16_t crc = calculateCRC(response, responseIndex);
  response[responseIndex++] = crc & 0xFF;
  response[responseIndex++] = (crc >> 8) & 0xFF;
  
  rs485Write(response, responseIndex);
  
  Serial.print("Modbus Read Holding Registers response sent, quantity: ");
  Serial.println(quantity);
}

void handleWriteSingleRegister(uint8_t* buffer, int length) {
  uint16_t address = (buffer[2] << 8) | buffer[3];
  uint16_t value = (buffer[4] << 8) | buffer[5];
  
  if (address < 4) {
    // 写继电器状态寄存器
    setRelay(address, value != 0);
    
    // 回送原始请求作为响应
    rs485Write(buffer, length);
    
    Serial.print("Modbus Write Single Register - Address: ");
    Serial.print(address);
    Serial.print(", Value: ");
    Serial.println(value);
  } else {
    sendModbusError(buffer[0], buffer[1], 0x02); // 非法数据地址
  }
}

void sendModbusError(uint8_t slaveId, uint8_t functionCode, uint8_t exceptionCode) {
  uint8_t response[5];
  response[0] = slaveId;
  response[1] = functionCode | 0x80; // 设置错误位
  response[2] = exceptionCode;
  
  uint16_t crc = calculateCRC(response, 3);
  response[3] = crc & 0xFF;
  response[4] = (crc >> 8) & 0xFF;
  
  rs485Write(response, 5);
  
  Serial.print("Modbus Error sent - Function: ");
  Serial.print(functionCode);
  Serial.print(", Exception: ");
  Serial.println(exceptionCode);
}

uint16_t calculateCRC(uint8_t* buffer, int length) {
  uint16_t crc = 0xFFFF;
  
  for (int pos = 0; pos < length; pos++) {
    crc ^= (uint16_t)buffer[pos];
    
    for (int i = 8; i != 0; i--) {
      if ((crc & 0x0001) != 0) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  
  return crc;
}

void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    Serial.print("Command received: ");
    Serial.println(command);
    
    if (command.startsWith("relay")) {
      handleSerialRelayCommand(command);
    } else if (command.startsWith("config")) {
      handleSerialConfigCommand(command);
    } else if (command == "status") {
      printSystemStatus();
    } else if (command == "restart") {
      Serial.println("Restarting...");
      delay(1000);
      ESP.restart();
    } else if (command == "help") {
      printSerialHelp();
    } else {
      Serial.println("Unknown command. Type 'help' for available commands.");
    }
  }
}

void handleSerialRelayCommand(String command) {
  // 解析命令格式: relay <1-4> <on/off>
  int spaceIndex1 = command.indexOf(' ');
  int spaceIndex2 = command.indexOf(' ', spaceIndex1 + 1);
  
  if (spaceIndex1 > 0 && spaceIndex2 > 0) {
    int relay = command.substring(spaceIndex1 + 1, spaceIndex2).toInt();
    String state = command.substring(spaceIndex2 + 1);
    
    if (relay >= 1 && relay <= 4) {
      bool relayState = (state == "on" || state == "ON" || state == "1");
      setRelay(relay - 1, relayState);
      Serial.print("Relay ");
      Serial.print(relay);
      Serial.print(" set to ");
      Serial.println(relayState ? "ON" : "OFF");
    } else {
      Serial.println("Invalid relay number (1-4)");
    }
  } else {
    Serial.println("Usage: relay <1-4> <on/off>");
  }
}

void handleSerialConfigCommand(String command) {
  if (command == "config") {
    printCurrentConfig();
  } else if (command.startsWith("config set")) {
    // 解析配置设置命令
    // 格式: config set <parameter> <value>
    int setIndex = command.indexOf("set");
    if (setIndex > 0) {
      String paramValue = command.substring(setIndex + 4);
      paramValue.trim();
      
      int spaceIndex = paramValue.indexOf(' ');
      if (spaceIndex > 0) {
        String param = paramValue.substring(0, spaceIndex);
        String value = paramValue.substring(spaceIndex + 1);
        
        setConfigParameter(param, value);
      }
    }
  } else {
    Serial.println("Usage: config [set <parameter> <value>]");
  }
}

void setConfigParameter(String param, String value) {
  if (param == "ssid") {
    value.toCharArray(config.ssid, 32);
    Serial.println("WiFi SSID updated");
  } else if (param == "password") {
    value.toCharArray(config.password, 64);
    Serial.println("WiFi password updated");
  } else if (param == "mqtt_server") {
    value.toCharArray(config.mqttServer, 64);
    Serial.println("MQTT server updated");
  } else if (param == "mqtt_port") {
    config.mqttPort = value.toInt();
    Serial.println("MQTT port updated");
  } else if (param == "device_id") {
    value.toCharArray(config.deviceId, 32);
    Serial.println("Device ID updated");
  } else {
    Serial.println("Unknown parameter");
    return;
  }
  
  saveConfig();
  Serial.println("Configuration saved. Restart to apply changes.");
}

void printCurrentConfig() {
  Serial.println("\n=== Current Configuration ===");
  Serial.print("Device ID: ");
  Serial.println(config.deviceId);
  Serial.print("WiFi SSID: ");
  Serial.println(config.ssid);
  Serial.print("MQTT Server: ");
  Serial.print(config.mqttServer);
  Serial.print(":");
  Serial.println(config.mqttPort);
  Serial.println("============================");
}

void printSystemStatus() {
  Serial.println("\n=== System Status ===");
  Serial.print("Device ID: ");
  Serial.println(config.deviceId);
  Serial.print("WiFi: ");
  Serial.print(WiFi.SSID());
  Serial.print(" (");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm)");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("MQTT: ");
  Serial.println(mqttClient.connected() ? "Connected" : "Disconnected");
  Serial.print("Uptime: ");
  Serial.print(millis() / 1000);
  Serial.println(" seconds");
  Serial.print("Free Heap: ");
  Serial.println(ESP.getFreeHeap());
  
  Serial.println("Relay States:");
  for (int i = 0; i < 4; i++) {
    Serial.print("  JDQ");
    Serial.print(i);
    Serial.print(" (Relay ");
    Serial.print(i + 1);
    Serial.print("): ");
    Serial.println(relayStates[i] ? "ON" : "OFF");
  }
  Serial.println("====================");
}

void printSerialHelp() {
  Serial.println("\n=== Available Commands ===");
  Serial.println("relay <1-4> <on/off>  - Control relay");
  Serial.println("config                - Show current config");
  Serial.println("config set <param> <value> - Set config parameter");
  Serial.println("  Parameters: ssid, password, mqtt_server, mqtt_port, device_id");
  Serial.println("status               - Show system status");
  Serial.println("restart              - Restart device");
  Serial.println("help                 - Show this help");
  Serial.println("=========================");
}
