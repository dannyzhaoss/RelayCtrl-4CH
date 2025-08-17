#include "config.h"
#include "relay_controller.h"
#include <ArduinoJson.h>

// 认证相关
String sessionToken = "";
bool isAuthenticated = false;
unsigned long sessionExpiry = 0;
const unsigned long SESSION_TIMEOUT = 3600000; // 1小时超时

// Web服务器处理函数

// 检查认证状态
bool checkAuth() {
  if (!isAuthenticated || millis() > sessionExpiry) {
    isAuthenticated = false;
    sessionToken = "";
    return false;
  }
  return true;
}

// 生成简单的会话令牌
String generateToken() {
  return String(random(100000, 999999)) + String(millis() % 100000);
}

// 登录页面
void handleLogin() {
  if (server.method() == HTTP_POST) {
    String username = server.arg("username");
    String password = server.arg("password");
    
    // 简单的认证检查 (用户名: admin, 密码: admin)
    if (username == "admin" && password == "admin") {
      isAuthenticated = true;
      sessionToken = generateToken();
      sessionExpiry = millis() + SESSION_TIMEOUT;
      server.sendHeader("Location", "/");
      server.send(302, "text/plain", "Redirect");
      return;
    } else {
      // 认证失败，重新显示登录页面
    }
  }
  
  String html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">";
  html += "<title>登录 - " PROJECT_NAME "</title>";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  html += "<style>";
  html += "body{font-family:'微软雅黑',Arial;margin:0;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;display:flex;align-items:center;justify-content:center}";
  html += ".login-container{background:rgba(255,255,255,0.95);border-radius:15px;padding:40px;box-shadow:0 15px 35px rgba(0,0,0,0.1);width:100%;max-width:400px}";
  html += ".logo{text-align:center;margin-bottom:30px}";
  html += ".logo h1{color:#333;margin:10px 0;font-size:24px}";
  html += ".logo p{color:#666;margin:0;font-size:14px}";
  html += ".form-group{margin:20px 0}";
  html += ".form-label{display:block;margin-bottom:8px;font-weight:bold;color:#333}";
  html += ".form-input{width:100%;padding:12px;border:1px solid #ddd;border-radius:8px;font-size:16px;box-sizing:border-box}";
  html += ".form-input:focus{outline:none;border-color:#007bff;box-shadow:0 0 0 2px rgba(0,123,255,0.25)}";
  html += ".btn{width:100%;padding:12px;border:none;border-radius:8px;font-size:16px;cursor:pointer;transition:all 0.3s}";
  html += ".btn-primary{background:#007bff;color:white}.btn-primary:hover{background:#0056b3}";
  html += ".error{background:#f8d7da;color:#721c24;padding:10px;border-radius:5px;margin:10px 0;display:none}";
  html += ".footer{text-align:center;margin-top:20px;color:#666;font-size:12px}";
  html += "</style></head><body>";
  
  html += "<div class=\"login-container\">";
  html += "<div class=\"logo\">";
  html += "<h1>🏠 联鲸科技</h1>";
  html += "<p>解码: " PROJECT_NAME " 2CH 单片机控制面板 v2.0</p>";
  html += "</div>";
  
  if (server.method() == HTTP_POST) {
    html += "<div class=\"error\" style=\"display:block\">❌ 用户名或密码错误</div>";
  }
  
  html += "<form method=\"POST\" action=\"/login\">";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">用户名:</label>";
  html += "<input type=\"text\" name=\"username\" class=\"form-input\" placeholder=\"admin\" required>";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">密码:</label>";
  html += "<input type=\"password\" name=\"password\" class=\"form-input\" placeholder=\"请输入密码\" required>";
  html += "</div>";
  html += "<button type=\"submit\" class=\"btn btn-primary\">登录系统</button>";
  html += "</form>";
  
  html += "<div class=\"footer\">";
  html += "<p>默认用户名: admin | 默认密码: admin</p>";
  html += "</div>";
  html += "</div>";
  
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

// 注销处理
void handleLogout() {
  isAuthenticated = false;
  sessionToken = "";
  sessionExpiry = 0;
  server.sendHeader("Location", "/login");
  server.send(302, "text/plain", "Redirect");
}

void handleRoot() {
  // 检查认证状态
  if (!checkAuth()) {
    server.sendHeader("Location", "/login");
    server.send(302, "text/plain", "Redirect");
    return;
  }
  
  String html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">";
  html += "<title>" PROJECT_NAME " 控制面板</title>";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  html += "<style>";
  html += "body{font-family:'微软雅黑',Arial;margin:0;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh}";
  html += ".header{background:rgba(255,255,255,0.1);backdrop-filter:blur(10px);padding:20px;text-align:center;color:white}";
  html += ".container{max-width:1200px;margin:20px auto;padding:0 20px}";
  html += ".nav-tabs{display:flex;background:rgba(255,255,255,0.1);border-radius:10px;margin-bottom:20px;overflow:hidden}";
  html += ".nav-tab{flex:1;padding:15px;text-align:center;color:white;cursor:pointer;transition:all 0.3s}";
  html += ".nav-tab.active{background:rgba(255,255,255,0.2)}";
  html += ".nav-tab:hover{background:rgba(255,255,255,0.15)}";
  html += ".tab-content{display:none;background:rgba(255,255,255,0.95);border-radius:15px;padding:30px;box-shadow:0 10px 30px rgba(0,0,0,0.1)}";
  html += ".tab-content.active{display:block}";
  html += ".relay-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:20px;margin-bottom:30px}";
  html += ".relay-card{background:white;border-radius:12px;padding:20px;box-shadow:0 4px 15px rgba(0,0,0,0.1);transition:transform 0.3s}";
  html += ".relay-card:hover{transform:translateY(-5px)}";
  html += ".relay-title{font-size:18px;font-weight:bold;margin-bottom:15px;color:#333}";
  html += ".relay-status{margin:10px 0;padding:8px 12px;border-radius:20px;display:inline-block;font-size:14px}";
  html += ".status-on{background:#d4edda;color:#155724}";
  html += ".status-off{background:#f8d7da;color:#721c24}";
  html += ".btn-group{margin-top:15px}";
  html += ".btn{padding:12px 24px;border:none;border-radius:25px;cursor:pointer;margin:5px;font-size:14px;transition:all 0.3s;min-width:80px}";
  html += ".btn-on{background:#28a745;color:white}.btn-on:hover{background:#218838}";
  html += ".btn-off{background:#dc3545;color:white}.btn-off:hover{background:#c82333}";
  html += ".btn-primary{background:#007bff;color:white}.btn-primary:hover{background:#0069d9}";
  html += ".btn-secondary{background:#6c757d;color:white}.btn-secondary:hover{background:#5a6268}";
  html += ".info-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:20px;margin-bottom:20px}";
  html += ".info-card{background:white;border-radius:12px;padding:20px;box-shadow:0 4px 15px rgba(0,0,0,0.1)}";
  html += ".info-title{font-size:16px;font-weight:bold;color:#333;margin-bottom:15px}";
  html += ".info-item{margin:8px 0;display:flex;justify-content:space-between;align-items:center}";
  html += ".info-label{color:#666;font-size:14px}";
  html += ".info-value{font-weight:bold;color:#333}";
  html += ".protocol-card{background:white;border-radius:12px;padding:20px;margin-bottom:15px;box-shadow:0 4px 15px rgba(0,0,0,0.1)}";
  html += ".protocol-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:10px}";
  html += ".protocol-name{font-size:16px;font-weight:bold;color:#333}";
  html += ".protocol-status{display:flex;align-items:center;gap:10px}";
  html += ".form-group{margin:15px 0}";
  html += ".form-label{display:block;margin-bottom:5px;font-weight:bold;color:#333}";
  html += ".form-input{width:100%;padding:10px;border:1px solid #ddd;border-radius:8px;font-size:14px}";
  html += ".settings-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:20px}";
  html += ".alert{padding:15px;border-radius:8px;margin:10px 0}";
  html += ".alert-warning{background:#fff3cd;color:#856404;border:1px solid #ffeaa7}";
  html += "@media (max-width:768px){.relay-grid{grid-template-columns:1fr}.info-grid{grid-template-columns:1fr}.nav-tab{font-size:14px;padding:12px}}";
  html += "</style></head><body>";
  
  html += "<div class=\"header\">";
  html += "<h1>🏠 " PROJECT_NAME " 智能继电器控制系统</h1>";
  html += "<p>联鲸科技 RelayCtrl-2CH 单片机控制面板 v2.0</p>";
  html += "</div>";
  
  html += "<div class=\"container\">";
  html += "<div class=\"nav-tabs\">";
  html += "<div class=\"nav-tab active\" onclick=\"showTab('control')\">🎛️ 控制面板</div>";
  html += "<div class=\"nav-tab\" onclick=\"showTab('network')\">🌐 网络配置</div>";
  html += "<div class=\"nav-tab\" onclick=\"showTab('system')\">⚙️ 系统信息</div>";
  html += "<div class=\"nav-tab\" onclick=\"logout()\" style=\"background:rgba(220,53,69,0.2)\">🚪 注销</div>";
  html += "</div>";
  
  // 控制面板标签页
  html += "<div id=\"control\" class=\"tab-content active\">";
  html += "<div class=\"relay-grid\">";
  for (int i = 1; i <= 4; i++) {
    html += "<div class=\"relay-card\">";
    html += "<div class=\"relay-title\">继电器 " + String(i) + " (JDQ" + String(i-1) + ")</div>";
    html += "<div id=\"relay" + String(i) + "-status\" class=\"relay-status status-off\">状态未知</div>";
    html += "<div class=\"btn-group\">";
    html += "<button class=\"btn btn-on\" onclick=\"controlRelay(" + String(i) + ", true)\">开启</button>";
    html += "<button class=\"btn btn-off\" onclick=\"controlRelay(" + String(i) + ", false)\">关闭</button>";
    html += "</div>";
    html += "</div>";
  }
  html += "</div>";
  
  html += "<div class=\"protocol-card\">";
  html += "<div class=\"protocol-header\">";
  html += "<div class=\"protocol-name\">🌐 通信协议控制</div>";
  html += "</div>";
  html += "<div class=\"alert alert-warning\">";
  html += "⚠️ 请选择一种通信协议进行配置（同时只能激活一种协议）";
  html += "</div>";
  html += "<div class=\"info-grid\">";
  html += "<div class=\"info-card\">";
  html += "<div class=\"info-title\">📡 MQTT协议</div>";
  html += "<div class=\"protocol-status\">";
  html += "<span id=\"mqttStatusText\" class=\"info-value\">未激活</span>";
  html += "<button id=\"mqttBtn\" class=\"btn btn-secondary\" onclick=\"toggleProtocol('mqtt')\">配置MQTT</button>";
  html += "</div>";
  html += "</div>";
  html += "<div class=\"info-card\">";
  html += "<div class=\"info-title\">🔌 TCP协议</div>";
  html += "<div class=\"protocol-status\">";
  html += "<span id=\"tcpStatusText\" class=\"info-value\">未激活</span>";
  html += "<button id=\"tcpBtn\" class=\"btn btn-secondary\" onclick=\"toggleProtocol('tcp')\">配置TCP</button>";
  html += "</div>";
  html += "</div>";
  html += "<div class=\"info-card\">";
  html += "<div class=\"info-title\">🏭 Modbus TCP</div>";
  html += "<div class=\"protocol-status\">";
  html += "<span id=\"modbusStatusText\" class=\"info-value\">未激活</span>";
  html += "<button id=\"modbusBtn\" class=\"btn btn-secondary\" onclick=\"toggleProtocol('modbusTcp')\">配置Modbus</button>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  
  // 网络配置标签页
  html += "<div id=\"network\" class=\"tab-content\">";
  html += "<div class=\"settings-grid\">";
  html += "<div class=\"info-card\">";
  html += "<div class=\"info-title\">📶 WiFi网络设置</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">WiFi SSID:</label>";
  html += "<span class=\"info-value\" id=\"wifiSSID\">-</span>";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">WiFi 密码:</label>";
  html += "<input type=\"password\" class=\"form-input\" placeholder=\"•••••••••\" readonly>";
  html += "</div>";
  html += "<button class=\"btn btn-primary\" onclick=\"saveWiFiConfig()\">保存WiFi配置</button>";
  html += "</div>";
  
  html += "<div class=\"info-card\">";
  html += "<div class=\"info-title\">📡 MQTT协议配置</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">HTTP服务器地址:</label>";
  html += "<input type=\"text\" id=\"mqttServer\" class=\"form-input\" value=\"\" placeholder=\"192.168.0.145\">";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">端口:</label>";
  html += "<input type=\"number\" id=\"mqttPort\" class=\"form-input\" value=\"80\" placeholder=\"80\">";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">API路径:</label>";
  html += "<input type=\"text\" id=\"mqttTopic\" class=\"form-input\" value=\"\" placeholder=\"/api/device\">";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">API密钥:</label>";
  html += "<input type=\"text\" id=\"mqttApiKey\" class=\"form-input\" value=\"\" placeholder=\"your-api-key\">";
  html += "</div>";
  html += "<button class=\"btn btn-primary\" onclick=\"updateMqttConfig()\">更新MQTT配置</button>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  
  // 系统信息标签页
  html += "<div id=\"system\" class=\"tab-content\">";
  html += "<div class=\"info-grid\">";
  html += "<div class=\"info-card\">";
  html += "<div class=\"info-title\">📋 设备信息</div>";
  html += "<div class=\"info-item\">";
  html += "<span class=\"info-label\">设备ID:</span>";
  html += "<span class=\"info-value\" id=\"deviceId\">-</span>";
  html += "</div>";
  html += "<div class=\"info-item\">";
  html += "<span class=\"info-label\">IP地址:</span>";
  html += "<span class=\"info-value\" id=\"ipAddress\">-</span>";
  html += "</div>";
  html += "<div class=\"info-item\">";
  html += "<span class=\"info-label\">MAC地址:</span>";
  html += "<span class=\"info-value\" id=\"macAddress\">-</span>";
  html += "</div>";
  html += "<div class=\"info-item\">";
  html += "<span class=\"info-label\">固件版本:</span>";
  html += "<span class=\"info-value\">" FIRMWARE_VERSION "</span>";
  html += "</div>";
  html += "</div>";
  
  html += "<div class=\"info-card\">";
  html += "<div class=\"info-title\">💾 系统状态</div>";
  html += "<div class=\"info-item\">";
  html += "<span class=\"info-label\">运行时间:</span>";
  html += "<span class=\"info-value\" id=\"uptime\">-</span>";
  html += "</div>";
  html += "<div class=\"info-item\">";
  html += "<span class=\"info-label\">可用内存:</span>";
  html += "<span class=\"info-value\" id=\"freeHeap\">-</span>";
  html += "</div>";
  html += "<div class=\"info-item\">";
  html += "<span class=\"info-label\">WiFi信号:</span>";
  html += "<span class=\"info-value\" id=\"wifiSignal\">-</span>";
  html += "</div>";
  html += "<div class=\"info-item\">";
  html += "<span class=\"info-label\">MQTT状态:</span>";
  html += "<span class=\"info-value\" id=\"mqttStatus\">-</span>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  
  html += "<div class=\"info-card\">";
  html += "<div class=\"info-title\">🔧 系统操作</div>";
  html += "<div style=\"margin:10px 0\">";
  html += "<button class=\"btn btn-secondary\" onclick=\"refreshStatus()\">🔄 刷新状态</button>";
  html += "<button class=\"btn btn-primary\" onclick=\"exportConfig()\">📥 导出配置</button>";
  html += "<button class=\"btn btn-off\" onclick=\"restartDevice()\">🔄 重启设备</button>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  
  html += "</div>"; // container end
  html += "<label>Modbus TCP (Port 502): </label>";
  html += "<button id=\"modbusTcpBtn\" class=\"btn\" onclick=\"toggleProtocol('modbusTcp')\">-</button>";
  html += "<span id=\"modbusTcpStatusText\" style=\"margin-left:10px\">-</span>";
  html += "</div>";
  html += "</div>";
  
  
  // JavaScript
  html += "<script>";
  html += "function showTab(tabName){";
  html += "document.querySelectorAll('.tab-content').forEach(tab=>tab.classList.remove('active'));";
  html += "document.querySelectorAll('.nav-tab').forEach(tab=>tab.classList.remove('active'));";
  html += "document.getElementById(tabName).classList.add('active');";
  html += "event.target.classList.add('active');";
  html += "}";
  
  html += "function controlRelay(relay,state){";
  html += "fetch('/api/relay',{method:'POST',headers:{'Content-Type':'application/json'},";
  html += "body:JSON.stringify({relay:relay,state:state})})";
  html += ".then(response=>{if(!response.ok)throw new Error('HTTP '+response.status);return response.json();})";
  html += ".then(data=>{if(data.success){updateRelayStatus(relay,state);}else{alert('错误: '+data.message);}})";
  html += ".catch(error=>{console.error('Control error:',error);alert('网络错误: '+error.message);});";
  html += "}";
  
  html += "function updateRelayStatus(relay,state){";
  html += "const elem=document.getElementById('relay'+relay+'-status');";
  html += "elem.textContent=state?'✅ 已开启':'❌ 已关闭';";
  html += "elem.className='relay-status '+(state?'status-on':'status-off');";
  html += "}";
  
  html += "function updateMqttConfig(){";
  html += "const server=document.getElementById('mqttServer').value;";
  html += "const port=document.getElementById('mqttPort').value;";
  html += "const topic=document.getElementById('mqttTopic').value;";
  html += "const apiKey=document.getElementById('mqttApiKey').value;";
  html += "fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},";
  html += "body:JSON.stringify({mqttServer:server,mqttPort:parseInt(port),mqttTopic:topic,mqttApiKey:apiKey})})";
  html += ".then(response=>response.json())";
  html += ".then(data=>{alert(data.message||'配置已更新');if(data.success)refreshStatus();})";
  html += ".catch(error=>alert('配置更新失败: '+error.message));";
  html += "}";
  
  html += "function refreshStatus(){";
  html += "fetch('/api/status').then(response=>{";
  html += "if(!response.ok)throw new Error('HTTP '+response.status);return response.json();";
  html += "}).then(data=>{";
  html += "for(let i=1;i<=4;i++){updateRelayStatus(i,data.relays[i-1]);}";
  html += "document.getElementById('deviceId').textContent=data.deviceId;";
  html += "document.getElementById('ipAddress').textContent=data.ip;";
  html += "document.getElementById('wifiSSID').textContent=data.wifi;";
  html += "document.getElementById('macAddress').textContent=data.mac||'-';";
  html += "document.getElementById('uptime').textContent=Math.floor(data.uptime/60)+' 分钟';";
  html += "document.getElementById('freeHeap').textContent=Math.round(data.freeHeap/1024)+' KB';";
  html += "document.getElementById('wifiSignal').textContent=data.rssi+' dBm';";
  html += "document.getElementById('mqttStatus').textContent=data.mqttConnected?'✅ 已连接':'❌ 未连接';";
  html += "updateProtocolStatus('mqtt',data.mqttEnabled);";
  html += "updateProtocolStatus('tcp',data.tcpEnabled);";
  html += "updateProtocolStatus('modbusTcp',data.modbusTcpEnabled);";
  html += "if(data.mqttServer)document.getElementById('mqttServer').value=data.mqttServer;";
  html += "if(data.mqttPort)document.getElementById('mqttPort').value=data.mqttPort;";
  html += "}).catch(error=>{console.error('Status error:',error);";
  html += "document.getElementById('deviceId').textContent='错误: '+error.message;});";
  html += "}";
  
  html += "function updateProtocolStatus(protocol,enabled){";
  html += "const btn=document.getElementById(protocol+'Btn');";
  html += "const text=document.getElementById(protocol+'StatusText');";
  html += "if(btn){";
  html += "btn.textContent=enabled?'禁用':'启用';";
  html += "btn.className='btn '+(enabled?'btn-off':'btn-primary');";
  html += "}";
  html += "if(text){";
  html += "text.textContent=enabled?'✅ 已激活':'❌ 未激活';";
  html += "text.style.color=enabled?'#28a745':'#6c757d';";
  html += "}";
  html += "}";
  
  html += "function toggleProtocol(protocol){";
  html += "const btn=document.getElementById(protocol+'Btn');";
  html += "const isEnabled=btn.textContent==='禁用';";
  html += "fetch('/api/protocol',{method:'POST',headers:{'Content-Type':'application/json'},";
  html += "body:JSON.stringify({protocol:protocol,enabled:!isEnabled})})";
  html += ".then(response=>{if(!response.ok)throw new Error('HTTP '+response.status);return response.json();})";
  html += ".then(data=>{if(data.success){updateProtocolStatus(protocol,!isEnabled);alert(data.message);}else{alert('错误: '+data.message);}})";
  html += ".catch(error=>{console.error('Protocol control error:',error);alert('错误: '+error.message);});";
  html += "}";
  
  html += "function restartDevice(){";
  html += "if(confirm('确定要重启设备吗？')){";
  html += "fetch('/api/restart',{method:'POST'})";
  html += ".then(response=>response.json())";
  html += ".then(data=>{alert(data.message);setTimeout(()=>location.reload(),5000);})";
  html += ".catch(error=>alert('重启命令发送失败'));";
  html += "}";
  html += "}";
  
  html += "function exportConfig(){";
  html += "fetch('/api/config').then(response=>response.json())";
  html += ".then(data=>{";
  html += "const config=JSON.stringify(data,null,2);";
  html += "const blob=new Blob([config],{type:'application/json'});";
  html += "const url=URL.createObjectURL(blob);";
  html += "const a=document.createElement('a');";
  html += "a.href=url;a.download='relay-config.json';a.click();";
  html += "URL.revokeObjectURL(url);";
  html += "});";
  html += "}";
  
  html += "function logout(){";
  html += "if(confirm('确定要注销登录吗？')){";
  html += "fetch('/logout',{method:'POST'})";
  html += ".then(()=>window.location.href='/login')";
  html += ".catch(()=>window.location.href='/login');";
  html += "}";
  html += "}";
  
  html += "document.addEventListener('DOMContentLoaded',()=>{refreshStatus();setInterval(refreshStatus,10000);});";
  html += "</script></body></html>";
  
  server.send(200, "text/html", html);
}

void handleStatus() {
  // 使用更小的JSON文档并预分配内存
  StaticJsonDocument<640> doc; // 增加大小以容纳协议状态
  
  doc["success"] = true;
  doc["deviceId"] = config.deviceId;
  doc["ip"] = WiFi.localIP().toString();
  doc["wifi"] = WiFi.SSID();
  doc["uptime"] = millis() / 1000;
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["mqttConnected"] = mqttClient.connected();
  
  // 添加协议启用状态
  doc["mqttEnabled"] = config.mqttEnabled;
  doc["tcpEnabled"] = config.tcpEnabled;
  doc["modbusTcpEnabled"] = config.modbusTcpEnabled;
  
  JsonArray relays = doc.createNestedArray("relays");
  for (int i = 0; i < 4; i++) {
    relays.add(relayStates[i]);
  }
  
  String response;
  serializeJson(doc, response);
  
  // 添加缓存控制头，减少频繁请求
  server.sendHeader("Cache-Control", "no-cache, max-age=1");
  server.send(200, "application/json", response);
}

void handleRelayControl() {
  if (server.hasArg("plain")) {
    StaticJsonDocument<128> doc; // 使用静态文档
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    
    if (error) {
      server.send(400, "application/json", "{\"success\":false,\"message\":\"JSON解析错误\"}");
      return;
    }
    
    int relay = doc["relay"];
    bool state = doc["state"];
    
    if (relay >= 1 && relay <= 4) {
      setRelay(relay - 1, state);
      
      // 简化响应，减少处理时间
      String response = "{\"success\":true,\"relay\":" + String(relay) + ",\"state\":" + (state ? "true" : "false") + "}";
      server.send(200, "application/json", response);
    } else {
      server.send(400, "application/json", "{\"success\":false,\"message\":\"无效的继电器编号\"}");
    }
  } else {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"缺少请求数据\"}");
  }
}

