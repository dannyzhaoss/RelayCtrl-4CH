# RelayCtrl-4CH v2.0 🏠

**联鲸科技 ESP8266 4通道继电器控制系统**

> 基于ESP8266的智能继电器控制解决方案，支持多协议通信、现代化Web界面和安全认证

![ESP8266](https://img.shields.io/badge/ESP8266-IoT-blue)
![Arduino](https://img.shields.io/badge/Arduino-Framework-green)
![Version](https://img.shields.io/badge/Version-v2.0-orange)
![License](https://img.shields.io/badge/License-MIT-red)

## ✨ 主要特性

### 🎨 全新UI设计
- **现代化界面**: 采用渐变背景、卡片布局和响应式设计
- **中文本地化**: 完全中文界面，符合国内用户习惯
- **移动端适配**: 支持手机、平板等移动设备访问
- **标签页导航**: 控制面板、网络配置、系统信息分类管理

### � 安全认证系统
- **登录保护**: 默认用户名/密码: `admin/admin`
- **会话管理**: 1小时自动超时，支持手动注销
- **安全访问**: 所有功能需要认证后访问

### 🌐 多协议支持
- **HTTP REST API**: 标准RESTful接口
- **MQTT协议**: 支持自定义服务器、端口、主题和API密钥
- **Raw TCP**: 端口8080，支持原始TCP命令
- **Modbus TCP**: 端口502，标准Modbus协议

### 🎛️ 智能控制
- **4路继电器**: 支持JDQ0-JDQ3独立控制
- **实时状态**: 继电器状态实时显示和更新
- **批量操作**: 支持全开、全关等批量控制
- **协议切换**: 可动态启用/禁用各种通信协议

## 🚀 快速开始

### 硬件要求
- ESP8266开发板 (NodeMCU/ESP-12E推荐)
- 4路继电器模块
- 连接线若干

### 软件要求
- PlatformIO IDE
- Arduino Framework for ESP8266

### 编译上传
```bash
# 克隆仓库
git clone https://github.com/dannyzhaoss/RelayCtrl-4CH.git
cd RelayCtrl-4CH

# 编译固件
platformio run

# 上传到设备
platformio run -t upload --upload-port COM_PORT
```

### 首次配置
1. 设备启动后创建WiFi热点 `RelayCtrl-XXXX`
2. 连接热点，密码: `12345678`
3. 访问 `192.168.4.1` 配置WiFi
4. 配置完成后设备自动连接到指定WiFi

## 📱 使用指南

### 访问控制面板
1. 在浏览器中访问设备IP地址
2. 使用账号密码登录: `admin/admin`
3. 进入主控制界面

### 控制面板功能
- **🎛️ 控制面板**: 继电器开关控制和状态显示
- **🌐 网络配置**: WiFi和MQTT协议参数设置
- **⚙️ 系统信息**: 设备状态、内存使用、运行时间等

### MQTT配置
1. 在网络配置页面设置MQTT参数:
   - HTTP服务器地址: `192.168.0.145`
   - 端口: `80`
   - API路径: `/api/device`
   - API密钥: `your-api-key`
2. 点击"更新MQTT配置"保存设置
3. 在控制面板启用MQTT协议
  - TXD2/RXD2: Modbus RTU串口 (9600 baud)
- **GPIO引脚映射**:
  - JDQ0 (继电器1): GPIO12 (D6)
  - JDQ1 (继电器2): GPIO13 (D7)
  - JDQ2 (继电器3): GPIO14 (D5)
  - JDQ3 (继电器4): GPIO16 (D0)

### 网络协议支持
- **HTTP REST API**: Web界面和API控制
- **TCP原始协议**: 端口8080，支持命令行控制
- **Modbus TCP**: 端口502，标准Modbus TCP协议
- **Modbus RTU**: 串口协议，从站ID=1
- **MQTT**: 发布/订阅消息控制
- **OTA更新**: 无线固件更新

### WiFi配置
- **默认WiFi**: SSKJ-4 / xszn486020zcs
- **AP配网模式**: ESP8266-RelayCtrl / 12345678
- **WiFiManager**: 自动配网支持

## 📡 通信协议详解

### 1. HTTP REST API

#### 基础端点
- `GET /` - Web控制界面
- `GET /api/status` - 获取系统状态
- `POST /api/relay` - 控制继电器
- `GET /api/config` - 获取配置
- `POST /api/config` - 设置配置
- `POST /api/restart` - 重启设备
- `/update` - OTA更新页面

#### 继电器控制示例
```bash
# 开启继电器1
curl -X POST http://192.168.1.100/api/relay \
  -H "Content-Type: application/json" \
  -d '{"relay":1,"state":true}'

# 关闭继电器2
curl -X POST http://192.168.1.100/api/relay \
  -H "Content-Type: application/json" \
  -d '{"relay":2,"state":false}'
```

### 2. MQTT协议

#### 主题结构
```
relay/{device_id}/control      - 控制主题 (订阅)
relay/{device_id}/relay{1-4}/state - 继电器状态 (发布)
relay/{device_id}/status       - 系统状态 (发布)
relay/{device_id}/heartbeat    - 心跳信息 (发布)
relay/{device_id}/online       - 在线状态 (发布)
```

#### 控制消息格式
```json
// 控制单个继电器
{
  "relay": 1,
  "state": true
}

// 系统命令
{
  "command": "status"        // 获取状态
}
{
  "command": "restart"       // 重启设备
}
{
  "command": "all_on"        // 全部开启
}
{
  "command": "all_off"       // 全部关闭
}
```

### 3. TCP原始协议 (端口8080)

支持telnet连接，提供命令行界面：

```bash
# 连接到设备
telnet 192.168.1.100 8080

# 可用命令
relay <1-4> <on/off>  # 控制继电器
status                # 显示状态
help                  # 显示帮助
quit/exit            # 断开连接
```

### 4. Modbus TCP (端口502)

#### 寄存器映射
| 地址 | 类型 | 描述 |
|------|------|------|
| 0-3 | 线圈 | 继电器1-4状态 |
| 0-3 | 保持寄存器 | 继电器1-4状态 |
| 4 | 保持寄存器 | 设备状态 (WiFi+MQTT) |
| 5 | 保持寄存器 | WiFi信号强度 |
| 6-7 | 保持寄存器 | 运行时间(32位) |

#### 支持的功能码
- 0x01: 读线圈 (Read Coils)
- 0x03: 读保持寄存器 (Read Holding Registers)
- 0x05: 写单个线圈 (Write Single Coil)
- 0x06: 写单个寄存器 (Write Single Register)
- 0x0F: 写多个线圈 (Write Multiple Coils)

### 5. Modbus RTU (串口)

#### 配置
- **波特率**: 9600
- **数据位**: 8
- **停止位**: 1
- **校验位**: 无
- **从站ID**: 1

#### 寄存器映射
与Modbus TCP相同

## 🔧 串口调试命令

通过调试串口(115200 baud)可以配置和控制设备：

```bash
# 继电器控制
relay 1 on          # 开启继电器1
relay 2 off         # 关闭继电器2

# 配置查看和设置
config              # 显示当前配置
config set ssid MyWiFi              # 设置WiFi SSID
config set password MyPassword      # 设置WiFi密码
config set mqtt_server 192.168.1.50 # 设置MQTT服务器
config set mqtt_port 1883           # 设置MQTT端口
config set device_id ESP8266_002    # 设置设备ID

# 系统命令
status              # 显示系统状态
restart             # 重启设备
help                # 显示帮助信息
```

## 📊 配置存储

配置数据存储在EEPROM中，包括：
- WiFi SSID和密码
- MQTT服务器地址和端口
- 设备ID
- 其他系统参数

## 🔌 硬件连接

```
ESP8266 NodeMCU    →    继电器模块
D6 (GPIO12)       →    JDQ0 (继电器1)
D7 (GPIO13)       →    JDQ1 (继电器2)
D5 (GPIO14)       →    JDQ2 (继电器3)
D0 (GPIO16)       →    JDQ3 (继电器4)

D1 (GPIO5)        →    RS485 TX 
D2 (GPIO4)        →    RS485 RX
注: RS485模块使用自动方向控制，无需DE/RE管脚

3.3V              →    继电器模块 VCC
GND               →    继电器模块 GND
```

## 🚀 快速开始

### 1. 硬件准备
- ESP8266开发板 (NodeMCU/Wemos D1等)
- 4路继电器模块
- 连接线

### 2. 编译上传
```bash
# 使用PlatformIO
pio run -t upload

# 或使用Arduino IDE
# 选择板子: NodeMCU 1.0 (ESP-12E Module)
# 上传速度: 921600
```

### 3. 首次配置
1. 设备启动后会尝试连接默认WiFi
2. 如果连接失败，会启动AP模式
3. 连接到"ESP8266-RelayCtrl"热点
4. 在浏览器中访问192.168.4.1进行配网

### 4. 访问控制
- Web界面: http://设备IP地址
- OTA更新: http://设备IP地址/update

## 📱 控制示例

### Python控制脚本
```python
import requests
import json

# 设备IP
DEVICE_IP = "192.168.1.100"

def control_relay(relay, state):
    url = f"http://{DEVICE_IP}/api/relay"
    data = {"relay": relay, "state": state}
    response = requests.post(url, json=data)
    return response.json()

# 开启继电器1
control_relay(1, True)

# 关闭继电器2
control_relay(2, False)
```

### Node.js MQTT控制
```javascript
const mqtt = require('mqtt');
const client = mqtt.connect('mqtt://192.168.1.100');

// 控制继电器
function controlRelay(relay, state) {
    const topic = 'relay/ESP8266_001/control';
    const message = JSON.stringify({relay: relay, state: state});
    client.publish(topic, message);
}

// 开启继电器1
controlRelay(1, true);
```

## 🔧 技术参数

- **MCU**: ESP8266
- **WiFi**: 802.11 b/g/n
- **工作电压**: 3.3V
- **继电器**: 低电平触发
- **最大负载**: 根据继电器规格
- **通信协议**: HTTP, TCP, MQTT, Modbus RTU/TCP
- **OTA支持**: ✅
- **Web界面**: ✅
- **配置存储**: EEPROM

## 📄 许可证

MIT License

## 🤝 贡献

欢迎提交Issue和Pull Request！

## 📞 支持

如有问题，请通过以下方式联系：
- 创建GitHub Issue
- 发送邮件到项目维护者

---

**注意**: 请确保继电器连接正确，避免短路或过载。使用前请仔细检查硬件连接。
