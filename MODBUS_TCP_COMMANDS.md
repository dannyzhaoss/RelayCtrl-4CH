# Modbus TCP 命令快速参考

## 连接信息
- **设备IP**: 192.168.0.132 (示例)
- **端口**: 502
- **Unit ID**: 2
- **协议**: Modbus TCP (无CRC校验)

## 帧格式
```
[Transaction ID][Protocol ID][Length][Unit ID][Function Code][Data]
   2字节        2字节      2字节   1字节     1字节       N字节
```

## 继电器控制命令 (功能码 0x05)

### 继电器1
```
ON:  00 01 00 00 00 06 02 05 00 00 FF 00
OFF: 00 01 00 00 00 06 02 05 00 00 00 00
```

### 继电器2
```
ON:  00 02 00 00 00 06 02 05 00 01 FF 00
OFF: 00 02 00 00 00 06 02 05 00 01 00 00
```

### 继电器3
```
ON:  00 03 00 00 00 06 02 05 00 02 FF 00
OFF: 00 03 00 00 00 06 02 05 00 02 00 00
```

### 继电器4
```
ON:  00 04 00 00 00 06 02 05 00 03 FF 00
OFF: 00 04 00 00 00 06 02 05 00 03 00 00
```

## 状态查询命令

### 读取继电器状态 (功能码 0x01)
```
00 05 00 00 00 06 02 01 00 00 00 04
```

### 读取系统寄存器 (功能码 0x03)
```
00 06 00 00 00 06 02 03 00 00 00 08
```

## 寄存器映射

| 地址 | 描述 | 数值说明 |
|------|------|----------|
| 0-3  | 继电器状态 | 0=OFF, 1=ON |
| 4    | 系统状态 | bit0=WiFi连接, bit1=MQTT连接 |
| 5    | WiFi信号强度 | RSSI绝对值 |
| 6-7  | 运行时间 | 32位秒数 |

## 测试工具使用

### ModbusPoll
1. Connection Type: TCP/IP
2. IP Address: 设备IP
3. Port: 502
4. Unit ID: 2
5. Function: 05 (Write Single Coil)
6. Address: 0-3 (继电器1-4)
7. Value: 65280 (ON) 或 0 (OFF)

### Python pymodbus
```python
from pymodbus.client import ModbusTcpClient

client = ModbusTcpClient('192.168.0.132', port=502)

# 继电器控制
client.write_coil(0, True, slave=2)   # 继电器1 ON
client.write_coil(0, False, slave=2)  # 继电器1 OFF

# 状态读取
result = client.read_coils(0, 4, slave=2)
print("继电器状态:", result.bits[:4])

client.close()
```

### Node.js modbus-serial
```javascript
const ModbusRTU = require("modbus-serial");
const client = new ModbusRTU();

client.connectTCP("192.168.0.132", { port: 502 })
  .then(() => client.setID(2))
  .then(() => client.writeCoil(0, true))  // 继电器1 ON
  .then(() => console.log("继电器1已开启"))
  .catch(console.error);
```

## 故障排除

### 常见问题
1. **连接失败**: 检查IP地址和端口502是否正确
2. **无响应**: 确认Unit ID为2
3. **控制失败**: 检查命令格式，特别是Transaction ID和Length字段

### 调试方法
1. 使用Wireshark抓包分析TCP数据
2. 检查设备串口输出的调试信息
3. 确认设备Web界面中的Modbus配置

---

更多详细信息请参考 [完整API文档](API_DOCUMENTATION.md) 和 [RS485命令文档](RS485_Commands.md)。
