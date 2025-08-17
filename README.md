# ESP8266 RelayCtrl-4CH 四路继电器控制器

![Version](https://img.shields.io/badge/version-v1.0.1-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP8266-green.svg)
![Framework](https://img.shields.io/badge/framework-Arduino-orange.svg)
![License](https://img.shields.io/badge/license-MIT-yellow.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)
![Modbus](https://img.shields.io/badge/Modbus-RTU%2FTCP-blue.svg)

基于ESP8266的专业级四路继电器控制系统，支持HTTP、TCP、MQTT、Modbus RTU/TCP等多种工业通信协议。

## 🚀 项目概述

RelayCtrl-4CH是一个功能完整的工业级继电器控制解决方案，采用ESP8266 NodeMCU开发板，提供稳定可靠的远程继电器控制功能。

### 📊 系统性能指标 (v1.0.1)
- **内存使用**: 63.7% (52188/81920 bytes)
- **Flash使用**: 52.7% (550223/1044464 bytes)  
- **编译状态**: ✅ 成功构建
- **运行稳定性**: ✅ 24/7连续运行
- **通信协议**: ✅ 6种协议全部正常

## ✨ 核心特性

### 🔌 硬件控制
- **4路独立继电器**: GPIO12/13/14/16精确控制
- **RS485通信**: GPIO4/5/2专业工业通信
- **状态指示**: 实时继电器状态反馈
- **电气隔离**: 安全可靠的继电器驱动

### 🌐 网络通信
- **HTTP API**: RESTful接口，JSON数据格式
- **TCP原始协议**: 自定义端口，高速数据传输  
- **MQTT协议**: 物联网标准，支持订阅/发布
- **Modbus RTU**: 串口工业协议，支持动态从机ID配置
- **Modbus TCP**: 网络工业协议，支持Unit ID验证
- **调试监控**: 30秒英文心跳输出，实时系统状态

### 🖥️ Web管理
- **响应式界面**: 支持手机、平板、电脑访问
- **统一配置管理**: 优化的设置界面，避免重复配置
- **实时监控**: 继电器状态实时显示
- **集成升级**: 在线固件升级，支持进度显示
- **安全认证**: Web访问权限控制
- **服务管理**: 网络服务统一配置入口

### ⚙️ 智能配置
- **WiFiManager配网**: 自动WiFi配置向导
- **动态端口**: 运行时配置服务端口
- **服务控制**: 按需启用/禁用网络服务
- **参数保存**: EEPROM持久化存储

## 🔧 硬件连接

### ESP8266 NodeMCU 引脚映射
```
继电器控制:
├── D6 (GPIO12) → JDQ0 (继电器1)
├── D7 (GPIO13) → JDQ1 (继电器2)
├── D5 (GPIO14) → JDQ2 (继电器3)
└── D0 (GPIO16) → JDQ3 (继电器4)

RS485通信:
├── D1 (GPIO5)  → RS485 TX
├── D2 (GPIO4)  → RS485 RX
└── D4 (GPIO2)  → RS485 DE/RE
```

### 供电要求
- **ESP8266**: 3.3V/500mA (USB供电)
- **继电器模块**: 5V/2A (外部供电推荐)
- **RS485模块**: 3.3V/50mA

## 📦 快速开始

### 环境准备
1. **安装PlatformIO**:
   ```bash
   pip install platformio
   ```

2. **克隆项目**:
   ```bash
   git clone https://github.com/dannyzhaoss/RelayCtrl-4CH.git
   cd RelayCtrl-4CH
   ```

3. **编译项目**:
   ```bash
   pio run
   ```

4. **上传固件** (替换COM13为实际端口):
   ```bash
   pio run -t upload --upload-port COM13
   ```

### 首次配置
1. **设备启动**: 上电后设备会尝试连接默认WiFi
2. **配网模式**: 如无法连接，设备会启动AP模式
3. **连接设备**: 手机连接`ESP8266-RelayCtrl`热点 (密码:12345678)
4. **配置WiFi**: 在弹出页面选择WiFi网络并输入密码
5. **访问界面**: 配网成功后通过设备IP访问Web界面

## 🌐 网络协议详解

### 1. HTTP REST API

#### 基础端点
- `GET /` - Web控制界面
- `GET /config` - 系统配置页面
- `GET /api/status` - 获取系统状态
- `POST /api/relay` - 控制继电器
- `POST /api/restart` - 重启设备

#### 继电器控制示例
```bash
# 开启继电器1 (需要认证)
curl -X POST http://192.168.1.100/api/relay \
  -H "Content-Type: application/json" \
  -d '{"relay":0,"state":true}' \
  -u admin:admin

# 获取所有继电器状态
curl http://192.168.1.100/api/status -u admin:admin
```

#### 响应格式
```json
{
  "status": "success",
  "data": {
    "relays": [false, false, false, false],
    "wifi": {
      "ssid": "YourWiFi",
      "ip": "192.168.1.100",
      "rssi": -45
    },
    "memory": {
      "free": 31352,
      "heap": 31352
    },
    "uptime": 86400
  }
}
```

### 2. TCP原始协议

#### 连接信息
- **端口**: 8888 (可配置)
- **协议**: 原始TCP，文本命令
- **分隔符**: `\n` (换行符)

#### 命令格式
```bash
# 控制继电器 (relay_id: 0-3, state: 0/1)
RELAY <relay_id> <state>

# 查询状态
STATUS

# 查询特定继电器
RELAY <relay_id>
```

#### 使用示例
```bash
# 使用telnet连接
telnet 192.168.1.100 8888

# 发送命令
RELAY 0 1    # 开启继电器1
RELAY 0 0    # 关闭继电器1
STATUS       # 查询所有状态
```

### 3. MQTT协议

#### 连接参数
- **Broker**: 可配置MQTT服务器
- **端口**: 1883 (默认)
- **ClientID**: ESP8266_Relay_{MAC}
- **用户认证**: 支持用户名/密码

#### 主题结构
```bash
# 控制主题 (订阅)
relay/control/0    # 控制继电器1 (payload: 1/0)
relay/control/1    # 控制继电器2
relay/control/2    # 控制继电器3
relay/control/3    # 控制继电器4
relay/control/all  # 控制所有继电器 (payload: 1111/0000)

# 状态主题 (发布)
relay/status/0     # 继电器1状态
relay/status/1     # 继电器2状态
relay/status/2     # 继电器3状态
relay/status/3     # 继电器4状态
relay/status/all   # 所有继电器状态

# 系统主题
relay/system/online    # 设备在线状态
relay/system/info      # 设备信息
```

#### 使用示例
```bash
# 使用mosquitto客户端
# 控制继电器1开启
mosquitto_pub -h 192.168.1.100 -t "relay/control/0" -m "1"

# 订阅状态更新
mosquitto_sub -h 192.168.1.100 -t "relay/status/+"
```

### 4. Modbus协议

#### Modbus RTU (串口)
- **端口**: GPIO4/5 (软件串口)
- **波特率**: 9600
- **数据位**: 8
- **停止位**: 1
- **校验位**: None
- **从站ID**: 1

#### Modbus TCP (网络)
- **端口**: 502 (可配置)
- **从站ID**: 1
- **功能码支持**: 01, 05, 15

#### 寄存器映射
```
线圈地址 (Coil):
├── 0 → 继电器1状态 (JDQ0)
├── 1 → 继电器2状态 (JDQ1)
├── 2 → 继电器3状态 (JDQ2)
└── 3 → 继电器4状态 (JDQ3)
```

#### 使用示例
```bash
# 使用modpoll工具 (Modbus TCP)
# 读取所有继电器状态
modpoll -m tcp -t 0 -r 0 -c 4 192.168.1.100

# 开启继电器1
modpoll -m tcp -t 0 -r 0 -c 1 192.168.1.100 1

# 使用串口工具 (Modbus RTU)
# 读取继电器状态: 01 01 00 00 00 04 [CRC]
# 写入继电器1:   01 05 00 00 FF 00 [CRC]
```

## ⚙️ 系统配置

### 默认配置
```
Web认证:
├── 状态: 启用
├── 用户名: admin
└── 密码: admin

网络服务:
├── HTTP服务: 启用 (端口80)
├── MQTT服务: 禁用
├── Modbus TCP: 禁用 (端口502)
└── Raw TCP: 禁用 (端口8888)

WiFi配置:
├── 默认SSID: SSKJ-4
├── 默认密码: xszn486020zcs
├── AP模式SSID: ESP8266-RelayCtrl
└── AP模式密码: 12345678
```

### 运行时配置
访问 `http://设备IP/config` 进行在线配置：

1. **WiFi设置**: 网络连接参数
2. **MQTT设置**: MQTT服务器和认证
3. **认证设置**: Web访问用户名密码
4. **服务设置**: 各网络服务启用状态和端口配置

### 固件升级
1. **访问配置页面**: `http://设备IP/config`
2. **找到固件升级卡片**
3. **选择固件文件**: 
   - **Firmware文件**: 包含程序代码，通常升级此文件
   - **FileSystem文件**: 包含网页资源，特殊情况才需要
4. **开始升级**: 点击上传，等待进度完成
5. **自动重启**: 升级完成后设备自动重启

## 📊 性能指标

### 资源使用
- **RAM使用**: 61.7% (50,568 / 81,920 bytes)
- **Flash使用**: 52.6% (549,167 / 1,044,464 bytes)
- **编译大小**: 549KB
- **空闲内存**: ~31KB

### 网络性能
- **HTTP响应时间**: < 100ms
- **TCP并发连接**: 4个
- **MQTT心跳间隔**: 30秒
- **Modbus响应时间**: < 50ms

### 稳定性指标
- **WiFi重连**: 自动重连机制
- **内存泄漏**: 无已知内存泄漏
- **长期运行**: 支持7x24小时运行
- **异常恢复**: 看门狗自动重启

## 🔒 安全特性

### 访问控制
- **Web认证**: HTTP基础认证，默认启用
- **服务隔离**: 网络服务按需启用
- **端口配置**: 自定义端口降低风险
- **固件保护**: 只有认证用户可升级

### 网络安全
- **默认关闭**: 所有网络服务默认禁用
- **最小权限**: 仅开放必要的网络端口
- **认证机制**: MQTT支持用户名密码认证
- **访问日志**: 串口输出访问记录

## 🛠️ 开发指南

### 项目结构
```
RelayCtrl-4CH/
├── src/                    # 源代码目录
│   ├── main.cpp           # 主程序入口
│   ├── config.h           # 配置定义
│   ├── relay_controller.h # 头文件声明
│   ├── web_handlers.cpp   # Web服务器处理
│   ├── mqtt_handlers.cpp  # MQTT协议处理
│   ├── modbus_serial.cpp  # Modbus RTU/串口
│   └── tcp_server.cpp     # TCP服务器
├── platformio.ini         # PlatformIO配置
├── README.md             # 项目说明
├── PROJECT_STATUS.md     # 项目状态报告
├── UPGRADE_NOTES.md      # 升级说明
└── RS485_Commands.md     # RS485命令参考
```

### 依赖库
```ini
[lib_deps]
ESP8266WiFi               # WiFi连接
ESP8266WebServer          # HTTP服务器
ESP8266HTTPUpdateServer   # OTA升级
ESP8266mDNS              # mDNS服务
WiFiManager              # WiFi配网
ArduinoJson              # JSON处理
PubSubClient             # MQTT客户端
ModbusMaster             # Modbus协议
EspSoftwareSerial        # 软件串口
```

### 编译环境
```ini
[env:esp8266]
platform = espressif8266
board = esp12e
framework = arduino
monitor_speed = 115200
upload_speed = 921600
```

### 自定义开发
1. **修改引脚映射**: 编辑 `config.h` 中的GPIO定义
2. **添加新协议**: 在对应的处理文件中添加代码
3. **修改Web界面**: 编辑 `web_handlers.cpp` 中的HTML
4. **调整默认配置**: 修改 `config.h` 中的默认值

## 🐛 故障排除

### 常见问题

#### 1. 设备无法连接WiFi
- **检查WiFi信号**: 确保信号强度足够
- **验证密码**: 确认WiFi密码正确
- **重置配置**: 长按复位键进入配网模式
- **串口调试**: 查看连接日志信息

#### 2. Web界面无法访问
- **确认IP地址**: 通过路由器查看设备IP
- **检查认证**: 使用正确的用户名密码 (admin/admin)
- **防火墙设置**: 确保端口80未被阻挡
- **浏览器缓存**: 清除浏览器缓存重试

#### 3. MQTT连接失败
- **服务启用**: 确认MQTT服务已在配置中启用
- **网络连通**: ping测试MQTT服务器连通性
- **认证信息**: 检查MQTT用户名密码设置
- **端口配置**: 确认MQTT端口(1883)未被阻挡

#### 4. Modbus通信异常
- **接线检查**: 确认RS485接线正确 (A+, B-, GND)
- **波特率匹配**: 确保主从设备波特率一致 (9600)
- **地址设置**: 确认从站地址设置为1
- **电气隔离**: 检查RS485模块供电和信号

#### 5. 内存不足
- **减少服务**: 关闭不需要的网络服务
- **监控内存**: 通过API查看内存使用情况
- **重启设备**: 定期重启释放内存碎片
- **固件升级**: 升级到最新优化版本

### 调试方法

#### 串口监控
```bash
# 使用PlatformIO监控串口
pio device monitor

# 或使用指定端口
pio device monitor --port COM13
```

#### 网络工具测试
```bash
# HTTP API测试
curl -v http://192.168.1.100/api/status -u admin:admin

# TCP连接测试
telnet 192.168.1.100 8888

# MQTT测试
mosquitto_pub -h 192.168.1.100 -t "relay/control/0" -m "1" -d

# Modbus TCP测试
modpoll -m tcp -t 0 -r 0 -c 4 192.168.1.100
```

#### 日志分析
设备会在串口输出详细的运行日志，包括：
- WiFi连接状态
- 网络服务启动信息
- 客户端连接记录
- 协议处理过程
- 错误和异常信息

## 📚 相关文档

- [项目状态报告](PROJECT_STATUS.md) - 完整的项目交付状态
- [升级说明](UPGRADE_NOTES.md) - 版本升级和功能更新
- [RS485命令参考](RS485_Commands.md) - 串口命令详细说明

## 📄 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 🤝 贡献

欢迎提交 Issue 和 Pull Request 来改进这个项目。

## 📞 联系我们

- **项目地址**: https://github.com/dannyzhaoss/RelayCtrl-4CH
- **问题反馈**: GitHub Issues
- **版本**: v1.0.0 正式版

---

**RelayCtrl-4CH** - 专业级ESP8266继电器控制解决方案 🚀
