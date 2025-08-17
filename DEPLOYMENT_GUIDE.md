# RelayCtrl-4CH 部署指南

## 部署概述

本指南将帮助您完成ESP8266 RelayCtrl-4CH继电器控制器的完整部署，包括硬件连接、固件烧录、系统配置和功能测试。

## 🔧 硬件准备

### 必需组件
- **ESP8266 NodeMCU开发板** (推荐ESP-12E)
- **4路继电器模块** (5V触发)
- **RS485转TTL模块** (MAX485芯片)
- **电源适配器** (5V/2A，用于继电器)
- **杜邦线** 若干
- **USB数据线** (用于程序烧录)

### 可选组件
- **面包板或万能板** (用于原型搭建)
- **螺丝端子** (便于现场接线)
- **外壳** (工业环境使用)

## 📐 硬件连接图

### ESP8266 引脚分配
```
NodeMCU Pin    GPIO    功能描述
---------------------------------
D0             GPIO16  继电器4 (JDQ3)
D1             GPIO5   RS485 TX
D2             GPIO4   RS485 RX  
D3             GPIO0   (保留，用于下载模式)
D4             GPIO2   RS485 DE/RE
D5             GPIO14  继电器3 (JDQ2)
D6             GPIO12  继电器1 (JDQ0)
D7             GPIO13  继电器2 (JDQ1)
D8             GPIO15  (保留)
3V3            -       3.3V电源输出
GND            -       公共地线
VIN            -       外部5V输入
```

### 连接示意图
```
ESP8266 NodeMCU              4路继电器模块
┌─────────────────┐         ┌─────────────────┐
│ D6 (GPIO12) ────┼────────▶│ IN1 (JDQ0)      │
│ D7 (GPIO13) ────┼────────▶│ IN2 (JDQ1)      │
│ D5 (GPIO14) ────┼────────▶│ IN3 (JDQ2)      │
│ D0 (GPIO16) ────┼────────▶│ IN4 (JDQ3)      │
│ 3V3 ────────────┼────────▶│ VCC             │
│ GND ────────────┼────────▶│ GND             │
└─────────────────┘         └─────────────────┘

ESP8266 NodeMCU              RS485转TTL模块
┌─────────────────┐         ┌─────────────────┐
│ D1 (GPIO5) ─────┼────────▶│ DI (发送)        │
│ D2 (GPIO4) ─────┼◀────────│ RO (接收)        │
│ D4 (GPIO2) ─────┼────────▶│ DE/RE (方向)     │
│ 3V3 ────────────┼────────▶│ VCC             │
│ GND ────────────┼────────▶│ GND             │
└─────────────────┘         └─────────────────┘
                             │ A+ ◀─── RS485总线
                             │ B- ◀─── RS485总线
```

## ⚡ 电源规划

### 供电方案
1. **ESP8266**: 通过USB供电 (5V → 3.3V板载转换)
2. **继电器模块**: 外部5V/2A适配器供电
3. **RS485模块**: 从ESP8266的3.3V供电

### 注意事项
- 继电器模块功耗较大，不建议从ESP8266供电
- 确保所有模块共地(GND连接)
- RS485模块使用3.3V供电，与ESP8266兼容

## 🛠️ 开发环境搭建

### 1. 安装Python
```bash
# Windows (下载Python 3.13)
https://www.python.org/downloads/

# 验证安装
python --version
```

### 2. 安装PlatformIO
```bash
# 使用pip安装
pip install platformio

# 验证安装
pio --version
```

### 3. 克隆项目
```bash
# 克隆代码库
git clone https://github.com/dannyzhaoss/RelayCtrl-4CH.git
cd RelayCtrl-4CH

# 查看项目结构
dir  # Windows
ls   # Linux/Mac
```

## 📝 编译和烧录

### 1. 项目编译
```bash
# 编译项目
pio run

# 查看编译结果
# RAM:   [======    ]  61.7% (used 50568 bytes from 81920 bytes)
# Flash: [=====     ]  52.6% (used 549167 bytes from 1044464 bytes)
```

### 2. 查找COM端口
```bash
# Windows - 设备管理器查看COM端口
# 或使用PlatformIO命令
pio device list
```

### 3. 烧录固件
```bash
# 烧录到设备 (替换COM13为实际端口)
pio run -t upload --upload-port COM13

# 如果烧录失败，尝试降低波特率
pio run -t upload --upload-port COM13 --upload-speed 115200
```

### 4. 监控串口
```bash
# 启动串口监控
pio device monitor --port COM13

# 查看设备启动日志
# WiFi连接状态
# 服务启动信息
# IP地址分配
```

## 🌐 网络配置

### 首次配网

#### 方法1: 默认WiFi配置
设备会自动尝试连接默认WiFi：
- **SSID**: SSKJ-4
- **密码**: xszn486020zcs

#### 方法2: WiFiManager配网
1. 设备启动后等待30秒
2. 如无法连接默认WiFi，自动进入AP模式
3. 手机连接热点：
   - **SSID**: ESP8266-RelayCtrl
   - **密码**: 12345678
4. 浏览器打开: http://192.168.4.1
5. 选择目标WiFi网络并输入密码
6. 设备重启并连接到新网络

### 查找设备IP
- **路由器管理页面**: 查看DHCP客户端列表
- **串口监控**: 观察启动日志中的IP地址
- **网络扫描**: 使用nmap等工具扫描网段

## 🔧 系统配置

### 1. 访问Web界面
```bash
# 在浏览器中打开设备IP
http://192.168.0.132

# 默认登录凭据
用户名: admin
密码: admin
```

### 2. 基础配置
访问 `http://设备IP/config` 进行配置：

#### WiFi设置
- **网络名称**: 目标WiFi SSID
- **网络密码**: WiFi密码
- **静态IP**: 可选，留空使用DHCP

