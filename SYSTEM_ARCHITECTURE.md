# 🏗️ RelayCtrl-4CH 系统架构与实现文档

## 📋 概述

本文档详细描述了ESP8266 RelayCtrl-4CH四路继电器控制器的完整系统架构、实现细节和技术规范。

## 🎯 系统架构总览

### 核心组件架构

```
┌─────────────────────┐    ┌─────────────────────┐    ┌─────────────────────┐
│    Web界面层        │    │     协议层          │    │     硬件层          │
│                     │    │                     │    │                     │
│ ├─ HTML/CSS/JS      │    │ ├─ HTTP/HTTPS       │    │ ├─ ESP8266 MCU      │
│ ├─ REST API         │    │ ├─ MQTT Client      │    │ ├─ 4路继电器        │
│ ├─ 实时监控         │    │ ├─ Modbus RTU       │    │ ├─ RS485接口        │
│ └─ 配置管理         │    │ ├─ Modbus TCP       │    │ └─ GPIO控制         │
│                     │    │ └─ Raw TCP          │    │                     │
└─────────────────────┘    └─────────────────────┘    └─────────────────────┘
           │                          │                          │
           └──────────────────────────┼──────────────────────────┘
                                      │
                         ┌─────────────────────┐
                         │     核心控制层      │
                         │                     │
                         │ ├─ 状态管理         │
                         │ ├─ 配置管理         │
                         │ ├─ 错误处理         │
                         │ ├─ 安全认证         │
                         │ └─ 系统监控         │
                         └─────────────────────┘
```

### 软件架构分层

| 层级 | 文件 | 功能描述 | 接口协议 |
|------|------|----------|----------|
| **应用层** | web_handlers.cpp | Web界面、HTTP API、用户交互 | HTTP/HTML/JSON |
| **协议层** | mqtt_handlers.cpp<br>tcp_server.cpp<br>modbus_serial.cpp | 多协议通信处理 | MQTT/TCP/Modbus |
| **控制层** | main.cpp | 系统初始化、主循环、状态管理 | 函数调用 |
| **硬件抽象层** | relay_controller.h<br>config.h | 硬件配置、GPIO控制 | GPIO/串口 |

## 🔧 核心模块详解

### 1. 系统初始化模块 (main.cpp)

#### 启动流程
```cpp
void setup() {
    // 1. 硬件初始化
    Serial.begin(115200);
    EEPROM.begin(EEPROM_SIZE);
    
    // 2. 配置管理
    loadConfig();
    updateDynamicConfig();
    
    // 3. 硬件设置
    initRelays();
    initSerialPorts();
    
    // 4. 网络服务
    initWiFi();
    initWebServer();
    initMQTT();
    initModbus();
    initTcpServers();
    
    // 5. 系统服务
    setupOTA();
    startMDNS();
}
```

#### 主循环架构
```cpp
void loop() {
    // 重启调度处理 (非阻塞)
    handleScheduledRestart();
    
    // 网络服务处理
    server.handleClient();
    MDNS.update();
    
    // 协议处理
    handleMQTT();
    handleModbus();
    handleTcpClients();
    
    // 系统维护
    performSystemMaintenance();
}
```

### 2. 配置管理系统

#### 配置结构体定义
```cpp
typedef struct {
    // 网络配置
    char ssid[32];
    char password[64];
    
    // MQTT配置
    char mqttServer[64];
    int mqttPort;
    char mqttTopic[64];
    char mqttUsername[32];
    char mqttPassword[32];
    
    // 设备配置
    char deviceId[32];
    char webUsername[16];
    char webPassword[16];
    
    // 端口配置
    int rawTcpPort;
    int modbusTcpPort;
    
    // Modbus配置
    int modbusSlaveId;
    int modbusBaudRate;
    
    // 协议启用状态
    bool mqttEnabled;
    bool tcpEnabled;
    bool modbusTcpEnabled;
    bool webAuthEnabled;
    
    bool valid;
} RelayConfig;
```

