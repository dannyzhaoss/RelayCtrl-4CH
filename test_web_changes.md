# Web Interface Changes Test

## 已完成的修改

### 1. 页面标题更新
- 将网页标题修改为: "RelayCtrl-4CH 控制器 V1.0 联鲸科技"

### 2. 卡片结构调整
- **原WiFi配置卡片** → **合并到系统信息卡片**
  - 新的系统信息卡片包含：
    - 设备状态显示（IP地址、MAC地址、固件版本等）
    - WiFi网络配置表单
    - 重置按钮功能

- **原系统信息卡片** → **替换为Modbus配置卡片**
  - 包含：
    - Modbus从机地址配置 (1-247)
    - 波特率选择 (9600/19200/38400/57600/115200)
    - 配置保存功能

### 3. 新增功能
- 添加了 `handleSaveModbus()` 函数处理Modbus配置
- 新增 `/saveModbus` 路由
- 在RelayConfig结构体中添加了 `modbusSlaveId` 和 `modbusBaudRate` 字段
- 在 `setDefaultConfig()` 中添加了Modbus默认值设置

### 4. 卡片布局改进
- 系统信息卡片现在是一个综合性信息+配置卡片
- Modbus配置独立成卡片，便于专门配置串口通信参数
- 保持了原有的MQTT、Web认证、网络服务等卡片不变

## 测试要点

1. **页面标题显示**：验证浏览器标题栏显示为新标题
2. **系统信息卡片**：检查设备信息显示和WiFi配置表单
3. **Modbus配置卡片**：测试从机地址和波特率设置
4. **配置保存**：验证各项配置能正确保存并生效
5. **页面导航**：确保配置更新后能正确返回配置页面

## 技术细节

### Modbus配置表单
```html
<form method='post' action='/saveModbus'>
  <input type='number' name='modbusSlaveId' value='...' min='1' max='247' required>
  <select name='modbusBaudRate'>
    <option value='9600'>9600</option>
    <option value='19200'>19200</option>
    <option value='38400'>38400</option>
    <option value='57600'>57600</option>
    <option value='115200'>115200</option>
  </select>
</form>
```

### 配置处理函数
- `handleSaveModbus()`: 处理Modbus配置提交
- 参数验证：从机地址范围1-247
- 配置保存：调用 `saveConfig()` 持久化配置
- 用户反馈：显示更新成功消息并自动跳转
