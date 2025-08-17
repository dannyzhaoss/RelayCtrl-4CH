# RelayCtrl-4CH 测试指南

## 测试概述

本文档提供完整的测试方案，确保RelayCtrl-4CH继电器控制器的所有功能正常工作。包括硬件测试、软件功能测试、性能测试和集成测试。

## 🧪 测试环境准备

### 硬件要求
- ESP8266 RelayCtrl-4CH设备 (已部署)
- 4路继电器负载 (LED、蜂鸣器或实际负载)
- 网络环境 (WiFi路由器)
- 计算机 (用于测试工具)
- 万用表 (用于电气测试)

### 软件工具
```bash
# HTTP测试工具
curl
postman (可选)

# MQTT测试工具
mosquitto-clients
MQTT Explorer (GUI工具)

# Modbus测试工具
modpoll
QModMaster (GUI工具)

# 网络工具
telnet
nmap
ping
```

## 📋 测试检查表

### 基础功能测试
- [ ] 设备启动和WiFi连接
- [ ] Web界面访问和认证
- [ ] 继电器基本控制
- [ ] API接口响应
- [ ] 配置保存和恢复

### 协议测试
- [ ] HTTP REST API
- [ ] TCP原始协议
- [ ] MQTT发布/订阅
- [ ] Modbus RTU (串口)
- [ ] Modbus TCP (网络)

### 性能测试
- [ ] 响应时间测试
- [ ] 并发连接测试
- [ ] 内存使用监控
- [ ] 长期稳定性测试

### 安全测试
- [ ] 认证机制验证
- [ ] 未授权访问阻止
- [ ] 服务启用/禁用控制

## 🔧 硬件测试

### 1. 供电测试
```bash
# 检查项目:
- ESP8266工作电压: 3.3V ±5%
- 继电器模块供电: 5V ±5%
- 工作电流 < 500mA (ESP8266)
- 待机电流 < 100mA
```

### 2. GPIO输出测试
使用万用表测试GPIO输出电平：

```bash
# 继电器关闭状态 (默认)
GPIO12 (JDQ0): 3.3V (高电平)
GPIO13 (JDQ1): 3.3V (高电平)
GPIO14 (JDQ2): 3.3V (高电平)
GPIO16 (JDQ3): 3.3V (高电平)

# 继电器开启状态
GPIO12 (JDQ0): 0V (低电平)
GPIO13 (JDQ1): 0V (低电平)
GPIO14 (JDQ2): 0V (低电平)
GPIO16 (JDQ3): 0V (低电平)
```

### 3. 继电器动作测试
```bash
# 验证项目:
- 继电器吸合声音清脆
- LED指示灯状态正确
- 触点导通阻抗 < 1Ω
- 触点断开阻抗 > 1MΩ
```

### 4. RS485通信测试
```bash
# 物理层测试:
- A+/B-差分电压: ±2V~5V
- 空闲状态电压: A+ > B+
- 传输状态波形正常
- 终端电阻: 120Ω (可选)
```

## 🌐 网络功能测试

### 1. WiFi连接测试

#### 默认WiFi连接
```bash
# 串口监控观察连接过程
pio device monitor --port COM13

# 预期输出:
# WiFi连接中...
# WiFi已连接: SSKJ-4
# IP地址: 192.168.x.x
# 信号强度: -XX dBm
```

#### WiFiManager配网测试
```bash
# 测试步骤:
1. 擦除WiFi配置或使用错误密码
2. 设备进入AP模式: ESP8266-RelayCtrl
3. 手机连接热点 (密码: 12345678)
4. 访问: http://192.168.4.1
5. 选择目标WiFi并配置
6. 验证重启后连接成功
```

### 2. Web界面测试

#### 基础访问测试
```bash
# 主页面访问
curl -I http://192.168.0.132/
# 预期: HTTP/1.1 401 Unauthorized (未认证)

# 认证访问
curl -u admin:admin http://192.168.0.132/
# 预期: HTTP/1.1 200 OK + HTML内容

# 配置页面
curl -u admin:admin http://192.168.0.132/config
# 预期: HTTP/1.1 200 OK + 配置页面
```

#### 界面功能测试
```bash
# 测试清单:
- [ ] 页面正常加载 (< 3秒)
- [ ] 继电器状态显示正确
- [ ] 继电器按钮控制有效
- [ ] 系统信息显示正确
- [ ] 配置页面功能正常
- [ ] 移动端响应式适配
```

## 🔌 API接口测试