#### EEPROM存储映射
```
地址范围    大小    内容描述
0-31       32字节  WiFi SSID
32-95      64字节  WiFi密码
96-159     64字节  MQTT服务器地址
160-161    2字节   MQTT端口
162-225    64字节  MQTT主题前缀
226-257    32字节  MQTT用户名
258-289    32字节  MQTT密码
290-321    32字节  设备ID
322-337    16字节  Web用户名
338-353    16字节  Web密码
354-355    2字节   原始TCP端口
356-357    2字节   Modbus TCP端口
358        1字节   Modbus从站地址
359-362    4字节   Modbus波特率
363        1字节   MQTT启用状态
364        1字节   TCP启用状态
365        1字节   Modbus TCP启用状态
366        1字节   Web认证启用状态
511        1字节   配置有效性标志(0xAA)
```

### 3. Web服务器模块 (web_handlers.cpp)

#### API端点设计

| 端点 | 方法 | 功能 | 认证 |
|------|------|------|------|
| `/` | GET | 主页面 | 可选 |
| `/config` | GET | 配置页面 | 必需 |
| `/api/relay/control` | POST | 继电器控制 | 必需 |
| `/api/config/get` | GET | 获取配置 | 必需 |
| `/api/config/set` | POST | 设置配置 | 必需 |
| `/api/protocol/control` | POST | 协议控制 | 必需 |
| `/api/status` | GET | 系统状态 | 可选 |
| `/restart` | POST | 系统重启 | 必需 |
| `/update` | GET/POST | OTA固件更新 | 必需 |

#### JSON API响应格式
```json
{
    "success": boolean,
    "message": "操作结果描述",
    "data": {
        // 具体数据内容
    },
    "timestamp": 1693392000
}
```

### 4. MQTT通信模块 (mqtt_handlers.cpp)

#### 主题结构设计
```
relay/{deviceId}/control     -> 接收控制命令
relay/{deviceId}/config      -> 接收配置命令  
relay/{deviceId}/relay1/state -> 发布继电器1状态
relay/{deviceId}/relay2/state -> 发布继电器2状态
relay/{deviceId}/relay3/state -> 发布继电器3状态
relay/{deviceId}/relay4/state -> 发布继电器4状态
relay/{deviceId}/status      -> 发布系统状态
relay/{deviceId}/online      -> 发布在线状态
relay/{deviceId}/heartbeat   -> 发布心跳信息
```

#### 消息格式规范
```json
// 继电器控制消息
{
    "relay": 1,        // 继电器编号 1-4
    "state": true      // 状态 true=开启, false=关闭
}

// 批量控制消息
{
    "command": "all_on"    // all_on | all_off | status | restart
}

// 状态发布消息
{
    "deviceId": "RelayCtrl_001",
    "ip": "192.168.1.100",
    "wifi": "SSKJ-4",
    "rssi": -45,
    "uptime": 3600,
    "freeHeap": 25000,
    "relays": [
        {"id": 1, "state": "ON", "pin": 12},
        {"id": 2, "state": "OFF", "pin": 13},
        {"id": 3, "state": "OFF", "pin": 14},
        {"id": 4, "state": "ON", "pin": 16}
    ]
}
```

### 5. Modbus通信模块 (modbus_serial.cpp)

#### 支持的功能码

| 功能码 | 名称 | 描述 | 实现状态 |
|--------|------|------|----------|
| 0x01 | Read Coils | 读取线圈状态 | ✅ |
| 0x03 | Read Holding Registers | 读取保持寄存器 | ✅ |
| 0x05 | Write Single Coil | 写单个线圈 | ✅ |
| 0x06 | Write Single Register | 写单个寄存器 | ✅ |
| 0x0F | Write Multiple Coils | 写多个线圈 | ✅ |

#### 地址映射表

**线圈地址 (Coils) - 继电器控制**
```
地址    描述        GPIO引脚
0x0000  继电器1     D6 (GPIO12)
0x0001  继电器2     D7 (GPIO13)  
0x0002  继电器3     D5 (GPIO14)
0x0003  继电器4     D0 (GPIO16)
```

**保持寄存器 (Holding Registers) - 状态读取**
```
地址    描述                    数据类型
0x0000  设备状态字              16位无符号整数
0x0001  继电器状态字            16位无符号整数
0x0002  系统运行时间(低16位)    16位无符号整数
0x0003  系统运行时间(高16位)    16位无符号整数
0x0004  空闲内存大小(低16位)    16位无符号整数
0x0005  空闲内存大小(高16位)    16位无符号整数
```

