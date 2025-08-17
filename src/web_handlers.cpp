#include "config.h"
#include "relay_controller.h"

// 将CSS样式存储在Flash中以节省RAM
const char CSS_STYLES[] PROGMEM = R"(
body{font-family:'Roboto',Arial,sans-serif;margin:0;background:#f4f4f4;color:#333}
.header{background:#305680;color:white;padding:15px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}
.header h1{margin:0;font-size:24px;font-weight:300;text-align:center}
.nav-bar{background:white;padding:10px 20px;margin-bottom:20px;border-radius:4px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}
.nav-bar a{color:#305680;text-decoration:none;margin-right:20px;font-weight:500}
.nav-bar a.active{color:#1976d2;font-weight:600}
.nav-bar a:hover{color:#1976d2}
.container{max-width:1200px;margin:0 auto;padding:20px}
.dashboard-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:20px;margin:20px 0}
.card{background:white;border-radius:8px;padding:20px;box-shadow:0 2px 8px rgba(0,0,0,0.1)}
.card-title{color:#305680;font-size:18px;font-weight:500;margin:0 0 15px 0;padding-bottom:8px;border-bottom:2px solid #e0e0e0}
.relay-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:15px}
.relay-card{background:#f8f9fa;border-radius:8px;padding:20px;border-left:5px solid #305680;text-align:center}
.relay-title{font-size:16px;font-weight:500;margin-bottom:10px;color:#305680}
.relay-status{font-size:14px;margin:10px 0;padding:8px;border-radius:4px;font-weight:500}
.status-on{background:#e8f5e8;color:#2e7d32;border:1px solid #4caf50}
.status-off{background:#fde7e7;color:#c62828;border:1px solid #f44336}
.btn{padding:12px 24px;border:none;border-radius:4px;cursor:pointer;font-size:14px;font-weight:500;margin:5px;transition:all 0.3s;min-width:80px}
.btn-on{background:#4caf50;color:white}.btn-on:hover{background:#45a049}
.btn-off{background:#f44336;color:white}.btn-off:hover{background:#d32f2f}
.btn-primary{background:#305680;color:white}.btn-primary:hover{background:#1976d2}
.btn-success{background:#4caf50;color:white}.btn-success:hover{background:#45a049}
.btn-danger{background:#f44336;color:white}.btn-danger:hover{background:#d32f2f}
.btn-group{display:flex;gap:10px;justify-content:center}
.info-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:15px}
.info-item{background:#f8f9fa;padding:15px;border-radius:4px;border-left:4px solid #305680}
.info-label{font-size:12px;color:#666;text-transform:uppercase;margin-bottom:5px}
.info-value{font-size:16px;font-weight:500;color:#305680}
.protocol-controls{margin-top:20px}
.protocol-item{display:flex;align-items:center;justify-content:space-between;padding:10px 0;border-bottom:1px solid #e0e0e0}
.protocol-item:last-child{border-bottom:none}
.protocol-status{font-size:14px;font-weight:500}
.status-running{color:#4caf50}
.status-stopped{color:#f44336}
.control-group{display:flex;gap:10px;margin-top:20px;justify-content:center}
.switch-container{display:flex;align-items:center;margin-bottom:15px}
.switch{position:relative;display:inline-block;width:60px;height:34px;margin-right:15px}
.switch input{opacity:0;width:0;height:0}
.slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background:#ccc;transition:.4s;border-radius:34px}
.slider:before{position:absolute;content:"";height:26px;width:26px;left:4px;bottom:4px;background:white;transition:.4s;border-radius:50%}
input:checked+.slider{background:#4caf50}
input:checked+.slider:before{transform:translateX(26px)}
.form-group{margin-bottom:15px}
.form-label{display:block;font-weight:500;margin-bottom:8px;color:#555;font-size:14px}
.form-input{width:100%;padding:10px;border:1px solid #ddd;border-radius:4px;font-size:14px}
.form-input:focus{outline:none;border-color:#305680}
.input-group{margin-left:20px;margin-top:8px}
.input-group .form-label{margin-bottom:4px;font-size:13px;color:#666}
.input-group .form-input{padding:8px;font-size:13px}
)";
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

// 外部声明的全局对象
extern ESP8266WebServer server;
extern ESP8266HTTPUpdateServer httpUpdater;

// Web服务器处理函数

void handleRoot() {
  // 检查认证
  if (!checkAuthentication()) {
    return;
  }
  
  String html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">";
  html += "<title>" PROJECT_NAME " 控制面板</title>";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<style>";
  // 使用PROGMEM中的CSS样式以节省RAM
  html += FPSTR(CSS_STYLES);
  html += "</style></head>";
  
  html += "<body>";
  html += "<div class=\"header\"><h1>" PROJECT_NAME " 继电器控制器</h1></div>";
  html += "<div class=\"nav-bar\">";
  html += "<a href=\"/\" class=\"active\">控制面板</a>";
  html += "<a href=\"/config\">系统配置</a>";
  html += "</div>";
  
  html += "<div class=\"container\">";
  
  // 继电器控制卡片
  html += "<div class=\"card\">";
  html += "<div class=\"card-title\">继电器控制</div>";
  html += "<div class=\"relay-grid\">";
  
  for (int i = 1; i <= 4; i++) {
    html += "<div class=\"relay-card\">";
    html += "<div class=\"relay-title\">继电器 " + String(i) + " (JDQ" + String(i-1) + ")</div>";
    html += "<div id=\"relay" + String(i) + "-status\" class=\"relay-status\">状态: 未知</div>";
    html += "<div class=\"btn-group\">";
    html += "<button class=\"btn btn-on\" onclick=\"controlRelay(" + String(i) + ", true)\">开启</button>";
    html += "<button class=\"btn btn-off\" onclick=\"controlRelay(" + String(i) + ", false)\">关闭</button>";
    html += "</div></div>";
  }
  
  html += "</div></div>";
  
  // 系统信息卡片
  html += "<div class=\"dashboard-grid\">";
  html += "<div class=\"card\">";
  html += "<div class=\"card-title\">系统信息</div>";
  html += "<div class=\"info-grid\">";
  html += "<div class=\"info-item\"><div class=\"info-label\">设备ID</div><div class=\"info-value\" id=\"deviceId\">-</div></div>";
  html += "<div class=\"info-item\"><div class=\"info-label\">IP地址</div><div class=\"info-value\" id=\"ipAddress\">-</div></div>";
  html += "<div class=\"info-item\"><div class=\"info-label\">WiFi网络</div><div class=\"info-value\" id=\"wifiSSID\">-</div></div>";
  html += "<div class=\"info-item\"><div class=\"info-label\">运行时间</div><div class=\"info-value\" id=\"uptime\">-</div></div>";
  html += "<div class=\"info-item\"><div class=\"info-label\">可用内存</div><div class=\"info-value\" id=\"freeHeap\">-</div></div>";
  html += "<div class=\"info-item\"><div class=\"info-label\">MQTT状态</div><div class=\"info-value\" id=\"mqttStatus\">-</div></div>";
  html += "</div></div>";
  
  // 协议控制卡片
  html += "<div class=\"card\">";
  html += "<div class=\"card-title\">网络协议</div>";
  html += "<div class=\"protocol-controls\">";
  html += "<div class=\"protocol-item\">";
  html += "<span>MQTT 协议</span>";
  html += "<div><span id=\"mqttStatusText\" class=\"protocol-status\">-</span>";
  html += "<button id=\"mqttBtn\" class=\"btn btn-primary\" onclick=\"toggleProtocol('mqtt')\">-</button></div>";
  html += "</div>";
  html += "<div class=\"protocol-item\">";
  html += "<span>原始TCP (端口 " + String(config.rawTcpPort) + ")</span>";
  html += "<div><span id=\"tcpStatusText\" class=\"protocol-status\">-</span>";
  html += "<button id=\"tcpBtn\" class=\"btn btn-primary\" onclick=\"toggleProtocol('tcp')\">-</button></div>";
  html += "</div>";
  html += "<div class=\"protocol-item\">";
  html += "<span>Modbus TCP (端口 " + String(config.modbusTcpPort) + ")</span>";
  html += "<div><span id=\"modbusTcpStatusText\" class=\"protocol-status\">-</span>";
  html += "<button id=\"modbusTcpBtn\" class=\"btn btn-primary\" onclick=\"toggleProtocol('modbusTcp')\">-</button></div>";
  html += "</div></div></div>";
  html += "</div>";
  
  // 控制按钮
  html += "<div class=\"control-group\">";
  html += "<button class=\"btn btn-primary\" onclick=\"refreshStatus()\">刷新状态</button>";
  html += "</div></div>";
  
  // JavaScript
  html += "<script>";
  html += "function controlRelay(relay,state){";
  html += "fetch('/api/relay',{method:'POST',headers:{'Content-Type':'application/json'},";
  html += "body:JSON.stringify({relay:relay,state:state})})";
  html += ".then(response=>{if(!response.ok)throw new Error('HTTP '+response.status);return response.json();})";
  html += ".then(data=>{if(data.success){updateRelayStatus(relay,state);}else{alert('Error: '+data.message);}})";
  html += ".catch(error=>{console.error('Control error:',error);alert('Network error: '+error.message);});";
  html += "}";
  
  html += "function updateRelayStatus(relay,state){";
  html += "const elem=document.getElementById('relay'+relay+'-status');";
  html += "elem.textContent='状态: '+(state?'开启':'关闭');";
  html += "elem.className='relay-status '+(state?'status-on':'status-off');";
  html += "}";
  
  html += "function refreshStatus(){";
  html += "fetch('/api/status').then(response=>{";
  html += "if(!response.ok)throw new Error('HTTP '+response.status);return response.json();";
  html += "}).then(data=>{";
  html += "for(let i=1;i<=4;i++){updateRelayStatus(i,data.relays[i-1]);}";
  html += "document.getElementById('deviceId').textContent=data.deviceId;";
  html += "document.getElementById('ipAddress').textContent=data.ip;";
  html += "document.getElementById('wifiSSID').textContent=data.wifi;";
  html += "document.getElementById('uptime').textContent=Math.floor(data.uptime/60)+' min';";
  html += "document.getElementById('freeHeap').textContent=data.freeHeap+' bytes';";
  html += "document.getElementById('mqttStatus').textContent=data.mqttConnected?'已连接':'未连接';";
  html += "updateProtocolStatus('mqtt',data.mqttEnabled);";
  html += "updateProtocolStatus('tcp',data.tcpEnabled);";
  html += "updateProtocolStatus('modbusTcp',data.modbusTcpEnabled);";
  html += "}).catch(error=>{console.error('Status error:',error);";
  html += "document.getElementById('deviceId').textContent='Error: '+error.message;});";
  html += "}";
  
  html += "function updateProtocolStatus(protocol,enabled){";
  html += "const btn=document.getElementById(protocol+'Btn');";
  html += "const text=document.getElementById(protocol+'StatusText');";
  html += "btn.textContent=enabled?'禁用':'启用';";
  html += "btn.className='btn '+(enabled?'btn-off':'btn-on');";
  html += "text.textContent=enabled?'运行中':'已停止';";
  html += "text.className='protocol-status '+(enabled?'status-running':'status-stopped');";
  html += "}";
  
  html += "function toggleProtocol(protocol){";
  html += "const btn=document.getElementById(protocol+'Btn');";
  html += "const isEnabled=btn.textContent==='禁用';";
  html += "fetch('/api/protocol',{method:'POST',headers:{'Content-Type':'application/json'},";
  html += "body:JSON.stringify({protocol:protocol,enabled:!isEnabled})})";
  html += ".then(response=>{if(!response.ok)throw new Error('HTTP '+response.status);return response.json();})";
  html += ".then(data=>{if(data.success){updateProtocolStatus(protocol,!isEnabled);}else{alert('Error: '+data.message);}})";
  html += ".catch(error=>{console.error('Protocol control error:',error);alert('Error: '+error.message);});";
  html += "}";
  
  html += "document.addEventListener('DOMContentLoaded',refreshStatus);";
  html += "setInterval(refreshStatus,5000);";
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
  doc["mqttEnabled"] = config.mqttEnabled;
  doc["tcpEnabled"] = config.tcpEnabled;
  doc["modbusTcpEnabled"] = config.modbusTcpEnabled;
  
  JsonArray relays = doc.createNestedArray("relays");
  for (int i = 0; i < 4; i++) {
    relays.add(relayStates[i]);
  }
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleRelayControl() {
  // 检查认证
  if (!checkAuthentication()) {
    return;
  }
  
  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"success\":false,\"message\":\"Method not allowed\"}");
    return;
  }
  
  StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  
  if (error) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
    return;
  }
  
  int relay = doc["relay"];
  bool state = doc["state"];
  
  if (relay < 1 || relay > 4) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid relay number\"}");
    return;
  }
  
  setRelay(relay - 1, state);
  
  StaticJsonDocument<128> response;
  response["success"] = true;
  response["relay"] = relay;
  response["state"] = state;
  response["message"] = "Relay control successful";
  
  String jsonString;
  serializeJson(response, jsonString);
  server.send(200, "application/json", jsonString);
}

void handleProtocolControl() {
  // 检查认证
  if (!checkAuthentication()) {
    return;
  }
  
  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"success\":false,\"message\":\"Method not allowed\"}");
    return;
  }
  
  StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  
  if (error) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
    return;
  }
  
  String protocol = doc["protocol"].as<String>();
  bool enabled = doc["enabled"];
  
  if (protocol == "mqtt") {
    config.mqttEnabled = enabled;
    if (!enabled && mqttClient.connected()) {
      mqttClient.disconnect();
    }
  } else if (protocol == "tcp") {
    config.tcpEnabled = enabled;
  } else if (protocol == "modbusTcp") {
    config.modbusTcpEnabled = enabled;
  } else {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid protocol\"}");
    return;
  }
  
  saveConfig();
  
  StaticJsonDocument<128> response;
  response["success"] = true;
  response["protocol"] = protocol;
  response["enabled"] = enabled;
  response["message"] = "Protocol control successful";
  
  String jsonString;
  serializeJson(response, jsonString);
  server.send(200, "application/json", jsonString);
}

void handleConfigPage() {
  // 检查认证
  if (!checkAuthentication()) {
    return;
  }
  
  String html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">";
  html += "<title>" PROJECT_NAME " 系统配置</title>";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<style>";
  // 使用PROGMEM中的CSS样式以节省RAM
  html += FPSTR(CSS_STYLES);
  html += "</style></head>";
  
  html += "<body>";
  html += "<div class=\"header\"><h1>" PROJECT_NAME " 系统配置</h1></div>";
  html += "<div class=\"nav-bar\">";
  html += "<a href=\"/\">控制面板</a>";
  html += "<a href=\"/config\" class=\"active\">系统配置</a>";
  html += "</div>";
  
  html += "<div class=\"container\">";
  html += "<div class=\"dashboard-grid\">";
  
  // WiFi 配置卡片
  html += "<div class=\"card\">";
  html += "<div class=\"card-title\">WiFi 网络配置</div>";
  html += "<form method=\"POST\" action=\"/saveWiFi\">";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">网络名称</label>";
  html += "<input type=\"text\" name=\"ssid\" class=\"form-input\" value=\"" + String(config.ssid) + "\" maxlength=\"32\">";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">密码</label>";
  html += "<input type=\"password\" name=\"password\" class=\"form-input\" value=\"" + String(config.password) + "\" maxlength=\"64\">";
  html += "</div>";
  html += "<div class=\"btn-group\">";
  html += "<button type=\"submit\" class=\"btn btn-primary\">更新WiFi配置</button>";
  html += "</div>";
  html += "</form>";
  html += "</div>";
  
  // MQTT 配置卡片
  html += "<div class=\"card\">";
  html += "<div class=\"card-title\">MQTT 协议配置</div>";
  html += "<form method=\"POST\" action=\"/saveMqtt\">";
  html += "<div class=\"form-group\">";
  html += "<div class=\"switch-container\">";
  html += "<label class=\"switch\">";
  html += "<input type=\"checkbox\" name=\"mqttEnabled\" " + String(config.mqttEnabled ? "checked" : "") + ">";
  html += "<span class=\"slider\"></span>";
  html += "</label>";
  html += "<label class=\"form-label\">启用 MQTT</label>";
  html += "</div>";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">MQTT 代理服务器</label>";
  html += "<input type=\"text\" name=\"mqttServer\" class=\"form-input\" value=\"" + String(config.mqttServer) + "\" maxlength=\"128\">";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">端口</label>";
  html += "<input type=\"number\" name=\"mqttPort\" class=\"form-input\" value=\"" + String(config.mqttPort) + "\" min=\"1\" max=\"65535\">";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">主题前缀</label>";
  html += "<input type=\"text\" name=\"mqttTopic\" class=\"form-input\" value=\"" + String(config.mqttTopic) + "\" maxlength=\"64\">";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">用户名</label>";
  html += "<input type=\"text\" name=\"mqttUsername\" class=\"form-input\" value=\"" + String(config.mqttUsername) + "\" maxlength=\"64\">";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">密码</label>";
  html += "<input type=\"password\" name=\"mqttPassword\" class=\"form-input\" value=\"" + String(config.mqttPassword) + "\" maxlength=\"64\">";
  html += "</div>";
  html += "<div class=\"btn-group\">";
  html += "<button type=\"submit\" class=\"btn btn-success\">更新MQTT配置</button>";
  html += "</div>";
  html += "</form>";
  html += "</div>";
  
  // Web 认证配置卡片
  html += "<div class=\"card\">";
  html += "<div class=\"card-title\">Web 访问认证</div>";
  html += "<form method=\"POST\" action=\"/saveAuth\">";
  html += "<div class=\"form-group\">";
  html += "<div class=\"switch-container\">";
  html += "<label class=\"switch\">";
  html += "<input type=\"checkbox\" name=\"webAuthEnabled\" " + String(config.webAuthEnabled ? "checked" : "") + ">";
  html += "<span class=\"slider\"></span>";
  html += "</label>";
  html += "<label class=\"form-label\">启用身份验证</label>";
  html += "</div>";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">用户名</label>";
  html += "<input type=\"text\" name=\"webUsername\" class=\"form-input\" value=\"" + String(config.webUsername) + "\" maxlength=\"32\">";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">密码</label>";
  html += "<input type=\"password\" name=\"webPassword\" class=\"form-input\" value=\"" + String(config.webPassword) + "\" maxlength=\"32\">";
  html += "</div>";
  html += "<div class=\"btn-group\">";
  html += "<button type=\"submit\" class=\"btn btn-success\">更新访问认证</button>";
  html += "</div>";
  html += "</form>";
  html += "</div>";
  
  // 网络服务配置卡片
  html += "<div class=\"card\">";
  html += "<div class=\"card-title\">网络服务</div>";
  html += "<p style=\"color:#666;font-size:14px;margin-bottom:15px;\">💡 <strong>提示:</strong> 所有网络服务默认关闭以优化性能，请根据实际需求启用相应服务。</p>";
  html += "<form method=\"POST\" action=\"/saveServices\">";
  html += "<div class=\"form-group\">";
  html += "<div class=\"switch-container\">";
  html += "<label class=\"switch\">";
  html += "<input type=\"checkbox\" name=\"tcpEnabled\" " + String(config.tcpEnabled ? "checked" : "") + ">";
  html += "<span class=\"slider\"></span>";
  html += "</label>";
  html += "<label class=\"form-label\">原始TCP</label>";
  html += "</div>";
  html += "<div class=\"input-group\">";
  html += "<label class=\"form-label\">端口:</label>";
  html += "<input type=\"number\" name=\"rawTcpPort\" value=\"" + String(config.rawTcpPort) + "\" min=\"1\" max=\"65535\" class=\"form-input\">";
  html += "</div>";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<div class=\"switch-container\">";
  html += "<label class=\"switch\">";
  html += "<input type=\"checkbox\" name=\"modbusTcpEnabled\" " + String(config.modbusTcpEnabled ? "checked" : "") + ">";
  html += "<span class=\"slider\"></span>";
  html += "</label>";
  html += "<label class=\"form-label\">Modbus TCP</label>";
  html += "</div>";
  html += "<div class=\"input-group\">";
  html += "<label class=\"form-label\">端口:</label>";
  html += "<input type=\"number\" name=\"modbusTcpPort\" value=\"" + String(config.modbusTcpPort) + "\" min=\"1\" max=\"65535\" class=\"form-input\">";
  html += "</div>";
  html += "</div>";
  html += "<div class=\"btn-group\">";
  html += "<button type=\"submit\" class=\"btn btn-success\">更新网络服务</button>";
  html += "</div>";
  html += "</form>";
  html += "</div>";
  
  // 系统信息卡片
  html += "<div class=\"card\">";
  html += "<div class=\"card-title\">系统信息</div>";
  html += "<div class=\"info-grid\">";
  html += "<div class=\"info-item\"><div class=\"info-label\">设备ID</div><div class=\"info-value\">" + String(config.deviceId) + "</div></div>";
  html += "<div class=\"info-item\"><div class=\"info-label\">固件版本</div><div class=\"info-value\">" + String(FIRMWARE_VERSION) + "</div></div>";
  html += "<div class=\"info-item\"><div class=\"info-label\">MAC地址</div><div class=\"info-value\">" + WiFi.macAddress() + "</div></div>";
  html += "<div class=\"info-item\"><div class=\"info-label\">IP地址</div><div class=\"info-value\">" + WiFi.localIP().toString() + "</div></div>";
  html += "<div class=\"info-item\"><div class=\"info-label\">可用内存</div><div class=\"info-value\">" + String(ESP.getFreeHeap()) + " 字节</div></div>";
  html += "<div class=\"info-item\"><div class=\"info-label\">运行时间</div><div class=\"info-value\">" + String(millis() / 60000) + " 分钟</div></div>";
  html += "</div>";
  html += "<div class=\"btn-group\">";
  html += "<button type=\"button\" class=\"btn btn-danger\" onclick=\"if(confirm('确定要重置为默认设置吗？')) location.href='/reset'\">重置为默认设置</button>";
  html += "</div>";
  html += "</div>";
  
  // 固件升级卡片
  html += "<div class=\"card\">";
  html += "<div class=\"card-title\">固件升级</div>";
  html += "<div style=\"margin-bottom:15px;color:#666;font-size:14px;\">";
  html += "当前版本: <strong>" + String(FIRMWARE_VERSION) + "</strong><br>";
  html += "升级前请确保网络连接稳定，升级过程中请勿断电或重启设备。";
  html += "</div>";
  html += "<div class=\"btn-group\">";
  html += "<button type=\"button\" class=\"btn btn-primary\" onclick=\"window.open('/update','_blank')\">开始固件升级</button>";
  html += "</div>";
  html += "</div>";
  
  html += "</div></div></body></html>";
  
  server.send(200, "text/html", html);
}

void handleSaveWiFi() {
  // 检查认证
  if (!checkAuthentication()) {
    return;
  }
  
  if (server.hasArg("ssid") && server.hasArg("password")) {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    
    ssid.toCharArray(config.ssid, sizeof(config.ssid));
    password.toCharArray(config.password, sizeof(config.password));
    
    saveConfig();
    
    server.send(200, "text/html", 
      "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>配置已更新</title></head><body>"
      "<h2>WiFi配置已更新</h2><p>设备将在3秒后重启...</p>"
      "<script>setTimeout(function(){window.location.href='/config';}, 3000);</script></body></html>");
    
    delay(3000);
    ESP.restart();
  } else {
    server.send(400, "text/html", "<h1>错误: 缺少参数</h1>");
  }
}

void handleSaveMqtt() {
  // 检查认证
  if (!checkAuthentication()) {
    return;
  }
  
  config.mqttEnabled = server.hasArg("mqttEnabled");
  
  if (server.hasArg("mqttServer")) {
    server.arg("mqttServer").toCharArray(config.mqttServer, sizeof(config.mqttServer));
  }
  
  if (server.hasArg("mqttPort")) {
    config.mqttPort = server.arg("mqttPort").toInt();
  }
  
  if (server.hasArg("mqttTopic")) {
    server.arg("mqttTopic").toCharArray(config.mqttTopic, sizeof(config.mqttTopic));
  }
  
  if (server.hasArg("mqttUsername")) {
    server.arg("mqttUsername").toCharArray(config.mqttUsername, sizeof(config.mqttUsername));
  }
  
  if (server.hasArg("mqttPassword")) {
    server.arg("mqttPassword").toCharArray(config.mqttPassword, sizeof(config.mqttPassword));
  }
  
  saveConfig();
  
  server.send(200, "text/html", 
    "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>配置已更新</title></head><body>"
    "<h2>MQTT配置已更新</h2><p>正在返回配置页面...</p>"
    "<script>setTimeout(function(){window.location.href='/config';}, 2000);</script></body></html>");
}

void handleSaveAuth() {
  // 检查认证
  if (!checkAuthentication()) {
    return;
  }
  
  config.webAuthEnabled = server.hasArg("webAuthEnabled");
  
  if (server.hasArg("webUsername")) {
    server.arg("webUsername").toCharArray(config.webUsername, sizeof(config.webUsername));
  }
  
  if (server.hasArg("webPassword")) {
    server.arg("webPassword").toCharArray(config.webPassword, sizeof(config.webPassword));
  }
  
  saveConfig();
  
  server.send(200, "text/html", 
    "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>配置已更新</title></head><body>"
    "<h2>访问认证配置已更新</h2><p>正在返回配置页面...</p>"
    "<script>setTimeout(function(){window.location.href='/config';}, 2000);</script></body></html>");
}

void handleSaveServices() {
  // 检查认证
  if (!checkAuthentication()) {
    return;
  }
  
  config.tcpEnabled = server.hasArg("tcpEnabled");
  config.modbusTcpEnabled = server.hasArg("modbusTcpEnabled");
  
  // 处理端口号配置
  bool portsChanged = false;
  if (server.hasArg("rawTcpPort")) {
    int port = server.arg("rawTcpPort").toInt();
    if (port > 0 && port <= 65535 && port != config.rawTcpPort) {
      config.rawTcpPort = port;
      portsChanged = true;
    }
  }
  
  if (server.hasArg("modbusTcpPort")) {
    int port = server.arg("modbusTcpPort").toInt();
    if (port > 0 && port <= 65535 && port != config.modbusTcpPort) {
      config.modbusTcpPort = port;
      portsChanged = true;
    }
  }
  
  saveConfig();
  
  // 如果端口号发生变化，重启TCP服务器
  if (portsChanged) {
    restartTcpServers();
  }
  
  server.send(200, "text/html", 
    "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>配置已更新</title></head><body>"
    "<h2>网络服务配置已更新</h2><p>正在返回配置页面...</p>"
    "<script>setTimeout(function(){window.location.href='/config';}, 2000);</script></body></html>");
}

void handleReset() {
  // 检查认证
  if (!checkAuthentication()) {
    return;
  }
  
  // 重置为默认配置
  setDefaultConfig();
  saveConfig();
  
  server.send(200, "text/html", 
    "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>重置完成</title></head><body>"
    "<h2>配置已重置为默认设置</h2><p>设备将在3秒后重启...</p>"
    "<script>setTimeout(function(){window.location.href='/config';}, 3000);</script></body></html>");
  
  delay(3000);
  ESP.restart();
}

void handleNotFound() {
  server.send(404, "text/plain", "找不到页面");
}

// 处理API配置请求 (为兼容性保留)
void handleGetConfig() {
  handleStatus(); // 重定向到状态处理函数
}

void handleSetConfig() {
  // 检查认证
  if (!checkAuthentication()) {
    return;
  }
  
  server.send(200, "application/json", "{\"success\":true,\"message\":\"请使用具体的配置端点\"}");
}

void handleRestart() {
  // 检查认证
  if (!checkAuthentication()) {
    return;
  }
  
  server.send(200, "application/json", "{\"success\":true,\"message\":\"设备正在重启...\"}");
  delay(1000);
  ESP.restart();
}

// 初始化Web服务器
void initWebServer() {
  Serial.println("Initializing web server...");
  
  // 静态页面路由
  server.on("/", handleRoot);
  server.on("/config", handleConfigPage);
  server.on("/saveWiFi", HTTP_POST, handleSaveWiFi);
  server.on("/saveMqtt", HTTP_POST, handleSaveMqtt);
  server.on("/saveAuth", HTTP_POST, handleSaveAuth);
  server.on("/saveServices", HTTP_POST, handleSaveServices);
  server.on("/reset", handleReset);
  
  // Favicon处理
  server.on("/favicon.ico", []() {
    server.send(204); // No Content
  });
  
  // API路由
  server.on("/api/status", handleStatus);
  server.on("/api/relay", HTTP_POST, handleRelayControl);
  server.on("/api/protocol", HTTP_POST, handleProtocolControl);
  server.on("/api/config", HTTP_GET, handleGetConfig);
  server.on("/api/config", HTTP_POST, handleSetConfig);
  server.on("/api/restart", HTTP_POST, handleRestart);
  
  // 404处理
  server.onNotFound(handleNotFound);
  
  // 设置OTA更新
  httpUpdater.setup(&server, "/update");
  
  server.begin();
  Serial.println("Web server started on port 80");
}