void handleGetConfig() {
  String json = "{";
  json += "\"deviceId\":\"" + String(config.deviceId) + "\",";
  json += "\"mqttEnabled\":" + String(config.mqttEnabled ? "true" : "false") + ",";
  json += "\"mqttServer\":\"" + String(config.mqttServer) + "\",";
  json += "\"mqttPort\":" + String(config.mqttPort) + ",";
  json += "\"mqttTopic\":\"" + String(config.mqttTopic) + "\",";
  json += "\"mqttApiKey\":\"" + String(config.mqttApiKey) + "\",";
  json += "\"tcpEnabled\":" + String(config.tcpEnabled ? "true" : "false") + ",";
  json += "\"modbusTcpEnabled\":" + String(config.modbusTcpEnabled ? "true" : "false") + ",";
  json += "\"firmwareVersion\":\"" FIRMWARE_VERSION "\",";
  json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"uptime\":" + String(millis() / 1000);
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleSetConfig() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
      server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
      return;
    }
    
    bool changed = false;
    
    // 检查MQTT服务器配置
    if (doc.containsKey("mqttServer")) {
      String newServer = doc["mqttServer"];
      if (newServer != String(config.mqttServer)) {
        strncpy(config.mqttServer, newServer.c_str(), sizeof(config.mqttServer) - 1);
        config.mqttServer[sizeof(config.mqttServer) - 1] = '\0';
        changed = true;
      }
    }
    
    // 检查MQTT端口配置
    if (doc.containsKey("mqttPort")) {
      int newPort = doc["mqttPort"];
      if (newPort != config.mqttPort && newPort > 0 && newPort <= 65535) {
        config.mqttPort = newPort;
        changed = true;
      }
    }
    
    // 检查MQTT主题配置
    if (doc.containsKey("mqttTopic")) {
      String newTopic = doc["mqttTopic"];
      if (newTopic != String(config.mqttTopic)) {
        strncpy(config.mqttTopic, newTopic.c_str(), sizeof(config.mqttTopic) - 1);
        config.mqttTopic[sizeof(config.mqttTopic) - 1] = '\0';
        changed = true;
      }
    }
    
    // 检查API密钥配置
    if (doc.containsKey("mqttApiKey")) {
      String newApiKey = doc["mqttApiKey"];
      if (newApiKey != String(config.mqttApiKey)) {
        strncpy(config.mqttApiKey, newApiKey.c_str(), sizeof(config.mqttApiKey) - 1);
        config.mqttApiKey[sizeof(config.mqttApiKey) - 1] = '\0';
        changed = true;
      }
    }
    
    // 检查MQTT启用状态
    if (doc.containsKey("mqttEnabled")) {
      bool newEnabled = doc["mqttEnabled"];
      if (newEnabled != config.mqttEnabled) {
        config.mqttEnabled = newEnabled;
        if (config.mqttEnabled) {
          reconnectMQTT();  // 使用现有的重连函数
        } else {
          mqttClient.disconnect();
        }
        changed = true;
      }
    }
    
    if (changed) {
      // 保存配置到EEPROM (如果需要)
      Serial.println("Configuration updated");
      server.send(200, "application/json", "{\"success\":true,\"message\":\"Configuration updated successfully\"}");
    } else {
      server.send(200, "application/json", "{\"success\":true,\"message\":\"No changes made\"}");
    }
  } else {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Missing request data\"}");
  }
}

