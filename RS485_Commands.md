# Modbus RTU & TCP 命令参考

## Modbus RTU (RS485)

### 设备配置
- **从站地址 (Slave ID)**: 2 (可通过Web界面配置)
- **波特率**: 9600
- **数据位**: 8
- **停止位**: 1
- **校验位**: None (无)

### 硬件连接
```
ESP8266 NodeMCU    →    RS485模块
D1 (GPIO5)        →    RS485 TX
D2 (GPIO4)        →    RS485 RX
```

## 支持的功能码

### 1. 读取线圈状态 (0x01 - Read Coils)
**描述**: 读取继电器当前状态

**命令格式**:
```
从站地址 | 功能码 | 起始地址高位 | 起始地址低位 | 线圈数量高位 | 线圈数量低位 | CRC低位 | CRC高位
```

**示例命令** (从站地址2):
```
读取所有4个继电器状态:
02 01 00 00 00 04 78 3C

读取单个继电器1状态:
02 01 00 00 00 01 79 CC
```

**响应格式**:
```
从站地址 | 功能码 | 字节数 | 数据 | CRC低位 | CRC高位
```

**示例响应**:
```
全部继电器OFF: 02 01 01 00 A0 B8
继电器1 ON:   02 01 01 01 61 78
```

### 2. 写单个线圈 (0x05 - Write Single Coil)
**描述**: 控制单个继电器

**命令格式**:
```
从站地址 | 功能码 | 线圈地址高位 | 线圈地址低位 | 输出值高位 | 输出值低位 | CRC低位 | CRC高位
```

**输出值**:
- `0xFF00` = ON (打开)
- `0x0000` = OFF (关闭)

**示例命令** (更新为从站地址2):
```
继电器1 ON:  02 05 00 00 FF 00 8D 5A
继电器1 OFF: 02 05 00 00 00 00 CC 9A
继电器2 ON:  02 05 00 01 FF 00 DC 9A
继电器2 OFF: 02 05 00 01 00 00 9D 5A
继电器3 ON:  02 05 00 02 FF 00 2C 9A
继电器3 OFF: 02 05 00 02 00 00 6D 5A
继电器4 ON:  02 05 00 03 FF 00 7D 5A
继电器4 OFF: 02 05 00 03 00 00 3C 9A
```

**响应**: 回显原命令

### 3. 写多个线圈 (0x0F - Write Multiple Coils)
**描述**: 同时控制多个继电器

**命令格式**:
```
从站地址 | 功能码 | 起始地址高位 | 起始地址低位 | 线圈数量高位 | 线圈数量低位 | 字节数 | 数据 | CRC低位 | CRC高位
```

**示例命令**:
```
全部继电器ON:  01 0F 00 00 00 04 01 0F B5 CC
全部继电器OFF: 01 0F 00 00 00 04 01 00 F4 0C
继电器1,3 ON:  01 0F 00 00 00 04 01 05 F4 6C
```

**数据字节解释** (位映射):
- 位0 = 继电器1
- 位1 = 继电器2  
- 位2 = 继电器3
- 位3 = 继电器4

## 继电器地址映射
```
继电器1 (JDQ0) -> Modbus地址 0x0000
继电器2 (JDQ1) -> Modbus地址 0x0001
继电器3 (JDQ2) -> Modbus地址 0x0002
继电器4 (JDQ3) -> Modbus地址 0x0003
```

## 错误码
- `0x01`: 非法功能码
- `0x02`: 非法数据地址
- `0x03`: 非法数据值

## 使用工具测试

### Python示例 (RTU)
```python
import serial
import struct

# 连接RS485
ser = serial.Serial('COM1', 9600, timeout=1)

# 继电器1 ON (从站地址2)
command = bytes([0x02, 0x05, 0x00, 0x00, 0xFF, 0x00, 0x8D, 0x5A])
ser.write(command)
response = ser.read(8)
print("Response:", response.hex())

# 读取所有继电器状态 (从站地址2)
command = bytes([0x02, 0x01, 0x00, 0x00, 0x00, 0x04, 0x78, 0x3C])
ser.write(command)
response = ser.read(6)
print("Relay Status:", response.hex())
```

### ModbusPoll/ModbusSlave工具配置
- **Connection**: Serial RTU
- **Port**: 选择对应COM口
- **Baud**: 9600
- **Data bits**: 8
- **Parity**: None
- **Stop bits**: 1
- **Slave ID**: 2

## Modbus TCP 支持

### 连接信息
- **IP地址**: 设备WiFi IP (如: 192.168.0.132)
- **端口**: 502 (可通过Web界面配置)
- **Unit ID**: 2 (与RTU从站地址相同)

### TCP帧格式
```
[Transaction ID][Protocol ID][Length][Unit ID][Function Code][Data]
   2字节        2字节      2字节   1字节     1字节       N字节
```
**注意**: TCP格式无CRC校验，依靠TCP协议可靠性

### 常用TCP命令

#### 读取继电器状态
```
00 05 00 00 00 06 02 01 00 00 00 04
```

#### 继电器控制命令

继电器1 ON:
```
00 01 00 00 00 06 02 05 00 00 FF 00
```

继电器1 OFF:
```
00 01 00 00 00 06 02 05 00 00 00 00
```

继电器2 ON:
```
00 02 00 00 00 06 02 05 00 01 FF 00
```

继电器2 OFF:
```
00 02 00 00 00 06 02 05 00 01 00 00
```

继电器3 ON:
```
00 03 00 00 00 06 02 05 00 02 FF 00
```

继电器3 OFF:
```
00 03 00 00 00 06 02 05 00 02 00 00
```

继电器4 ON:
```
00 04 00 00 00 06 02 05 00 03 FF 00
```

继电器4 OFF:
```
00 04 00 00 00 06 02 05 00 03 00 00
```

#### 读取系统状态
```
00 06 00 00 00 06 02 03 00 00 00 08
```

**寄存器映射**:
- 寄存器 0-3: 继电器状态 (0=OFF, 1=ON)
- 寄存器 4: 系统状态 (bit0=WiFi, bit1=MQTT)
- 寄存器 5: WiFi信号强度 (RSSI绝对值)
- 寄存器 6-7: 运行时间 (32位秒数)

### TCP测试工具

**ModbusPoll配置**:
1. Connection: TCP/IP
2. IP Address: 设备IP地址
3. Port: 502
4. Unit ID: 2

**Python示例**:
```python
from pymodbus.client import ModbusTcpClient

# 连接设备
client = ModbusTcpClient('192.168.0.132', port=502)

# 继电器1 ON
result = client.write_coil(0, True, slave=2)
print(f"继电器1控制: {result}")

# 读取所有继电器状态
result = client.read_coils(0, 4, slave=2)
print(f"继电器状态: {result.bits}")

client.close()
```

## 注意事项
1. 所有命令都包含CRC校验，必须正确计算
2. 发送命令后等待100ms再发送下一条命令
3. 继电器状态变化会立即生效
4. 支持广播地址0x00（所有设备响应）
