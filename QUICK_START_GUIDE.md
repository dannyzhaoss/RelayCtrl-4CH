# 🚀 RelayCtrl-4CH 快速部署指南

## 📦 项目准备

### 环境检查清单

```bash
# 检查PlatformIO环境
platformio --version

# 检查Python环境  
python --version

# 检查串口设备
platformio device list
```

### 硬件准备清单

- [x] ESP8266 NodeMCU开发板
- [x] 4路继电器模块
- [x] RS485转换器(可选)
- [x] 连接线若干
- [x] USB数据线

## ⚡ 5分钟快速部署

### 步骤1: 下载代码

```bash
# 克隆项目
git clone <项目地址>
cd RelayCtrl-4CH2

# 或者直接下载ZIP解压
```

### 步骤2: 连接硬件

```text
ESP8266 引脚     →    继电器模块
D6 (GPIO12)     →    JDQ0 继电器1
D7 (GPIO13)     →    JDQ1 继电器2  
D5 (GPIO14)     →    JDQ2 继电器3
D0 (GPIO16)     →    JDQ3 继电器4

可选RS485连接:
D1 (GPIO5)      →    RS485 TX
D2 (GPIO4)      →    RS485 RX
D4 (GPIO2)      →    RS485 DE/RE
```

### 步骤3: 编译上传

```bash
# 进入项目目录
cd RelayCtrl-4CH2

# 编译固件
platformio run

# 上传固件 (确保ESP8266已连接)
platformio run -t upload

# 监控串口输出
platformio device monitor
```

### 步骤4: 首次配置

#### 方式1: WiFiManager配网

1. 设备启动后，连接WiFi热点: `RelayCtrl-Setup`
2. 浏览器访问: `192.168.4.1`
3. 选择WiFi网络，输入密码
4. 点击"Save"保存配置

#### 方式2: 默认WiFi配置

修改 `src/config.h` 文件:

```cpp
// 默认WiFi配置
#define DEFAULT_SSID "Your_WiFi_Name"
#define DEFAULT_PASSWORD "Your_WiFi_Password"
```

重新编译上传即可。

### 步骤5: 访问控制界面

设备连接WiFi后，通过串口查看IP地址:

```text
WiFi连接成功! IP地址: 192.168.1.100
Web服务器已启动，端口: 80
```

浏览器访问: `http://192.168.1.100`

## 🔧 配置选项

### 基本配置

| 参数 | 默认值 | 说明 |
|------|--------|------|
| WiFi SSID | 空 | WiFi网络名称 |
| WiFi密码 | 空 | WiFi网络密码 |
| 设备名称 | RelayCtrl | 设备标识符 |
| HTTP端口 | 80 | Web服务端口 |

### MQTT配置

| 参数 | 默认值 | 说明 |
|------|--------|------|
| MQTT服务器 | 192.168.1.1 | MQTT代理地址 |
| MQTT端口 | 1883 | MQTT服务端口 |
| 用户名 | 空 | MQTT认证用户名 |
| 密码 | 空 | MQTT认证密码 |

### Modbus配置

| 参数 | 默认值 | 说明 |
|------|--------|------|
| 从站地址 | 1 | Modbus从站ID |
| 波特率 | 9600 | RS485通信速率 |
| TCP端口 | 502 | Modbus TCP端口 |

### 安全配置

| 参数 | 默认值 | 说明 |
|------|--------|------|
| Web用户名 | admin | HTTP认证用户名 |
| Web密码 | admin | HTTP认证密码 |
| OTA密码 | relay123 | OTA更新密码 |

## 🧪 功能测试

### Web界面测试

1. **主控制页面**: `http://设备IP/`
   - 测试继电器开关控制
   - 查看实时状态显示
   - 验证批量操作功能

2. **配置页面**: `http://设备IP/config`
   - 修改WiFi配置
   - 调整MQTT参数
   - 设置Modbus参数

### API接口测试

```bash
# 控制继电器
curl -X POST http://设备IP/api/relay/control \
  -H "Content-Type: application/json" \
  -d '{"relay": 1, "state": true}'

# 查询状态
curl http://设备IP/api/status

# 批量控制
curl -X POST http://设备IP/api/relay/control \
  -H "Content-Type: application/json" \
  -d '{"command": "all_on"}'
```