### 1. 状态查询API

```bash
# 基础状态查询
curl -u admin:admin \
  -H "Accept: application/json" \
  http://192.168.0.132/api/status

# 预期响应结构验证:
{
  "status": "success",
  "data": {
    "relays": [false, false, false, false],
    "wifi": { "ssid": "...", "ip": "...", "rssi": -XX },
    "memory": { "free": XXXXX, "heap": XXXXX },
    "uptime": XXXXX,
    "firmware": "v1.0.0"
  }
}
```

### 2. 继电器控制API

```bash
# 单个继电器控制测试
for i in {0..3}; do
  echo "测试继电器 $i"
  
  # 开启继电器
  curl -u admin:admin \
    -X POST \
    -H "Content-Type: application/json" \
    -d "{\"relay\":$i,\"state\":true}" \
    http://192.168.0.132/api/relay
  
  sleep 2
  
  # 关闭继电器
  curl -u admin:admin \
    -X POST \
    -H "Content-Type: application/json" \
    -d "{\"relay\":$i,\"state\":false}" \
    http://192.168.0.132/api/relay
  
  sleep 1
done
```

```bash
# 批量控制测试
curl -u admin:admin \
  -X POST \
  -H "Content-Type: application/json" \
  -d '{"relays":[true,false,true,false]}' \
  http://192.168.0.132/api/relay

# 验证状态
curl -u admin:admin http://192.168.0.132/api/status | jq '.data.relays'
# 预期: [true, false, true, false]
```

### 3. 错误处理测试

```bash
# 无效继电器编号
curl -u admin:admin \
  -X POST \
  -H "Content-Type: application/json" \
  -d '{"relay":5,"state":true}' \
  http://192.168.0.132/api/relay
# 预期: HTTP 400 + 错误信息

# 无效JSON格式
curl -u admin:admin \
  -X POST \
  -H "Content-Type: application/json" \
  -d '{invalid json}' \
  http://192.168.0.132/api/relay
# 预期: HTTP 400 + JSON格式错误

# 无认证访问
curl -X POST http://192.168.0.132/api/relay
# 预期: HTTP 401 + 认证要求
```

## 📡 TCP协议测试

### 1. 原始TCP连接测试

```bash
# 连接测试
telnet 192.168.0.132 8888

# 命令测试序列
STATUS          # 查询所有状态
RELAY 0 1       # 开启继电器1
RELAY 0 0       # 关闭继电器1
RELAY 1 1       # 开启继电器2
STATUS          # 再次查询状态
quit            # 退出连接
```

### 2. 并发连接测试

```bash
# 测试脚本 (bash)
#!/bin/bash
for i in {1..4}; do
  {
    echo "连接 $i 开始"
    (
      echo "STATUS"
      sleep 2
      echo "RELAY $((i-1)) 1"
      sleep 2
      echo "RELAY $((i-1)) 0"
      sleep 1
    ) | telnet 192.168.0.132 8888
    echo "连接 $i 结束"
  } &
done
wait
```

### 3. 命令响应时间测试

```bash
# 性能测试脚本
#!/bin/bash
start_time=$(date +%s%N)
echo "STATUS" | telnet 192.168.0.132 8888 >/dev/null 2>&1
end_time=$(date +%s%N)
response_time=$((($end_time - $start_time) / 1000000))
echo "TCP响应时间: ${response_time}ms"
```

## 📨 MQTT协议测试

### 1. 连接和认证测试

```bash
# 基础连接测试
mosquitto_pub -h 192.168.0.132 -t "test/connection" -m "hello"

# 带认证连接 (如果配置了MQTT用户密码)
mosquitto_pub -h 192.168.0.132 -u username -P password \
  -t "test/auth" -m "authenticated"
```

### 2. 继电器控制测试

```bash
# 控制单个继电器
for i in {0..3}; do
  echo "测试继电器 $i"
  mosquitto_pub -h 192.168.0.132 -t "relay/control/$i" -m "1"
  sleep 2
  mosquitto_pub -h 192.168.0.132 -t "relay/control/$i" -m "0"
  sleep 1
done

# 批量控制测试
mosquitto_pub -h 192.168.0.132 -t "relay/control/all" -m "1111"
sleep 3
mosquitto_pub -h 192.168.0.132 -t "relay/control/all" -m "0000"
```

### 3. 状态订阅测试