void handleRestart() {
  server.send(200, "application/json", "{\"success\":true,\"message\":\"设备将在3秒后重启\"}");
  delay(3000);
  ESP.restart();
}

void handleProtocolControl() {
  if (server.hasArg("plain")) {
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    
    if (error) {
      server.send(400, "application/json", "{\"success\":false,\"message\":\"JSON解析错误\"}");
      return;
    }
    
    String protocol = doc["protocol"];
    bool enabled = doc["enabled"];
    
    Serial.print("Protocol control: ");
    Serial.print(protocol);
    Serial.print(" -> ");
    Serial.println(enabled ? "Enable" : "Disable");
    
    if (protocol == "mqtt") {
      enableMQTT(enabled);
      server.send(200, "application/json", "{\"success\":true,\"message\":\"MQTT设置已更新\"}");
    } else if (protocol == "tcp") {
      enableTCP(enabled);
      server.send(200, "application/json", "{\"success\":true,\"message\":\"TCP setting updated\"}");
    } else if (protocol == "modbusTcp") {
      enableModbusTCP(enabled);
      server.send(200, "application/json", "{\"success\":true,\"message\":\"Modbus TCP setting updated\"}");
    } else {
      server.send(400, "application/json", "{\"success\":false,\"message\":\"Unknown protocol type\"}");
    }
  } else {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Missing request data\"}");
  }
}

