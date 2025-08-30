# 🚀 RelayCtrl-4CH 部署运维手册

## 📋 部署概览

本手册提供ESP8266 RelayCtrl-4CH四路继电器控制器的完整部署、配置、运维和故障排除指南。

## 🔧 环境准备

### 开发环境要求

**软件环境**
- **PlatformIO Core**: v6.1.18+
- **Python**: 3.6+ (PlatformIO依赖)
- **VSCode**: 推荐使用PlatformIO IDE扩展
- **Git**: 代码版本管理

**硬件环境**
- **ESP8266 NodeMCU**: 开发板
- **4路继电器模块**: JDQ0-JDQ3
- **RS485模块**: TTL转RS485
- **USB数据线**: 程序下载
- **杜邦线**: 硬件连接

### 硬件连接图

```
ESP8266 NodeMCU    →    继电器模块
D6 (GPIO12)       →    JDQ0 (继电器1)
D7 (GPIO13)       →    JDQ1 (继电器2)
D5 (GPIO14)       →    JDQ2 (继电器3)
D0 (GPIO16)       →    JDQ3 (继电器4)

ESP8266 NodeMCU    →    RS485模块
D1 (GPIO5)        →    RS485 TX
D2 (GPIO4)        →    RS485 RX
D4 (GPIO2)        →    RS485 DE/RE

电源连接
5V/3.3V           →    继电器模块VCC
GND               →    继电器模块GND
```

## 📦 固件编译与烧录

### 编译环境配置

1. **克隆项目代码**
```bash
git clone https://github.com/dannyzhaoss/RelayCtrl-4CH.git
cd RelayCtrl-4CH
```

2. **安装PlatformIO依赖**
```bash
# 通过Python安装
pip install platformio

# 或通过VSCode安装PlatformIO扩展
# Extensions -> PlatformIO IDE
```

3. **编译固件**
```bash
# 进入项目目录
cd c:\CODE\RelayCtrl-4CH2

# 编译固件
python -m platformio run

# 编译结果
RAM:   58.4% (47,864 / 81,920 字节)
Flash: 52.4% (546,975 / 1,044,464 字节)
```

### 固件烧录

1. **连接设备**
   - 使用USB数据线连接NodeMCU到电脑
   - 确认COM端口号（如COM14）

2. **烧录固件**
```bash
# 烧录到指定端口
python -m platformio run -t upload --upload-port COM14

# 烧录过程
Connecting....
Chip is ESP8266EX
Writing at 0x00000000... (100%)
Hard resetting via RTS pin...
```

3. **监控串口输出**
```bash
# 监控调试信息
python -m platformio device monitor --port COM14 --baud 115200

# 预期输出
=== RelayCtrl 4CH Relay Controller ===
FW: 1.0.0 | Device: JDQ0-3 Relay Control
SYS: Init...
WIFI: Connecting...
WEB: Server started on port 80
```

## 🌐 网络配置

### WiFi配网方式

**方式1: 默认WiFi连接**
- 固件内置默认WiFi配置
- SSID: `SSKJ-4`
- 密码: `xszn486020zcs`

**方式2: WiFiManager配网**
- 当默认WiFi连接失败时自动启用
- 创建AP热点: `RelayCtrl-XXXX`
- 密码: `12345678`
- 配网地址: `http://192.168.4.1`

### 网络服务端口

| 服务 | 端口 | 协议 | 描述 |
|------|------|------|------|
| Web服务器 | 80 | HTTP | Web界面和API |
| MQTT客户端 | 1883 | TCP | MQTT通信 |
| Modbus TCP | 502 | TCP | Modbus TCP协议 |
| 原始TCP | 8080 | TCP | 简单TCP命令 |
| OTA更新 | 80 | HTTP | 固件在线更新 |

### IP地址获取

```bash
# 方法1: 路由器管理界面查看
# 登录路由器 -> DHCP客户端列表 -> 查找RelayCtrl设备

# 方法2: 网络扫描
nmap -sn 192.168.1.0/24

# 方法3: 串口监控
# 设备启动时会打印IP地址
WIFI: Connected to SSKJ-4
IP: 192.168.1.100
```

## ⚙️ 系统配置

### Web界面配置

1. **访问配置页面**
   - 浏览器访问: `http://设备IP地址/config`
   - 默认认证: 用户名`admin` 密码`admin`

