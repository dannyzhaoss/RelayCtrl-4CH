# 🔧 RelayCtrl-4CH 开发规范文档

## 📋 代码规范

### 文件组织规范

```
src/
├── main.cpp              # 主程序入口、系统初始化、主循环
├── config.h              # 硬件配置、常量定义、EEPROM地址映射
├── relay_controller.h    # 全局声明、结构体定义、函数原型
├── web_handlers.cpp      # HTTP服务器、Web界面、API处理
├── mqtt_handlers.cpp     # MQTT客户端、消息处理、订阅管理
├── modbus_serial.cpp     # Modbus RTU、RS485通信、串口命令
└── tcp_server.cpp        # TCP服务器、Modbus TCP、原始TCP
```

### 命名规范

#### 函数命名
- **初始化函数**: `init*()` - 如 `initWiFi()`, `initMQTT()`
- **处理函数**: `handle*()` - 如 `handleRelayControl()`, `handleMqttControl()`
- **控制函数**: `enable*()`, `start*()`, `stop*()` - 如 `enableMQTT()`
- **工具函数**: 描述性命名 - 如 `saveConfig()`, `printSystemInfo()`

#### 变量命名
- **全局变量**: 驼峰命名 - 如 `relayStates[]`, `config`
- **静态变量**: 描述性命名 - 如 `lastMqttReconnect`, `systemErrorCount`
- **常量定义**: 大写下划线 - 如 `RELAY1_PIN`, `DEFAULT_SSID`

### 内存管理规范

#### EEPROM地址分配
```cpp
#define WIFI_SSID_ADDR 0          // 32字节 - WiFi网络名称
#define WIFI_PASS_ADDR 32         // 64字节 - WiFi密码
#define MQTT_SERVER_ADDR 96       // 64字节 - MQTT服务器地址
#define MQTT_PORT_ADDR 160        // 2字节 - MQTT端口
// ... 依次分配，避免重叠
#define CONFIG_VALID_ADDR 511     // 1字节 - 配置有效性标志
```

#### 字符串处理
- 使用 `char[]` 数组替代 `String` 对象
- 使用 `strncpy()` 和手动null终止
- 预分配固定大小的缓冲区

### 调试输出规范

#### 模块前缀标识
- `SYS:` - 系统初始化和状态
- `WIFI:` - WiFi连接和网络
- `MQTT:` - MQTT通信
- `RELAY:` - 继电器控制
- `MODBUS:` - Modbus通信
- `TCP:` - TCP服务器
- `WEB:` - Web服务器
- `CONFIG:` - 配置管理

#### 输出格式
```cpp
// 推荐格式
Serial.printf("MQTT: R%d=%s\n", relay, state ? "ON" : "OFF");
Serial.println("SYS: Init complete");

// 避免格式
Serial.print("MQTT control: Relay ");
Serial.print(relay);
Serial.println(" state changed");
```

### 错误处理规范

#### 返回值标准
```cpp
// 配置函数返回bool
bool saveConfig() {
    // ... 保存逻辑
    return success;
}

// 初始化函数无返回值，使用Serial报告
void initWiFi() {
    // ... 初始化逻辑
    Serial.println("WIFI: Connected");
}
```

#### 异常处理
```cpp
// 使用try-catch保护关键操作
try {
    handleModbusTcpRequest(clients[i]);
} catch (...) {
    Serial.printf("MODBUS: Error client %d\n", i);
    clients[i].stop();
}
```

## 📡 协议实现规范

### Web API设计

#### 端点命名
- `/` - 主页面
- `/config` - 配置页面
- `/api/relay/control` - 继电器控制API
- `/api/config/get` - 获取配置API
- `/api/config/set` - 设置配置API
- `/api/protocol/control` - 协议控制API

#### 响应格式
```json
{
    "success": true,
    "message": "Operation successful",
    "data": {
        "relay": 1,
        "state": true
    }
}
```

### MQTT主题规范