#### MQTT设置
- **MQTT服务器**: broker.hivemq.com (测试用)
- **端口**: 1883
- **用户名**: 留空或填写
- **密码**: 留空或填写
- **设备ID**: 自动生成或自定义

#### 认证设置
- **Web用户名**: 修改默认用户名
- **Web密码**: 修改默认密码
- **启用认证**: 建议保持开启

#### 服务设置
- **MQTT服务**: 按需启用
- **Modbus TCP**: 按需启用，默认端口502
- **Raw TCP**: 按需启用，默认端口8888

### 3. 保存配置
点击各配置卡片的"保存"按钮，设备会自动应用新配置。

## 🧪 功能测试

### 1. Web界面测试
1. 访问主页面查看继电器状态
2. 点击继电器按钮测试控制功能
3. 观察继电器模块LED指示灯
4. 听到继电器动作声音

### 2. HTTP API测试
```bash
# 获取设备状态
curl -u admin:admin http://192.168.0.132/api/status

# 控制继电器1开启
curl -u admin:admin \
  -X POST \
  -H "Content-Type: application/json" \
  -d '{"relay":0,"state":true}' \
  http://192.168.0.132/api/relay

# 批量控制继电器
curl -u admin:admin \
  -X POST \
  -H "Content-Type: application/json" \
  -d '{"relays":[true,false,true,false]}' \
  http://192.168.0.132/api/relay
```

### 3. TCP原始协议测试
```bash
# 使用telnet连接
telnet 192.168.0.132 8888

# 发送命令测试
RELAY 0 1    # 开启继电器1
RELAY 0 0    # 关闭继电器1
STATUS       # 查询状态
```

### 4. MQTT协议测试
```bash
# 使用mosquitto客户端
# 控制继电器1
mosquitto_pub -h 192.168.0.132 -t "relay/control/0" -m "1"

# 订阅状态更新
mosquitto_sub -h 192.168.0.132 -t "relay/status/+"
```

### 5. Modbus协议测试
```bash
# 使用modpoll工具测试Modbus TCP
# 读取所有继电器状态
modpoll -m tcp -t 0 -r 0 -c 4 192.168.0.132

# 开启继电器1
modpoll -m tcp -t 0 -r 0 -c 1 192.168.0.132 1
```

## 🔧 生产部署

### 1. 硬件优化
- **PCB设计**: 制作专用PCB替代面包板
- **外壳保护**: 选择合适的IP等级外壳
- **端子连接**: 使用螺丝端子便于现场接线
- **指示灯**: 添加电源和状态指示LED

### 2. 软件定制
- **修改默认配置**: 编辑 `config.h` 中的默认值
- **OTA升级**: 配置自动更新服务器
- **日志记录**: 启用生产环境日志
- **看门狗**: 确保系统稳定运行

### 3. 安全加固
```cpp
// 修改默认认证信息
#define DEFAULT_WEB_USERNAME "your_username"
#define DEFAULT_WEB_PASSWORD "your_secure_password"

// 修改默认WiFi配置
#define DEFAULT_WIFI_SSID "your_wifi"
#define DEFAULT_WIFI_PASSWORD "your_wifi_password"
```

### 4. 批量部署
```bash
# 批量编译
pio run

# 批量烧录脚本
for port in COM3 COM4 COM5 COM6; do
  pio run -t upload --upload-port $port
done
```

## 📊 性能优化

### 1. 内存优化
- 当前RAM使用率: 61.7%
- 建议关闭不需要的服务
- 定期重启释放内存碎片

### 2. 网络优化
- 设置合理的MQTT心跳间隔
- 限制同时连接数
- 使用静态IP减少DHCP开销

### 3. 响应速度
- HTTP API响应时间 < 100ms
- TCP连接建立时间 < 50ms
- 继电器动作延迟 < 20ms

## 🚨 故障排除

### 常见问题

#### 1. 设备无法启动
- 检查供电是否正常
- 确认USB连接和驱动
- 查看串口是否有输出

#### 2. WiFi连接失败
- 确认WiFi信号强度
- 检查密码是否正确
- 尝试重置网络配置

#### 3. 继电器无动作
- 检查继电器模块供电
- 确认控制信号连接
- 测试GPIO输出电平

#### 4. 网络服务无响应
- 确认服务已启用
- 检查防火墙设置
- 验证端口配置

### 调试工具
```bash
# 串口监控
pio device monitor

# 网络测试
ping 192.168.0.132
telnet 192.168.0.132 80

# HTTP测试
curl -v http://192.168.0.132/api/status
```

## 📋 验收清单

### 硬件验收
- [ ] 所有连接正确无误
- [ ] 供电电压和电流符合要求
- [ ] 继电器动作正常
- [ ] RS485通信正常
- [ ] 外壳和固定牢靠

### 软件验收
- [ ] 固件烧录成功
- [ ] WiFi连接正常
- [ ] Web界面访问正常
- [ ] API功能测试通过
- [ ] 所需协议测试通过

### 性能验收
- [ ] 响应时间满足要求
- [ ] 内存使用在合理范围
- [ ] 长期运行稳定
- [ ] 断电恢复正常

## 📞 技术支持

### 文档资源
- [项目主页](README.md)
- [API文档](API_DOCUMENTATION.md)
- [项目状态](PROJECT_STATUS.md)

### 问题反馈
- **GitHub Issues**: 报告bug和功能请求
- **项目地址**: https://github.com/dannyzhaoss/RelayCtrl-4CH

### 更新计划
- v1.1.0: WebSocket实时通信
- v1.2.0: 定时器功能
- v1.3.0: 数据记录功能

---

**部署成功后，您的ESP8266继电器控制器即可投入使用！** 🎉