2. **基础设置**
   - **设备ID**: RelayCtrl_001 (可自定义)
   - **Web认证**: 建议启用并更改默认密码

3. **网络设置**
   - **WiFi SSID**: 目标网络名称
   - **WiFi密码**: 网络密码

4. **MQTT设置**
   - **服务器地址**: MQTT代理服务器IP
   - **端口**: 默认1883
   - **主题前缀**: relay/
   - **用户名/密码**: 可选认证

5. **Modbus设置**
   - **从站地址**: 1-247
   - **波特率**: 9600/19200/38400/115200
   - **端口配置**: TCP端口502，原始TCP端口8080

### 配置文件管理

**配置存储位置**
- 存储介质: EEPROM (512字节)
- 配置结构: 二进制格式
- 有效性标志: 0xAA

**配置备份与恢复**
```bash
# 通过API获取配置
curl http://192.168.1.100/api/config/get

# 通过API设置配置
curl -X POST http://192.168.1.100/api/config/set \
  -H "Content-Type: application/json" \
  -d @config.json
```

## 🔄 服务管理

### 协议服务控制

**通过Web界面**
- 访问 `http://设备IP/config`
- 在协议控制区域启用/禁用服务
- MQTT客户端、TCP服务器、Modbus TCP可独立控制

**通过API控制**
```bash
# 启用MQTT
curl -X POST http://192.168.1.100/api/protocol/control \
  -H "Content-Type: application/json" \
  -d '{"protocol": "mqtt", "enabled": true}'

# 禁用TCP服务器
curl -X POST http://192.168.1.100/api/protocol/control \
  -H "Content-Type: application/json" \
  -d '{"protocol": "tcp", "enabled": false}'
```

### 系统维护操作

**重启系统**
```bash
# Web界面重启
curl -X POST http://192.168.1.100/restart

# MQTT命令重启
mosquitto_pub -h 192.168.1.100 -t "relay/设备ID/control" \
  -m '{"command": "restart"}'
```

**OTA固件更新**
1. 浏览器访问: `http://设备IP/update`
2. 使用Web认证登录
3. 选择固件文件上传
4. 等待更新完成自动重启

## 📊 监控与诊断

### 系统状态监控

**Web界面监控**
- 访问主页: `http://设备IP/`
- 实时显示继电器状态
- 系统信息: IP、运行时间、内存使用

**API状态查询**
```bash
curl http://192.168.1.100/api/status

# 返回JSON格式系统状态
{
  "success": true,
  "data": {
    "deviceId": "RelayCtrl_001",
    "ip": "192.168.1.100",
    "wifi": "SSKJ-4",
    "rssi": -45,
    "uptime": 3600,
    "freeHeap": 25000,
    "relays": [...]
  }
}
```

**MQTT状态监控**
```bash
# 订阅状态主题
mosquitto_sub -h 192.168.1.100 -t "relay/+/status"
mosquitto_sub -h 192.168.1.100 -t "relay/+/online"  
mosquitto_sub -h 192.168.1.100 -t "relay/+/heartbeat"
```

### 日志分析

**串口调试日志**
```bash
# 实时监控串口输出
python -m platformio device monitor --port COM14 --baud 115200

# 典型日志格式
SYS: Init complete
WIFI: Connected to SSKJ-4
MQTT: Connected as RelayCtrl_001
RELAY: R1=ON
MODBUS: Slave=1 Func=5
```

**日志级别说明**
- `SYS:` - 系统初始化和状态
- `WIFI:` - WiFi连接信息
- `MQTT:` - MQTT通信日志
- `RELAY:` - 继电器控制日志
- `MODBUS:` - Modbus通信日志
- `TCP:` - TCP服务器日志
- `WEB:` - Web服务器日志
- `CONFIG:` - 配置管理日志

## 🛠️ 故障排除

### 常见问题解决

**问题1: 设备无法连接WiFi**

*症状*: 设备创建AP热点，无法连接目标WiFi

*解决方案*:
1. 检查WiFi名称和密码正确性
2. 确认WiFi信号强度（RSSI > -70dBm）
3. 检查路由器MAC地址过滤设置
4. 使用WiFiManager重新配置