#### 主题结构
```
relay/{deviceId}/control     # 接收控制命令
relay/{deviceId}/config      # 接收配置命令
relay/{deviceId}/relay1/state # 发布继电器状态
relay/{deviceId}/status      # 发布系统状态
relay/{deviceId}/online      # 发布在线状态
relay/{deviceId}/heartbeat   # 发布心跳信息
```

#### 消息格式
```json
// 控制消息
{"relay": 1, "state": true}
{"command": "all_on"}

// 状态消息
{"deviceId": "RelayCtrl_001", "relays": [...]}
```

### Modbus地址映射

#### 线圈地址 (Coils)
- `0x0000` - 继电器1 (JDQ0)
- `0x0001` - 继电器2 (JDQ1)
- `0x0002` - 继电器3 (JDQ2)
- `0x0003` - 继电器4 (JDQ3)

#### 保持寄存器 (Holding Registers)
- `0x0000` - 设备状态字
- `0x0001` - 继电器状态字
- `0x0002` - 系统运行时间（低16位）
- `0x0003` - 系统运行时间（高16位）

## 🔄 系统生命周期

### 启动序列
1. **Serial初始化** (115200波特率)
2. **EEPROM初始化** (512字节)
3. **配置加载** (`loadConfig()`)
4. **硬件初始化** (`initRelays()`, `initSerialPorts()`)
5. **网络初始化** (`initWiFi()`)
6. **服务初始化** (`initWebServer()`, `initMQTT()`, `initModbus()`, `initTcpServers()`)
7. **OTA设置** (`httpUpdater.setup()`)
8. **mDNS启动** (`MDNS.begin()`)

### 主循环处理
```cpp
void loop() {
    // 1. 处理重启调度
    if (restartScheduled && millis() >= restartScheduledTime) {
        ESP.restart();
    }
    
    // 2. 网络服务处理
    server.handleClient();
    MDNS.update();
    
    // 3. 协议处理
    if (config.mqttEnabled) {
        if (!mqttClient.connected()) {
            reconnectMQTT();
        }
        mqttClient.loop();
    }
    
    // 4. 串口通信
    handleModbus();
    handleSerialCommands();
    
    // 5. TCP服务器
    handleTcpClients();
    
    // 6. 系统维护
    sendHeartbeat();
    printDebugHeartbeat();
    performHealthCheck();
}
```

### 关机/重启流程
1. **调度重启** (`scheduleRestart()`) - 非阻塞式
2. **保存配置** (`saveConfig()`)
3. **断开连接** (MQTT, TCP)
4. **硬件复位** (`ESP.restart()`)

## 📊 性能优化指南

### 内存优化
- **当前使用**: RAM 58.4% (47,864字节), Flash 52.4% (546,975字节)
- **优化策略**:
  - 减少String对象使用
  - 使用静态缓冲区
  - 限制并发TCP连接数
  - 优化JSON缓冲区大小

### 网络优化
```cpp
// MQTT重连间隔优化
if (millis() - lastMqttReconnect < 10000) return;

// TCP处理频率限制
static unsigned long lastProcessTime = 0;
if (millis() - lastProcessTime < 10) return;

// 心跳间隔控制
if (millis() - lastHeartbeat > 30000) {
    sendHeartbeat();
}
```

### 稳定性优化
- **看门狗定时器**: 系统健康检查
- **错误计数器**: 连接失败统计
- **自动恢复**: 服务重启机制
- **资源释放**: 及时清理无效连接

## 🛡️ 安全规范

### Web认证
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

### 输入验证
- **参数范围检查**: 继电器编号1-4
- **JSON格式验证**: 使用deserializeJson错误检查
- **缓冲区边界**: 防止数组越界

### 配置安全
- **默认密码**: 强制用户更改默认密码
- **EEPROM加密**: 考虑敏感数据加密存储
- **OTA认证**: HTTP Basic认证保护固件更新

---

**最后更新**: 2025年8月30日
**文档版本**: v1.0.0