void handleConfigPage() {
  String html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">";
  html += "<title>" PROJECT_NAME " Configuration</title>";
  html += "<style>";
  html += "body{font-family:Arial;margin:20px;background:#f0f0f0}";
  html += ".container{max-width:800px;margin:0 auto;background:white;padding:20px;border-radius:10px}";
  html += ".config-section{border:1px solid #ddd;border-radius:8px;padding:15px;margin:15px 0}";
  html += ".form-group{margin:10px 0}";
  html += "label{display:inline-block;width:150px;font-weight:bold}";
  html += "input,select{padding:8px;width:200px;border:1px solid #ddd;border-radius:4px}";
  html += ".btn{padding:10px 20px;border:none;border-radius:5px;cursor:pointer;margin:5px}";
  html += ".btn-primary{background:#007bff;color:white}";
  html += ".btn-success{background:#28a745;color:white}";
  html += ".btn-warning{background:#ffc107;color:black}";
  html += ".status{padding:10px;border-radius:5px;margin:10px 0}";
  html += ".status-on{background:#d4edda;color:#155724}";
  html += ".status-off{background:#f8d7da;color:#721c24}";
  html += "</style></head><body>";
  html += "<div class=\"container\">";
  html += "<h1>" PROJECT_NAME " Configuration</h1>";
  html += "<a href=\"/\" style=\"color:#007bff;text-decoration:none\">← Back to Main</a>";
  
  // WiFi配置
  html += "<div class=\"config-section\">";
  html += "<h3>WiFi Configuration</h3>";
  html += "<div class=\"form-group\">";
  html += "<label>Current SSID:</label>";
  html += "<span>" + WiFi.SSID() + "</span>";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label>IP Address:</label>";
  html += "<span>" + WiFi.localIP().toString() + "</span>";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label>Signal Strength:</label>";
  html += "<span>" + String(WiFi.RSSI()) + " dBm</span>";
  html += "</div>";
  html += "</div>";
  
  // MQTT配置
  html += "<div class=\"config-section\">";
  html += "<h3>MQTT Configuration</h3>";
  html += "<div class=\"form-group\">";
  html += "<label>Status:</label>";
  html += "<span class=\"status " + String(config.mqttEnabled ? "status-on" : "status-off") + "\">";
  html += config.mqttEnabled ? "Enabled" : "Disabled";
  html += "</span>";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label>Server:</label>";
  html += "<input type=\"text\" id=\"mqttServer\" value=\"" + String(config.mqttServer) + "\">";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label>Port:</label>";
  html += "<input type=\"number\" id=\"mqttPort\" value=\"" + String(config.mqttPort) + "\">";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label>Client ID:</label>";
  html += "<span>" + String(config.deviceId) + "</span>";
  html += "</div>";
  html += "<button class=\"btn btn-primary\" onclick=\"updateMqttConfig()\">Update MQTT</button>";
  html += "</div>";
  
  // TCP服务器配置
  html += "<div class=\"config-section\">";
  html += "<h3>TCP Servers Configuration</h3>";
  html += "<div class=\"form-group\">";
  html += "<label>Raw TCP (" + String(RAW_TCP_PORT) + "):</label>";
  html += "<span class=\"status " + String(config.tcpEnabled ? "status-on" : "status-off") + "\">";
  html += config.tcpEnabled ? "Enabled" : "Disabled";
  html += "</span>";
  html += "<button class=\"btn " + String(config.tcpEnabled ? "btn-warning" : "btn-success") + "\" onclick=\"toggleProtocol('tcp')\">";
  html += config.tcpEnabled ? "Disable" : "Enable";
  html += "</button>";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label>Modbus TCP (" + String(MODBUS_TCP_PORT) + "):</label>";
  html += "<span class=\"status " + String(config.modbusTcpEnabled ? "status-on" : "status-off") + "\">";
  html += config.modbusTcpEnabled ? "Enabled" : "Disabled";
  html += "</span>";
  html += "<button class=\"btn " + String(config.modbusTcpEnabled ? "btn-warning" : "btn-success") + "\" onclick=\"toggleProtocol('modbusTcp')\">";
  html += config.modbusTcpEnabled ? "Disable" : "Enable";
  html += "</button>";
  html += "</div>";
  html += "</div>";
  
  // 系统信息
  html += "<div class=\"config-section\">";
  html += "<h3>System Information</h3>";
  html += "<div class=\"form-group\">";
  html += "<label>Device ID:</label>";
  html += "<span>" + String(config.deviceId) + "</span>";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label>MAC Address:</label>";
  html += "<span>" + WiFi.macAddress() + "</span>";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label>Firmware Version:</label>";
  html += "<span>" FIRMWARE_VERSION "</span>";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label>Free Memory:</label>";
  html += "<span id=\"freeHeap\">" + String(ESP.getFreeHeap()) + " bytes</span>";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label>Uptime:</label>";
  html += "<span id=\"uptime\">" + String(millis() / 1000) + " seconds</span>";
  html += "</div>";
  html += "</div>";
  
  // JavaScript
  html += "<script>";
  html += "function updateMqttConfig() {";
  html += "  const server = document.getElementById('mqttServer').value;";
  html += "  const port = document.getElementById('mqttPort').value;";
  html += "  fetch('/api/config', {";
  html += "    method: 'POST',";
  html += "    headers: {'Content-Type': 'application/json'},";
  html += "    body: JSON.stringify({mqttServer: server, mqttPort: parseInt(port)})";
  html += "  }).then(response => response.json())";
  html += "    .then(data => {";
  html += "      alert(data.message || 'Configuration updated');";
  html += "      if(data.success) location.reload();";
  html += "    });";
  html += "}";
  html += "function toggleProtocol(protocol) {";
  html += "  fetch('/api/protocol', {";
  html += "    method: 'POST',";
  html += "    headers: {'Content-Type': 'application/json'},";
  html += "    body: JSON.stringify({protocol: protocol, enabled: !getCurrentState(protocol)})";
  html += "  }).then(response => response.json())";
  html += "    .then(data => {";
  html += "      alert(data.message || 'Protocol setting updated');";
  html += "      if(data.success) location.reload();";
  html += "    });";
  html += "}";
  html += "function getCurrentState(protocol) {";
  html += "  if(protocol === 'tcp') return " + String(config.tcpEnabled ? "true" : "false") + ";";
  html += "  if(protocol === 'modbusTcp') return " + String(config.modbusTcpEnabled ? "true" : "false") + ";";
  html += "  return false;";
  html += "}";
  html += "</script>";
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}
