# RelayCtrl-4CH API 接口文档

## 概述

RelayCtrl-4CH 提供完整的 RESTful API 接口，支持继电器控制、状态查询、系统配置等功能。所有API都采用JSON格式进行数据交换。

## 认证

API 使用 HTTP Basic Authentication 进行访问控制。

```http
Authorization: Basic YWRtaW46YWRtaW4=
```

**默认凭据**:
- 用户名: `admin`
- 密码: `admin`

## 基础信息

- **Base URL**: `http://{device_ip}`
- **Content-Type**: `application/json`
- **认证方式**: HTTP Basic Auth
- **字符编码**: UTF-8

## API 端点

### 1. 系统状态

#### 获取系统状态
```http
GET /api/status
```

**响应示例**:
```json
{
  "status": "success",
  "data": {
    "relays": [false, true, false, true],
    "wifi": {
      "ssid": "SSKJ-4",
      "ip": "192.168.0.132",
      "rssi": -45,
      "mac": "AA:BB:CC:DD:EE:FF"
    },
    "memory": {
      "free": 31352,
      "heap": 31352,
      "usage": "61.7%"
    },
    "uptime": 86400,
    "firmware": "v1.0.0",
    "services": {
      "mqtt": false,
      "modbus_tcp": false,
      "raw_tcp": false
    }
  }
}
```

**字段说明**:
- `relays`: 四路继电器状态数组 (0-3对应JDQ0-JDQ3)
- `wifi.ssid`: 当前连接的WiFi网络名称
- `wifi.ip`: 设备IP地址
- `wifi.rssi`: WiFi信号强度 (dBm)
- `memory.free`: 空闲内存 (bytes)
- `uptime`: 设备运行时间 (秒)
- `services`: 各网络服务启用状态

### 2. 继电器控制

#### 控制单个继电器
```http
POST /api/relay
```

**请求参数**:
```json
{
  "relay": 0,        // 继电器编号 (0-3)
  "state": true      // 继电器状态 (true=开启, false=关闭)
}
```

**响应示例**:
```json
{
  "status": "success",
  "message": "继电器1已开启",
  "data": {
    "relay": 0,
    "state": true,
    "timestamp": 1640995200
  }
}
```

#### 批量控制继电器
```http
POST /api/relay
```

**请求参数**:
```json
{
  "relays": [true, false, true, false]  // 四路继电器状态数组
}
```

**响应示例**:
```json
{
  "status": "success",
  "message": "继电器状态已更新",
  "data": {
    "relays": [true, false, true, false],
    "changed": [0, 2],  // 状态发生变化的继电器编号
    "timestamp": 1640995200
  }
}
```

### 3. 协议控制

#### 启用/禁用网络服务
```http
POST /api/protocol
```

**请求参数**:
```json
{
  "mqtt": true,           // MQTT服务
  "modbus_tcp": false,    // Modbus TCP服务
  "raw_tcp": true,        // 原始TCP服务
  "ports": {              // 端口配置 (可选)
    "modbus_tcp": 502,
    "raw_tcp": 8888
  }
}
```

**响应示例**:
```json
{
  "status": "success",
  "message": "网络服务配置已更新",
  "data": {
    "services": {
      "mqtt": true,
      "modbus_tcp": false,
      "raw_tcp": true
    },
    "ports": {
      "modbus_tcp": 502,
      "raw_tcp": 8888
    },
    "restart_required": false
  }
}
```

### 4. 系统管理

#### 重启设备
```http
POST /api/restart
```

**请求参数**: 无

**响应示例**:
```json
{
  "status": "success",
  "message": "设备将在3秒后重启",
  "data": {
    "restart_in": 3
  }
}
```

## 错误处理

### 错误响应格式
```json
{
  "status": "error",
  "error": "错误类型",
  "message": "详细错误信息",
  "code": 400
}
```

### 常见错误码

| HTTP状态码 | 错误类型 | 说明 |
|-----------|---------|------|
| 400 | Bad Request | 请求参数错误 |
| 401 | Unauthorized | 认证失败 |
| 404 | Not Found | 接口不存在 |
| 405 | Method Not Allowed | HTTP方法不支持 |
| 500 | Internal Server Error | 服务器内部错误 |

### 错误示例

#### 参数错误
```json
{
  "status": "error",
  "error": "InvalidParameter",
  "message": "继电器编号超出范围，应为0-3",
  "code": 400
}
```

