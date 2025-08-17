# RS485 (Modbus RTU) 命令参考

## 设备配置
- **从站地址 (Slave ID)**: 1
- **波特率**: 9600
- **数据位**: 8
- **停止位**: 1
- **校验位**: None (无)

## 支持的功能码

### 1. 读取线圈状态 (0x01 - Read Coils)
**描述**: 读取继电器当前状态

**命令格式**:
```
从站地址 | 功能码 | 起始地址高位 | 起始地址低位 | 线圈数量高位 | 线圈数量低位 | CRC低位 | CRC高位
```

**示例命令**:
```
读取所有4个继电器状态:
01 01 00 00 00 04 FD CA

读取单个继电器1状态:
01 01 00 00 00 01 FC 0A
```

**响应格式**:
```
从站地址 | 功能码 | 字节数 | 数据 | CRC低位 | CRC高位
```

**示例响应**:
```
全部继电器OFF: 01 01 01 00 51 88
继电器1 ON:   01 01 01 01 90 48
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

**示例命令**:
```
继电器1 ON:  01 05 00 00 FF 00 8C 3A
继电器1 OFF: 01 05 00 00 00 00 CD CA
继电器2 ON:  01 05 00 01 FF 00 DD FA
继电器2 OFF: 01 05 00 01 00 00 9C 0A
继电器3 ON:  01 05 00 02 FF 00 2D FA
继电器3 OFF: 01 05 00 02 00 00 6C 0A
继电器4 ON:  01 05 00 03 FF 00 7C 3A
继电器4 OFF: 01 05 00 03 00 00 3D CA
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

### Python示例
```python
import serial
import struct

# 连接RS485
ser = serial.Serial('COM1', 9600, timeout=1)

# 继电器1 ON
command = bytes([0x01, 0x05, 0x00, 0x00, 0xFF, 0x00, 0x8C, 0x3A])
ser.write(command)
response = ser.read(8)
print("Response:", response.hex())

# 读取所有继电器状态
command = bytes([0x01, 0x01, 0x00, 0x00, 0x00, 0x04, 0xFD, 0xCA])
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
- **Slave ID**: 1

## 注意事项
1. 所有命令都包含CRC校验，必须正确计算
2. 发送命令后等待100ms再发送下一条命令
3. 继电器状态变化会立即生效
4. 支持广播地址0x00（所有设备响应）