### MQTT测试

```bash
# 订阅状态主题
mosquitto_sub -h MQTT服务器IP -t "relay/RelayCtrl/+/state"

# 发送控制命令  
mosquitto_pub -h MQTT服务器IP -t "relay/RelayCtrl/control" \
  -m '{"relay": 1, "state": true}'
```

### Modbus测试

使用Modbus工具 (如ModbusPoll):

1. **Modbus RTU测试**:
   - 串口: COM端口
   - 波特率: 9600
   - 从站地址: 1
   - 读取线圈: 地址0-3

2. **Modbus TCP测试**:
   - IP地址: 设备IP
   - 端口: 502
   - 从站地址: 1
   - 读取线圈: 地址0-3

### TCP原始协议测试

```bash
# Windows系统
telnet 设备IP 8080

# 发送命令
RELAY 1 ON
STATUS
HELP
```

## 🔍 故障排除

### 常见问题及解决方案

#### 1. 编译失败

**问题**: 缺少依赖库

**解决**:

```bash
# 更新PlatformIO
pip install -U platformio

# 清理重新编译
platformio run -t clean
platformio run
```

#### 2. 上传失败

**问题**: 串口被占用或权限不足

**解决**:

```bash
# 检查串口设备
platformio device list

# 指定串口上传
platformio run -t upload --upload-port COM3
```

#### 3. WiFi连接失败

**问题**: WiFi配置错误或信号弱

**解决**:

1. 检查WiFi名称和密码
2. 使用WiFiManager重新配置
3. 靠近路由器测试

#### 4. Web界面无法访问

**问题**: IP地址获取失败或防火墙阻挡

**解决**:

1. 串口查看IP地址
2. ping测试网络连通性
3. 检查防火墙设置

#### 5. 继电器无响应

**问题**: 硬件连接错误或继电器模块故障

**解决**:

1. 检查GPIO连接
2. 测量继电器控制信号
3. 更换继电器模块

### 调试技巧

#### 串口调试

```bash
# 监控串口输出
platformio device monitor

# 指定波特率
platformio device monitor --baud 115200
```

#### 网络调试

```bash
# ping测试
ping 设备IP

# 端口扫描
nmap -p 80,502,8080,1883 设备IP

# 抓包分析
wireshark
```

#### MQTT调试

```bash
# 查看所有MQTT消息
mosquitto_sub -h MQTT服务器IP -t "#" -v

# 测试MQTT连接
mosquitto_pub -h MQTT服务器IP -t "test" -m "hello"
```

## 📚 进阶配置

### 自定义主题

修改 `web_handlers.cpp` 中的CSS样式:

```cpp
// 自定义颜色主题
const char* custom_css = R"(
  :root {
    --primary-color: #your-color;
    --background-color: #your-bg;
  }
)";
```

### 扩展功能

1. **添加传感器**: 修改 `main.cpp` 添加传感器读取
2. **自定义协议**: 扩展 `tcp_server.cpp` 添加命令
3. **云平台集成**: 修改 `mqtt_handlers.cpp` 适配云端

### 性能优化

1. **内存优化**: 调整字符串缓冲区大小
2. **响应速度**: 优化JSON解析和生成
3. **稳定性**: 增加异常处理和重试机制

## 🎯 部署检查清单

### 部署前检查

- [ ] 硬件连接正确
- [ ] 代码编译成功
- [ ] 串口设备识别
- [ ] WiFi配置正确
- [ ] 防火墙设置适当

### 部署后验证

- [ ] WiFi连接成功
- [ ] Web界面可访问
- [ ] 继电器控制正常
- [ ] MQTT通信正常
- [ ] Modbus功能正常
- [ ] 系统运行稳定

### 生产环境部署

- [ ] 修改默认密码
- [ ] 配置固定IP
- [ ] 设置自动重启
- [ ] 建立监控机制
- [ ] 准备维护文档

---

**快速支持**: 遇到问题请查看 `TROUBLESHOOTING.md` 或提交Issue  
**项目文档**: 完整文档请查看 `README.md`  
**更新日志**: 版本变更请查看 `CHANGELOG.md`