### 6. TCP服务器模块 (tcp_server.cpp)

#### 双TCP服务架构
```
原始TCP服务器 (端口8080)
├─ 简单文本命令协议
├─ 继电器控制指令
├─ 状态查询指令  
└─ 帮助信息显示

Modbus TCP服务器 (端口502)
├─ 标准Modbus TCP协议
├─ MBAP头部处理
├─ PDU数据处理
└─ 功能码路由
```

#### 原始TCP命令格式
```
RELAY 1 ON          -> 开启继电器1
RELAY 2 OFF         -> 关闭继电器2  
RELAY ALL ON        -> 开启所有继电器
RELAY ALL OFF       -> 关闭所有继电器
STATUS              -> 查询系统状态
HELP                -> 显示帮助信息
```

## 🔐 安全机制

### 1. Web认证
```cpp
bool checkAuthentication() {
    if (!config.webAuthEnabled) return true;
    
    if (!server.authenticate(config.webUsername, config.webPassword)) {
        server.requestAuthentication();
        return false;
    }
    return true;
}
```

### 2. 输入验证
- **继电器编号范围**: 1-4
- **JSON格式验证**: deserializeJson错误检查
- **缓冲区边界保护**: 防止数组越界
- **参数合法性检查**: 空值和格式验证

### 3. 网络安全
- **默认密码策略**: 强制用户更改
- **OTA更新保护**: HTTP Basic认证
- **连接数限制**: 防止资源耗尽
- **超时机制**: 防止连接泄漏

## 📊 性能特征

### 内存使用分析
```
当前编译状态 (v1.0.0):
RAM:   58.4% (47,864 / 81,920 字节)
Flash: 52.4% (546,975 / 1,044,464 字节)

内存分配详情:
- 全局变量:     ~8KB
- 网络缓冲区:   ~12KB  
- MQTT客户端:   ~8KB
- Web服务器:    ~15KB
- 其他运行时:   ~4KB
```

### 性能优化策略

#### 1. 内存优化
- 使用静态缓冲区替代动态分配
- char[]数组替代String对象
- 减少JSON缓冲区大小
- 限制并发TCP连接数量

#### 2. 网络优化  
- MQTT重连间隔控制 (10秒)
- TCP处理频率限制 (10ms)
- 心跳间隔优化 (30秒)
- 连接超时设置 (50ms)

#### 3. 系统稳定性
- 非阻塞重启机制
- 错误计数和自动恢复
- 看门狗式健康检查
- 资源泄漏防护

## 🚀 扩展能力

### 1. 硬件扩展
- **继电器数量**: 支持扩展到8路/16路
- **传感器接入**: 预留GPIO用于传感器
- **通信接口**: 支持SPI/I2C设备
- **电源管理**: 低功耗模式支持

### 2. 协议扩展
- **CoAP协议**: 物联网轻量级协议
- **WebSocket**: 实时双向通信
- **LoRaWAN**: 长距离低功耗通信
- **BLE**: 蓝牙低功耗支持

### 3. 功能扩展
- **定时任务**: 定时开关继电器
- **场景模式**: 预设控制方案
- **数据记录**: 操作日志存储
- **故障诊断**: 自动故障检测

## 🔍 调试与维护

### 调试输出规范
```cpp
// 模块前缀标识
SYS:     系统初始化和状态
WIFI:    WiFi连接和网络
MQTT:    MQTT通信
RELAY:   继电器控制
MODBUS:  Modbus通信
TCP:     TCP服务器
WEB:     Web服务器
CONFIG:  配置管理

// 输出格式示例
Serial.printf("MQTT: R%d=%s\n", relay, state ? "ON" : "OFF");
Serial.println("SYS: Init complete");
```

### 系统监控
- **内存监控**: 定期检查空闲内存
- **连接监控**: 网络连接状态检查
- **错误统计**: 系统错误计数
- **性能指标**: 响应时间统计

---

**文档版本**: v1.0.1  
**最后更新**: 2025年8月30日  
**适用固件**: v1.0.0+