```bash
# 订阅所有状态更新
mosquitto_sub -h 192.168.0.132 -t "relay/status/+" -v

# 订阅系统信息
mosquitto_sub -h 192.168.0.132 -t "relay/system/+" -v

# 在另一个终端控制继电器，观察状态更新
mosquitto_pub -h 192.168.0.132 -t "relay/control/0" -m "1"
```

### 4. QoS和保持连接测试

```bash
# QoS 1 消息测试
mosquitto_pub -h 192.168.0.132 -t "relay/control/0" -m "1" -q 1

# 保持连接测试
mosquitto_sub -h 192.168.0.132 -t "relay/status/+" -k 60 -v
```

## ⚙️ Modbus协议测试

### 1. Modbus TCP测试

```bash
# 读取所有继电器状态 (功能码01)
modpoll -m tcp -t 0 -r 0 -c 4 192.168.0.132
# 预期: 返回4个线圈状态

# 写入单个继电器 (功能码05)
modpoll -m tcp -t 0 -r 0 -c 1 192.168.0.132 1  # 开启
modpoll -m tcp -t 0 -r 0 -c 1 192.168.0.132 0  # 关闭

# 写入多个继电器 (功能码15)
modpoll -m tcp -t 0 -r 0 -c 4 192.168.0.132 1 0 1 0
```

### 2. Modbus RTU测试

```bash
# 使用串口工具测试 (需要RS485转USB设备)
# 读取线圈状态: 从站1，功能码01，起始地址0，数量4
# 命令: 01 01 00 00 00 04 [CRC]

# 写入单个线圈: 从站1，功能码05，地址0，值FF00
# 命令: 01 05 00 00 FF 00 [CRC]
```

### 3. 异常响应测试

```bash
# 无效功能码测试
# 预期返回异常码01 (非法功能码)

# 无效地址测试
modpoll -m tcp -t 0 -r 10 -c 1 192.168.0.132
# 预期返回异常码02 (非法数据地址)

# 无效数量测试
modpoll -m tcp -t 0 -r 0 -c 100 192.168.0.132
# 预期返回异常码03 (非法数据值)
```

## 🎯 性能测试

### 1. 响应时间测试

```bash
# HTTP API响应时间
#!/bin/bash
total_time=0
count=100

for i in {1..100}; do
  start=$(date +%s%N)
  curl -s -u admin:admin http://192.168.0.132/api/status >/dev/null
  end=$(date +%s%N)
  time_ms=$(((end - start) / 1000000))
  total_time=$((total_time + time_ms))
  echo "请求 $i: ${time_ms}ms"
done

avg_time=$((total_time / count))
echo "平均响应时间: ${avg_time}ms"
```

### 2. 并发测试

```bash
# 并发HTTP请求测试
#!/bin/bash
concurrent_users=10
requests_per_user=10

for i in $(seq 1 $concurrent_users); do
  {
    for j in $(seq 1 $requests_per_user); do
      curl -s -u admin:admin \
        -X POST \
        -H "Content-Type: application/json" \
        -d '{"relay":0,"state":true}' \
        http://192.168.0.132/api/relay >/dev/null
      
      curl -s -u admin:admin \
        -X POST \
        -H "Content-Type: application/json" \
        -d '{"relay":0,"state":false}' \
        http://192.168.0.132/api/relay >/dev/null
    done
  } &
done
wait
echo "并发测试完成"
```

### 3. 内存监控测试

```bash
# 通过API监控内存使用
#!/bin/bash
while true; do
  memory=$(curl -s -u admin:admin http://192.168.0.132/api/status | \
    jq -r '.data.memory.free')
  timestamp=$(date '+%Y-%m-%d %H:%M:%S')
  echo "$timestamp 空闲内存: $memory bytes"
  sleep 10
done
```

### 4. 长期稳定性测试

```bash
# 24小时压力测试脚本
#!/bin/bash
start_time=$(date +%s)
test_duration=$((24 * 60 * 60))  # 24小时
iteration=0

while [ $(($(date +%s) - start_time)) -lt $test_duration ]; do
  iteration=$((iteration + 1))
  
  # 随机控制继电器
  relay=$((RANDOM % 4))
  state=$((RANDOM % 2))
  
  curl -s -u admin:admin \
    -X POST \
    -H "Content-Type: application/json" \
    -d "{\"relay\":$relay,\"state\":$([ $state -eq 1 ] && echo true || echo false)}" \
    http://192.168.0.132/api/relay >/dev/null
  
  if [ $((iteration % 100)) -eq 0 ]; then
    echo "$(date) - 完成 $iteration 次操作"
  fi
  
  sleep 5
done

echo "长期稳定性测试完成"
```