*诊断命令*:
```bash
# 查看串口WiFi连接日志
WIFI: Scanning networks...
WIFI: Found 5 networks
WIFI: Connecting to SSKJ-4...
WIFI: Connection failed, starting AP mode
```

**问题2: Web界面无法访问**

*症状*: 浏览器无法打开设备IP地址

*解决方案*:
1. 确认设备IP地址正确
2. 检查防火墙设置
3. 验证设备和电脑在同一网段
4. 检查80端口是否被占用

*诊断命令*:
```bash
# 测试网络连通性
ping 192.168.1.100

# 检查端口开放状态  
nmap -p 80 192.168.1.100

# 查看串口Web服务器日志
WEB: Server started on port 80
```

**问题3: 继电器不响应**

*症状*: 发送控制命令后继电器无动作

*解决方案*:
1. 检查硬件连接线路
2. 验证继电器模块电源
3. 测量GPIO输出电压
4. 检查继电器模块规格匹配

*诊断命令*:
```bash
# 查看继电器控制日志
RELAY: R1=ON
RELAY: R1=OFF

# 通过API测试控制
curl -X POST http://192.168.1.100/api/relay/control \
  -d '{"relay": 1, "state": true}'
```

**问题4: MQTT连接失败**

*症状*: MQTT客户端无法连接到代理服务器

*解决方案*:
1. 验证MQTT服务器地址和端口
2. 检查MQTT服务器是否启动
3. 确认用户名密码正确（如果启用认证）
4. 检查网络防火墙设置

*诊断命令*:
```bash
# 测试MQTT服务器连通性
mosquitto_pub -h 192.168.1.100 -p 1883 -t "test" -m "hello"

# 查看MQTT连接日志
MQTT: Connecting to 192.168.1.100:1883
MQTT: Connected as RelayCtrl_001
MQTT: Connection failed (rc=-2)
```

### 恢复操作

**配置重置**
1. 断电重启设备3次（间隔5秒）
2. 设备将自动重置为出厂配置
3. 或通过串口发送配置重置命令

**固件重刷**
```bash
# 重新烧录固件（会清除配置）
python -m platformio run -t upload --upload-port COM14

# 擦除Flash后重新烧录  
python -m platformio run -t erase
python -m platformio run -t upload --upload-port COM14
```

## 📈 性能优化

### 内存优化建议

**当前内存使用**: RAM 58.4%, Flash 52.4%

**优化措施**:
- 限制同时连接的TCP客户端数量（当前最大2个）
- 减少MQTT消息发送频率（心跳30秒）
- 使用静态缓冲区避免内存碎片
- 定期重启清理内存（可选）

### 网络性能优化

**WiFi信号优化**:
- 确保RSSI > -70dBm
- 避免2.4GHz频段干扰
- 使用固定IP避免DHCP延迟

**协议性能优化**:
- MQTT QoS级别设置为0（最快）
- TCP连接超时设置为50ms
- Modbus响应超时50ms

## 🔐 安全加固

### 基础安全措施

1. **更改默认密码**
   - Web认证密码: admin/admin → 强密码
   - WiFi AP密码: 12345678 → 强密码

2. **网络隔离**
   - 将设备部署在IoT专用VLAN
   - 限制跨网段访问
   - 配置防火墙规则

3. **固件更新**
   - 定期检查固件更新
   - 使用HTTPS进行OTA更新（如果支持）
   - 验证固件签名（高级功能）

### 访问控制

**Web接口访问控制**:
- 启用HTTP Basic认证
- 配置强密码策略
- 记录访问日志

**网络访问控制**:
- 限制管理端口访问源IP
- 使用VPN进行远程管理
- 配置端口扫描检测

## 📝 维护计划

### 日常维护

**每日检查**:
- 设备在线状态
- 继电器响应正常
- 无异常重启

**每周检查**:
- 内存使用情况
- 网络连接稳定性
- 日志异常记录

**每月维护**:
- 固件版本检查
- 配置备份
- 性能基准测试

### 定期维护

**季度维护**:
- 硬件连接检查
- 继电器接点清洁
- 散热检查

**年度维护**:
- 硬件更换（如需要）
- 系统安全审计
- 性能优化评估

---

**文档版本**: v1.0.0  
**最后更新**: 2025年8月30日  
**适用固件**: v1.0.0+
