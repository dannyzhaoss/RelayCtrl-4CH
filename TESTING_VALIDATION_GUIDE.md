# 🔬 RelayCtrl-4CH 测试与验证规范

## 📋 测试概览

本文档提供ESP8266 RelayCtrl-4CH四路继电器控制器的完整测试指南，包括单元测试、集成测试、性能测试和验证方法。

## 🧪 测试分类

### 1. 单元测试

#### 硬件功能测试
- **继电器控制测试**
- **GPIO状态验证**
- **串口通信测试**
- **配置读写测试**

#### 软件模块测试
- **网络连接测试**
- **协议处理测试**
- **错误处理测试**
- **内存管理测试**

### 2. 集成测试

#### 协议集成测试
- **Web + 继电器控制**
- **MQTT + 状态发布**
- **Modbus + 寄存器读写**
- **TCP + 命令处理**

#### 系统集成测试
- **多协议并发测试**
- **配置持久化测试**
- **OTA升级测试**
- **故障恢复测试**

## 🔧 硬件测试指南

### 继电器控制测试

#### 测试用例1: 单独继电器控制
```bash
# 通过Web界面测试
curl -X POST http://192.168.1.100/api/relay/control \
  -H "Content-Type: application/json" \
  -d '{"relay": 1, "state": true}'

# 预期结果
# - 继电器1（JDQ0）应该响应
# - 对应LED指示灯点亮
# - Web界面状态更新
# - 串口输出: "RELAY: R1=ON"
```

#### 测试用例2: 批量继电器控制
```bash
# 全部开启
curl -X POST http://192.168.1.100/api/relay/control \
  -H "Content-Type: application/json" \
  -d '{"command": "all_on"}'

# 预期结果
# - 所有继电器响应
# - 所有LED指示灯点亮
# - 串口输出: "RELAY: R1=ON" ... "RELAY: R4=ON"
```

### GPIO引脚映射验证

| 继电器 | GPIO | NodeMCU引脚 | 测试方法 |
|--------|------|-------------|----------|
| JDQ0 | GPIO12 | D6 | 万用表测量继电器线圈电压 |
| JDQ1 | GPIO13 | D7 | 万用表测量继电器线圈电压 |
| JDQ2 | GPIO14 | D5 | 万用表测量继电器线圈电压 |
| JDQ3 | GPIO16 | D0 | 万用表测量继电器线圈电压 |

### RS485通信测试

#### 硬件连接验证
```
ESP8266 NodeMCU    →    RS485模块
D1 (GPIO5)         →    RS485 TX
D2 (GPIO4)         →    RS485 RX  
D4 (GPIO2)         →    RS485 DE/RE
```

#### 通信测试
```bash
# 使用Modbus调试工具
# 从站地址: 1 (默认)
# 波特率: 9600
# 数据位: 8, 停止位: 1, 无校验

# 读取线圈状态 (功能码0x01)
01 01 00 00 00 04 BD CA

# 写单个线圈 (功能码0x05)
01 05 00 00 FF 00 8C 3A  # 开启继电器1
01 05 00 00 00 00 CD CA  # 关闭继电器1
```

## 🌐 网络协议测试

### Web接口测试

#### 基础连接测试
```bash
# 检查Web服务器响应
curl -I http://192.168.1.100/

# 预期响应
HTTP/1.1 200 OK
Content-Type: text/html
```

#### API功能测试
```bash
# 获取系统状态
curl http://192.168.1.100/api/status

# 预期JSON响应
{
  "success": true,
  "data": {
    "deviceId": "RelayCtrl_001",
    "ip": "192.168.1.100",
    "uptime": 3600,
    "freeHeap": 25000,
    "relays": [...]
  }
}
```

#### 认证功能测试
```bash
# 无认证访问(应该被拒绝)
curl http://192.168.1.100/config

# 有认证访问
curl -u admin:admin http://192.168.1.100/config
```

### MQTT通信测试

#### 连接测试
```bash
# 使用MQTT客户端工具连接
mosquitto_sub -h 192.168.1.100 -p 1883 -t "relay/+/+/+"

# 预期主题
relay/RelayCtrl_001/relay1/state
relay/RelayCtrl_001/relay2/state  
relay/RelayCtrl_001/status
relay/RelayCtrl_001/online
relay/RelayCtrl_001/heartbeat
```

#### 控制命令测试
```bash
# 发送继电器控制命令
mosquitto_pub -h 192.168.1.100 -p 1883 \
  -t "relay/RelayCtrl_001/control" \
  -m '{"relay": 1, "state": true}'

# 发送批量控制命令
mosquitto_pub -h 192.168.1.100 -p 1883 \
  -t "relay/RelayCtrl_001/control" \
  -m '{"command": "all_off"}'
```

### TCP服务器测试