## 🔒 安全测试

### 1. 认证测试

```bash
# 无认证访问测试
curl -I http://192.168.0.132/api/status
# 预期: HTTP 401 Unauthorized

# 错误认证测试
curl -u admin:wrongpassword http://192.168.0.132/api/status
# 预期: HTTP 401 Unauthorized

# 正确认证测试
curl -u admin:admin http://192.168.0.132/api/status
# 预期: HTTP 200 OK + JSON数据
```

### 2. 服务端口扫描

```bash
# 扫描开放端口
nmap -p 1-65535 192.168.0.132

# 预期结果分析:
# 端口80: 开放 (HTTP服务)
# 端口502: 开放/关闭 (Modbus TCP，取决于配置)
# 端口8888: 开放/关闭 (Raw TCP，取决于配置)
# 端口1883: 不应开放 (设备不提供MQTT broker)
```

### 3. 输入验证测试

```bash
# SQL注入测试 (应该被阻止)
curl -u admin:admin \
  -X POST \
  -H "Content-Type: application/json" \
  -d '{"relay":"0 OR 1=1","state":true}' \
  http://192.168.0.132/api/relay

# XSS测试 (应该被转义)
curl -u admin:admin \
  -X POST \
  -H "Content-Type: application/json" \
  -d '{"relay":"<script>alert(1)</script>","state":true}' \
  http://192.168.0.132/api/relay
```

## 📊 测试报告模板

### 测试环境信息
```
设备型号: ESP8266 NodeMCU
固件版本: v1.0.0
测试日期: YYYY-MM-DD
测试工程师: [姓名]
网络环境: [WiFi信息]
```

### 测试结果汇总
```
硬件测试: ✅ 通过 / ❌ 失败
网络功能: ✅ 通过 / ❌ 失败
API接口: ✅ 通过 / ❌ 失败
TCP协议: ✅ 通过 / ❌ 失败
MQTT协议: ✅ 通过 / ❌ 失败
Modbus协议: ✅ 通过 / ❌ 失败
性能指标: ✅ 达标 / ❌ 不达标
安全测试: ✅ 通过 / ❌ 失败
```

### 性能指标
```
HTTP响应时间: < 100ms ✅
TCP响应时间: < 50ms ✅
内存使用率: 61.7% ✅
并发连接数: 4个 ✅
长期运行: 24小时 ✅
```

### 问题记录
```
问题编号: P001
问题描述: [详细描述]
重现步骤: [步骤列表]
预期结果: [预期行为]
实际结果: [实际行为]
严重级别: 高/中/低
状态: 待修复/已修复/已验证
```

## 🚀 自动化测试

### 测试脚本集成
```bash
# 创建完整测试套件
#!/bin/bash
DEVICE_IP="192.168.0.132"
TEST_USER="admin"
TEST_PASS="admin"

echo "开始RelayCtrl-4CH完整测试..."

# 1. 基础连通性测试
echo "1. 基础连通性测试"
ping -c 3 $DEVICE_IP || exit 1

# 2. HTTP API测试
echo "2. HTTP API测试"
./test_http_api.sh $DEVICE_IP $TEST_USER $TEST_PASS || exit 1

# 3. TCP协议测试
echo "3. TCP协议测试"
./test_tcp_protocol.sh $DEVICE_IP || exit 1

# 4. MQTT协议测试
echo "4. MQTT协议测试"
./test_mqtt_protocol.sh $DEVICE_IP || exit 1

# 5. 性能测试
echo "5. 性能测试"
./test_performance.sh $DEVICE_IP $TEST_USER $TEST_PASS || exit 1

echo "所有测试完成！"
```

### CI/CD集成
```yaml
# GitHub Actions配置示例
name: Hardware Test
on: [push, pull_request]

jobs:
  hardware-test:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v2
    - name: Install test tools
      run: |
        sudo apt-get update
        sudo apt-get install curl mosquitto-clients
    - name: Run tests
      run: ./run_all_tests.sh
      env:
        DEVICE_IP: ${{ secrets.DEVICE_IP }}
        TEST_USER: ${{ secrets.TEST_USER }}
        TEST_PASS: ${{ secrets.TEST_PASS }}
```

---

通过以上完整的测试方案，可以确保RelayCtrl-4CH设备的所有功能正常、性能达标、安全可靠。建议在生产部署前执行完整的测试流程，并保留测试记录作为质量保证依据。