#### 认证失败
```json
{
  "status": "error",
  "error": "Unauthorized",
  "message": "用户名或密码错误",
  "code": 401
}
```

## 使用示例

### cURL 示例

#### 1. 获取状态
```bash
curl -u admin:admin \
  http://192.168.0.132/api/status
```

#### 2. 开启继电器1
```bash
curl -u admin:admin \
  -X POST \
  -H "Content-Type: application/json" \
  -d '{"relay":0,"state":true}' \
  http://192.168.0.132/api/relay
```

#### 3. 批量控制继电器
```bash
curl -u admin:admin \
  -X POST \
  -H "Content-Type: application/json" \
  -d '{"relays":[true,false,true,false]}' \
  http://192.168.0.132/api/relay
```

#### 4. 启用MQTT服务
```bash
curl -u admin:admin \
  -X POST \
  -H "Content-Type: application/json" \
  -d '{"mqtt":true}' \
  http://192.168.0.132/api/protocol
```

### JavaScript 示例

```javascript
// 基础配置
const baseURL = 'http://192.168.0.132';
const auth = btoa('admin:admin');
const headers = {
  'Authorization': `Basic ${auth}`,
  'Content-Type': 'application/json'
};

// 获取状态
async function getStatus() {
  const response = await fetch(`${baseURL}/api/status`, { headers });
  return await response.json();
}

// 控制继电器
async function controlRelay(relay, state) {
  const response = await fetch(`${baseURL}/api/relay`, {
    method: 'POST',
    headers,
    body: JSON.stringify({ relay, state })
  });
  return await response.json();
}

// 使用示例
getStatus().then(data => console.log('设备状态:', data));
controlRelay(0, true).then(data => console.log('继电器控制:', data));
```

### Python 示例

```python
import requests
import json

class RelayController:
    def __init__(self, host, username='admin', password='admin'):
        self.base_url = f'http://{host}'
        self.auth = (username, password)
        self.headers = {'Content-Type': 'application/json'}
    
    def get_status(self):
        """获取设备状态"""
        response = requests.get(f'{self.base_url}/api/status', auth=self.auth)
        return response.json()
    
    def control_relay(self, relay, state):
        """控制单个继电器"""
        data = {'relay': relay, 'state': state}
        response = requests.post(
            f'{self.base_url}/api/relay',
            json=data,
            auth=self.auth,
            headers=self.headers
        )
        return response.json()
    
    def control_all_relays(self, states):
        """批量控制继电器"""
        data = {'relays': states}
        response = requests.post(
            f'{self.base_url}/api/relay',
            json=data,
            auth=self.auth,
            headers=self.headers
        )
        return response.json()
    
    def restart_device(self):
        """重启设备"""
        response = requests.post(f'{self.base_url}/api/restart', auth=self.auth)
        return response.json()

# 使用示例
controller = RelayController('192.168.0.132')

# 获取状态
status = controller.get_status()
print(f"设备状态: {status}")

# 开启继电器1
result = controller.control_relay(0, True)
print(f"继电器控制: {result}")

# 批量控制 (1和3开启，2和4关闭)
result = controller.control_all_relays([True, False, True, False])
print(f"批量控制: {result}")
```

## WebSocket 支持 (计划中)

未来版本将支持WebSocket实时通信：

```javascript
// WebSocket连接示例 (计划功能)
const ws = new WebSocket('ws://192.168.0.132/ws');

ws.onmessage = function(event) {
  const data = JSON.parse(event.data);
  console.log('实时状态更新:', data);
};

// 发送控制命令
ws.send(JSON.stringify({
  type: 'relay_control',
  relay: 0,
  state: true
}));
```

## 注意事项

1. **认证安全**: 请及时修改默认的用户名密码
2. **并发限制**: 同时最多支持4个HTTP连接
3. **响应时间**: 正常情况下API响应时间 < 100ms
4. **内存使用**: 频繁调用API可能增加内存使用，建议适当控制频率
5. **网络稳定**: 确保设备网络连接稳定，避免请求超时

## 更新日志

### v1.0.0 (2024-12)
- ✅ 基础API功能完成
- ✅ 继电器控制接口
- ✅ 系统状态查询
- ✅ 协议配置接口
- ✅ 错误处理机制
- ✅ 认证安全机制

---

更多信息请参考 [项目主页](README.md) 和 [开发文档](PROJECT_STATUS.md)。