#### 原始TCP测试 (端口8080)
```bash
# 使用telnet连接
telnet 192.168.1.100 8080

# 发送命令
RELAY 1 ON
STATUS  
HELP

# 预期响应
OK: Relay 1 ON
Device: RelayCtrl_001, IP: 192.168.1.100, Uptime: 3600s
Available commands: RELAY <1-4|ALL> <ON|OFF>, STATUS, HELP
```

#### Modbus TCP测试 (端口502)
```bash
# 使用Modbus TCP客户端
# 读取线圈状态 (功能码0x01)
事务ID: 0001, 协议ID: 0000, 长度: 0006, 单元ID: 01
PDU: 01 00 00 00 04

# 写单个线圈 (功能码0x05)  
事务ID: 0002, 协议ID: 0000, 长度: 0006, 单元ID: 01
PDU: 05 00 00 FF 00
```

## 📊 性能测试

### 内存使用测试

#### 内存监控
```cpp
// 在串口监视器中观察
void printMemoryStatus() {
    Serial.printf("Free Heap: %u bytes\n", ESP.getFreeHeap());
    Serial.printf("Heap Fragmentation: %u%%\n", ESP.getHeapFragmentation());
}
```

#### 内存压力测试
- **并发连接测试**: 同时建立多个TCP连接
- **大数据传输测试**: 发送大JSON负载
- **长时间运行测试**: 24小时连续运行

### 网络性能测试

#### 响应时间测试
```bash
# Web接口响应时间
for i in {1..100}; do
  curl -w "%{time_total}\n" -o /dev/null -s http://192.168.1.100/api/status
done | awk '{sum+=$1; count++} END {print "Average:", sum/count, "seconds"}'
```

#### 并发处理测试
```bash
# 并发继电器控制
for i in {1..10}; do
  curl -X POST http://192.168.1.100/api/relay/control \
    -H "Content-Type: application/json" \
    -d '{"relay": 1, "state": true}' &
done
wait
```

### 稳定性测试

#### 长时间运行测试
- **24小时连续运行**: 监控内存泄漏和系统稳定性
- **1000次继电器切换**: 验证硬件耐久性
- **网络断线重连测试**: 验证自动恢复能力

#### 错误恢复测试
- **WiFi断线恢复**: 断开WiFi后重新连接
- **MQTT断线重连**: 服务器重启后重连
- **配置损坏恢复**: 清空EEPROM后恢复默认配置

## ✅ 验证检查清单

### 🔧 硬件验证
- [ ] 所有继电器可独立控制
- [ ] GPIO引脚映射正确
- [ ] RS485通信正常
- [ ] 电源指示和工作指示正常
- [ ] 继电器切换无异常声音

### 🌐 网络验证  
- [ ] WiFi连接稳定
- [ ] Web界面加载正常
- [ ] 所有API端点响应正确
- [ ] MQTT发布/订阅正常
- [ ] TCP服务器可连接

### 📡 协议验证
- [ ] HTTP API功能完整
- [ ] MQTT主题结构正确
- [ ] Modbus RTU通信正常
- [ ] Modbus TCP功能正确
- [ ] 原始TCP命令响应

### 🔐 安全验证
- [ ] Web认证功能正常
- [ ] OTA更新需要认证
- [ ] 输入参数验证有效
- [ ] 默认密码已更改
- [ ] 无明文密码存储

### ⚡ 性能验证
- [ ] 内存使用在安全范围
- [ ] 响应时间在可接受范围
- [ ] 并发处理能力符合预期
- [ ] 长期运行稳定
- [ ] 错误恢复机制有效

## 🐛 故障排除指南

### 常见问题诊断

#### Web界面无法访问
```bash
# 检查设备IP地址
nmap -sn 192.168.1.0/24

# 检查端口开放状态
nmap -p 80 192.168.1.100

# 查看串口调试信息
WEB: Server started on port 80
```

#### MQTT连接失败
```bash
# 检查MQTT服务器连接
mosquitto_pub -h 192.168.1.100 -p 1883 -t "test" -m "hello"

# 查看串口调试信息  
MQTT: Connected as RelayCtrl_001
MQTT: Sub relay/RelayCtrl_001/control
```

#### Modbus通信异常
```bash
# 检查RS485连接
# 查看串口调试信息
MODBUS: Slave=1 Func=1
RELAY: R1(JDQ0)=ON
```

#### 继电器不响应
- 检查GPIO连接
- 验证电源电压
- 查看串口调试输出
- 测量继电器线圈电阻

### 调试工具推荐

#### 硬件调试
- **万用表**: 电压电流测量
- **示波器**: 信号波形分析
- **逻辑分析仪**: 数字信号分析

#### 网络调试
- **Wireshark**: 网络包分析
- **Postman**: API接口测试
- **MQTT.fx**: MQTT客户端测试
- **ModbusPoll**: Modbus客户端测试

#### 串口调试
- **PlatformIO Serial Monitor**: 实时调试输出
- **CoolTerm**: 高级串口终端
- **Termite**: Windows串口工具

---

**文档版本**: v1.0.0  
**最后更新**: 2025年8月30日  
**适用固件**: v1.0.0+
