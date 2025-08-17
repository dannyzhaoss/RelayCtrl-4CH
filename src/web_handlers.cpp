#include "config.h"
#include "relay_controller.h"
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
  html += "<title>" PROJECT_NAME " Dashboard</title>";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<style>";
  // ThingsBoard风格CSS
  html += "body{font-family:'Roboto',Arial,sans-serif;margin:0;background:#f4f4f4;color:#333}";
  html += ".header{background:#305680;color:white;padding:15px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}";
  html += ".header h1{margin:0;font-size:24px;font-weight:300}";
  html += ".nav-bar{background:white;padding:10px 20px;margin-bottom:20px;border-radius:4px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}";
  html += ".nav-bar a{color:#305680;text-decoration:none;margin-right:20px;font-weight:500}";
  html += ".nav-bar a:hover{color:#1976d2}";
  html += ".container{max-width:1200px;margin:0 auto;padding:20px}";
  html += ".dashboard-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:20px;margin:20px 0}";
  html += ".card{background:white;border-radius:8px;padding:20px;box-shadow:0 2px 8px rgba(0,0,0,0.1)}";
  html += ".card-title{color:#305680;font-size:18px;font-weight:500;margin:0 0 15px 0;padding-bottom:8px;border-bottom:2px solid #e0e0e0}";
  html += ".relay-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:15px}";
  html += ".relay-card{background:#f8f9fa;border-radius:8px;padding:20px;border-left:5px solid #305680;text-align:center}";
  html += ".relay-title{font-size:16px;font-weight:500;margin-bottom:10px;color:#305680}";
  html += ".relay-status{font-size:14px;margin:10px 0;padding:8px;border-radius:4px;font-weight:500}";
  html += ".status-on{background:#e8f5e8;color:#2e7d32;border:1px solid #4caf50}";
  html += ".status-off{background:#fde7e7;color:#c62828;border:1px solid #f44336}";
  html += ".btn{padding:12px 24px;border:none;border-radius:4px;cursor:pointer;font-size:14px;font-weight:500;margin:5px;transition:all 0.3s;min-width:80px}";
  html += ".btn-on{background:#4caf50;color:white}.btn-on:hover{background:#45a049}";
  html += ".btn-off{background:#f44336;color:white}.btn-off:hover{background:#d32f2f}";
  html += ".btn-primary{background:#305680;color:white}.btn-primary:hover{background:#1976d2}";
  html += ".btn-group{display:flex;gap:10px;justify-content:center}";
  html += ".info-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:15px}";
  html += ".info-item{background:#f8f9fa;padding:15px;border-radius:4px;border-left:4px solid #305680}";
  html += ".info-label{font-size:12px;color:#666;text-transform:uppercase;margin-bottom:5px}";
  html += ".info-value{font-size:16px;font-weight:500;color:#305680}";
  html += ".protocol-controls{margin-top:20px}";
  html += ".protocol-item{display:flex;align-items:center;justify-content:space-between;padding:10px 0;border-bottom:1px solid #e0e0e0}";
  html += ".protocol-item:last-child{border-bottom:none}";
  html += ".protocol-status{font-size:14px;font-weight:500}";
  html += ".status-running{color:#4caf50}";
  html += ".status-stopped{color:#f44336}";
  html += ".control-group{display:flex;gap:10px;margin-top:20px;justify-content:center}";
  html += "</style></head>";
  
  html += "<body>";
  html += "<div class=\"header\"><h1>" PROJECT_NAME " Relay Controller</h1></div>";
  html += "<div class=\"nav-bar\">";
  html += "<a href=\"/\">Dashboard</a>";
  html += "<a href=\"/config\">Configuration</a>";
  html += "<a href=\"/update\">OTA Update</a>";
  html += "</div>";
  
  html += "<div class=\"container\">";
  
  // 继电器控制卡片
  html += "<div class=\"card\">";
  html += "<div class=\"card-title\">继电器控制</div>";
  html += "<div class=\"relay-grid\">";
  
  for (int i = 1; i <= 4; i++) {
    html += "<div class=\"relay-card\">";
    html += "<div class=\"relay-title\">Relay " + String(i) + " (JDQ" + String(i-1) + ")</div>";
    html += "<div id=\"relay" + String(i) + "-status\" class=\"relay-status\">Status: Unknown</div>";
    html += "<div class=\"btn-group\">";
    html += "<button class=\"btn btn-on\" onclick=\"controlRelay(" + String(i) + ", true)\">ON</button>";
    html += "<button class=\"btn btn-off\" onclick=\"controlRelay(" + String(i) + ", false)\">OFF</button>";
    html += "</div></div>";
  }
  
  html += "</div></div>";
  
  // 系统信息卡片
  html += "<div class=\"dashboard-grid\">";
  html += "<div class=\"card\">";
  html += "<div class=\"card-title\">系统信息</div>";
  html += "<div class=\"info-grid\">";
  html += "<div class=\"info-item\"><div class=\"info-label\">Device ID</div><div class=\"info-value\" id=\"deviceId\">-</div></div>";
  html += "<div class=\"info-item\"><div class=\"info-label\">IP Address</div><div class=\"info-value\" id=\"ipAddress\">-</div></div>";
  html += "<div class=\"info-item\"><div class=\"info-label\">WiFi</div><div class=\"info-value\" id=\"wifiSSID\">-</div></div>";
  html += "<div class=\"info-item\"><div class=\"info-label\">Uptime</div><div class=\"info-value\" id=\"uptime\">-</div></div>";
  html += "<div class=\"info-item\"><div class=\"info-label\">Free Memory</div><div class=\"info-value\" id=\"freeHeap\">-</div></div>";
  html += "<div class=\"info-item\"><div class=\"info-label\">MQTT</div><div class=\"info-value\" id=\"mqttStatus\">-</div></div>";
  html += "</div></div>";
  
  // 协议控制卡片
  html += "<div class=\"card\">";
  html += "<div class=\"card-title\">协议控制</div>";
  html += "<div class=\"protocol-controls\">";
  html += "<div class=\"protocol-item\">";
  html += "<span>MQTT</span>";
  html += "<div><span id=\"mqttStatusText\" class=\"protocol-status\">-</span>";
  html += "<button id=\"mqttBtn\" class=\"btn btn-primary\" onclick=\"toggleProtocol('mqtt')\">-</button></div>";
  html += "</div>";
  html += "<div class=\"protocol-item\">";
  html += "<span>Raw TCP (Port 8080)</span>";
  html += "<div><span id=\"tcpStatusText\" class=\"protocol-status\">-</span>";
  html += "<button id=\"tcpBtn\" class=\"btn btn-primary\" onclick=\"toggleProtocol('tcp')\">-</button></div>";
  html += "</div>";
  html += "<div class=\"protocol-item\">";
  html += "<span>Modbus TCP (Port 502)</span>";
  html += "<div><span id=\"modbusTcpStatusText\" class=\"protocol-status\">-</span>";
  html += "<button id=\"modbusTcpBtn\" class=\"btn btn-primary\" onclick=\"toggleProtocol('modbusTcp')\">-</button></div>";
  html += "</div></div></div>";
  html += "</div>";
  
  // 控制按钮
  html += "<div class=\"control-group\">";
  html += "<button class=\"btn btn-primary\" onclick=\"refreshStatus()\">刷新状态</button>";
  html += "<button class=\"btn btn-primary\" onclick=\"window.open('/update','_blank')\">OTA升级</button>";
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
  html += "elem.textContent='Status: '+(state?'ON':'OFF');";
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
  html += "document.getElementById('mqttStatus').textContent=data.mqttConnected?'Connected':'Disconnected';";
  html += "updateProtocolStatus('mqtt',data.mqttEnabled);";
  html += "updateProtocolStatus('tcp',data.tcpEnabled);";
  html += "updateProtocolStatus('modbusTcp',data.modbusTcpEnabled);";
  html += "}).catch(error=>{console.error('Status error:',error);";
  html += "document.getElementById('deviceId').textContent='Error: '+error.message;});";
  html += "}";
  
  html += "function updateProtocolStatus(protocol,enabled){";
  html += "const btn=document.getElementById(protocol+'Btn');";
  html += "const text=document.getElementById(protocol+'StatusText');";
  html += "btn.textContent=enabled?'Disable':'Enable';";
  html += "btn.className='btn '+(enabled?'btn-off':'btn-on');";
  html += "text.textContent=enabled?'Running':'Stopped';";
  html += "text.className='protocol-status '+(enabled?'status-running':'status-stopped');";
  html += "}";
  
  html += "function toggleProtocol(protocol){";
  html += "const btn=document.getElementById(protocol+'Btn');";
  html += "const isEnabled=btn.textContent==='Disable';";
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
  html += "<title>" PROJECT_NAME " Configuration</title>";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<style>";
  // ThingsBoard风格的CSS样式
  html += "* { box-sizing: border-box; margin: 0; padding: 0; }";
  html += "body { font-family: 'Roboto', 'Helvetica Neue', sans-serif; background: #f4f4f4; color: #333; line-height: 1.6; }";
  html += ".header { background: linear-gradient(135deg, #305680 0%, #1976d2 100%); color: white; padding: 20px 0; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
  html += ".header h1 { text-align: center; font-weight: 300; font-size: 28px; margin: 0; }";
  html += ".nav-bar { background: white; padding: 15px 0; margin-bottom: 30px; box-shadow: 0 1px 3px rgba(0,0,0,0.1); }";
  html += ".nav-container { max-width: 1200px; margin: 0 auto; display: flex; justify-content: center; }";
  html += ".nav-bar a { color: #305680; text-decoration: none; margin: 0 20px; padding: 10px 15px; border-radius: 4px; font-weight: 500; transition: all 0.3s; }";
  html += ".nav-bar a:hover { background: #e3f2fd; color: #1976d2; }";
  html += ".nav-bar a.active { background: #305680; color: white; }";
  html += ".container { max-width: 800px; margin: 0 auto; padding: 0 20px; }";
  html += ".config-card { background: white; border-radius: 12px; padding: 30px; margin-bottom: 30px; box-shadow: 0 4px 20px rgba(0,0,0,0.08); }";
  html += ".section-title { color: #305680; font-size: 20px; font-weight: 500; margin-bottom: 20px; padding-bottom: 10px; border-bottom: 2px solid #e0e0e0; display: flex; align-items: center; }";
  html += ".section-title::before { content: ''; width: 4px; height: 20px; background: #305680; margin-right: 10px; }";
  html += ".form-group { margin-bottom: 20px; }";
  html += ".form-label { display: block; font-weight: 500; margin-bottom: 8px; color: #555; font-size: 14px; }";
  html += ".form-input { width: 100%; padding: 12px 16px; border: 2px solid #e0e0e0; border-radius: 8px; font-size: 14px; transition: all 0.3s; background: #fafafa; }";
  html += ".form-input:focus { outline: none; border-color: #305680; background: white; box-shadow: 0 0 0 3px rgba(48, 86, 128, 0.1); }";
  html += ".form-select { width: 100%; padding: 12px 16px; border: 2px solid #e0e0e0; border-radius: 8px; font-size: 14px; background: #fafafa; transition: all 0.3s; }";
  html += ".form-select:focus { outline: none; border-color: #305680; background: white; box-shadow: 0 0 0 3px rgba(48, 86, 128, 0.1); }";
  html += ".btn { padding: 12px 24px; border: none; border-radius: 8px; cursor: pointer; font-size: 14px; font-weight: 500; transition: all 0.3s; text-decoration: none; display: inline-block; text-align: center; }";
  html += ".btn-primary { background: linear-gradient(135deg, #305680 0%, #1976d2 100%); color: white; }";
  html += ".btn-primary:hover { transform: translateY(-2px); box-shadow: 0 4px 15px rgba(48, 86, 128, 0.3); }";
  html += ".btn-secondary { background: #6c757d; color: white; }";
  html += ".btn-secondary:hover { background: #545b62; transform: translateY(-2px); }";
  html += ".btn-success { background: linear-gradient(135deg, #28a745 0%, #20c997 100%); color: white; margin-right: 10px; }";
  html += ".btn-success:hover { transform: translateY(-2px); box-shadow: 0 4px 15px rgba(40, 167, 69, 0.3); }";
  html += ".btn-danger { background: linear-gradient(135deg, #dc3545 0%, #fd7e14 100%); color: white; }";
  html += ".btn-danger:hover { transform: translateY(-2px); box-shadow: 0 4px 15px rgba(220, 53, 69, 0.3); }";
  html += ".btn-group { display: flex; gap: 10px; justify-content: center; margin-top: 30px; }";
  html += ".switch-container { display: flex; align-items: center; gap: 10px; }";
  html += ".switch { position: relative; display: inline-block; width: 50px; height: 28px; }";
  html += ".switch input { opacity: 0; width: 0; height: 0; }";
  html += ".slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; border-radius: 28px; }";
  html += ".slider:before { position: absolute; content: ''; height: 20px; width: 20px; left: 4px; bottom: 4px; background-color: white; transition: .4s; border-radius: 50%; }";
  html += "input:checked + .slider { background-color: #305680; }";
  html += "input:checked + .slider:before { transform: translateX(22px); }";
  html += ".info-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin-top: 20px; }";
  html += ".info-item { background: #f8f9fa; padding: 15px; border-radius: 8px; border-left: 4px solid #305680; }";
  html += ".info-label { font-size: 12px; color: #666; text-transform: uppercase; margin-bottom: 5px; font-weight: 500; }";
  html += ".info-value { font-size: 16px; font-weight: 600; color: #305680; }";
  html += ".alert { padding: 15px; border-radius: 8px; margin-bottom: 20px; font-weight: 500; }";
  html += ".alert-success { background: #d4edda; color: #155724; border: 1px solid #c3e6cb; }";
  html += ".alert-danger { background: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }";
  html += "@media (max-width: 768px) { .container { padding: 0 15px; } .config-card { padding: 20px; } .btn-group { flex-direction: column; } .btn { width: 100%; } }";
  html += "</style></head>";
  
  html += "<body>";
  html += "<div class=\"header\"><h1>" PROJECT_NAME " Configuration</h1></div>";
  html += "<div class=\"nav-bar\"><div class=\"nav-container\">";
  html += "<a href=\"/\">Dashboard</a>";
  html += "<a href=\"/config\" class=\"active\">Configuration</a>";
  html += "<a href=\"/update\">OTA Update</a>";
  html += "</div></div>";
  
  html += "<div class=\"container\">";
  
  // WiFi 配置
  html += "<div class=\"config-card\">";
  html += "<div class=\"section-title\">WiFi Configuration</div>";
  html += "<form method=\"POST\" action=\"/saveWiFi\">";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">SSID</label>";
  html += "<input type=\"text\" name=\"ssid\" class=\"form-input\" value=\"" + String(config.ssid) + "\" maxlength=\"32\">";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">Password</label>";
  html += "<input type=\"password\" name=\"password\" class=\"form-input\" value=\"" + String(config.password) + "\" maxlength=\"64\">";
  html += "</div>";
  html += "<div class=\"btn-group\">";
  html += "<button type=\"submit\" class=\"btn btn-primary\">Update WiFi</button>";
  html += "</div>";
  html += "</form>";
  html += "</div>";
  
  // MQTT 配置
  html += "<div class=\"config-card\">";
  html += "<div class=\"section-title\">MQTT Configuration</div>";
  html += "<form method=\"POST\" action=\"/saveMqtt\">";
  html += "<div class=\"form-group\">";
  html += "<div class=\"switch-container\">";
  html += "<label class=\"switch\">";
  html += "<input type=\"checkbox\" name=\"mqttEnabled\" " + String(config.mqttEnabled ? "checked" : "") + ">";
  html += "<span class=\"slider\"></span>";
  html += "</label>";
  html += "<label class=\"form-label\">Enable MQTT</label>";
  html += "</div>";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">MQTT Broker</label>";
  html += "<input type=\"text\" name=\"mqttServer\" class=\"form-input\" value=\"" + String(config.mqttServer) + "\" maxlength=\"128\">";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">Port</label>";
  html += "<input type=\"number\" name=\"mqttPort\" class=\"form-input\" value=\"" + String(config.mqttPort) + "\" min=\"1\" max=\"65535\">";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">Topic Prefix</label>";
  html += "<input type=\"text\" name=\"mqttTopic\" class=\"form-input\" value=\"" + String(config.mqttTopic) + "\" maxlength=\"64\">";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">Username</label>";
  html += "<input type=\"text\" name=\"mqttUsername\" class=\"form-input\" value=\"" + String(config.mqttUsername) + "\" maxlength=\"64\">";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">Password</label>";
  html += "<input type=\"password\" name=\"mqttPassword\" class=\"form-input\" value=\"" + String(config.mqttPassword) + "\" maxlength=\"64\">";
  html += "</div>";
  html += "<div class=\"btn-group\">";
  html += "<button type=\"submit\" class=\"btn btn-success\">Update MQTT Configuration</button>";
  html += "</div>";
  html += "</form>";
  html += "</div>";
  
  // Web Authentication
  html += "<div class=\"config-card\">";
  html += "<div class=\"section-title\">Web Authentication</div>";
  html += "<form method=\"POST\" action=\"/saveAuth\">";
  html += "<div class=\"form-group\">";
  html += "<div class=\"switch-container\">";
  html += "<label class=\"switch\">";
  html += "<input type=\"checkbox\" name=\"webAuthEnabled\" " + String(config.webAuthEnabled ? "checked" : "") + ">";
  html += "<span class=\"slider\"></span>";
  html += "</label>";
  html += "<label class=\"form-label\">Enable Authentication</label>";
  html += "</div>";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">Username</label>";
  html += "<input type=\"text\" name=\"webUsername\" class=\"form-input\" value=\"" + String(config.webUsername) + "\" maxlength=\"32\">";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">Password</label>";
  html += "<input type=\"password\" name=\"webPassword\" class=\"form-input\" value=\"" + String(config.webPassword) + "\" maxlength=\"32\">";
  html += "</div>";
  html += "<div class=\"btn-group\">";
  html += "<button type=\"submit\" class=\"btn btn-success\">Update Authentication</button>";
  html += "</div>";
  html += "</form>";
  html += "</div>";
  
  // Network Services
  html += "<div class=\"config-card\">";
  html += "<div class=\"section-title\">Network Services</div>";
  html += "<form method=\"POST\" action=\"/saveServices\">";
  html += "<div class=\"form-group\">";
  html += "<div class=\"switch-container\">";
  html += "<label class=\"switch\">";
  html += "<input type=\"checkbox\" name=\"tcpEnabled\" " + String(config.tcpEnabled ? "checked" : "") + ">";
  html += "<span class=\"slider\"></span>";
  html += "</label>";
  html += "<label class=\"form-label\">Raw TCP (Port 8080)</label>";
  html += "</div>";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<div class=\"switch-container\">";
  html += "<label class=\"switch\">";
  html += "<input type=\"checkbox\" name=\"modbusTcpEnabled\" " + String(config.modbusTcpEnabled ? "checked" : "") + ">";
  html += "<span class=\"slider\"></span>";
  html += "</label>";
  html += "<label class=\"form-label\">Modbus TCP (Port 502)</label>";
  html += "</div>";
  html += "</div>";
  html += "<div class=\"btn-group\">";
  html += "<button type=\"submit\" class=\"btn btn-success\">Update Services</button>";
  html += "</div>";
  html += "</form>";
  html += "</div>";
  
  // System Information
  html += "<div class=\"config-card\">";
  html += "<div class=\"section-title\">System Information</div>";
  html += "<div class=\"info-grid\">";
  html += "<div class=\"info-item\"><div class=\"info-label\">Device ID</div><div class=\"info-value\">" + String(config.deviceId) + "</div></div>";
  html += "<div class=\"info-item\"><div class=\"info-label\">Firmware</div><div class=\"info-value\">" + String(FIRMWARE_VERSION) + "</div></div>";
  html += "<div class=\"info-item\"><div class=\"info-label\">MAC Address</div><div class=\"info-value\">" + WiFi.macAddress() + "</div></div>";
  html += "<div class=\"info-item\"><div class=\"info-label\">IP Address</div><div class=\"info-value\">" + WiFi.localIP().toString() + "</div></div>";
  html += "<div class=\"info-item\"><div class=\"info-label\">Free Memory</div><div class=\"info-value\">" + String(ESP.getFreeHeap()) + " bytes</div></div>";
  html += "<div class=\"info-item\"><div class=\"info-label\">Uptime</div><div class=\"info-value\">" + String(millis() / 60000) + " min</div></div>";
  html += "</div>";
  html += "<div class=\"btn-group\">";
  html += "<button type=\"button\" class=\"btn btn-danger\" onclick=\"if(confirm('Are you sure?')) location.href='/reset'\">Reset to Defaults</button>";
  html += "</div>";
  html += "</div>";
  
  html += "</div></body></html>";
  
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
      "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>Configuration Updated</title></head><body>"
      "<h2>WiFi Configuration Updated</h2><p>The device will restart in 3 seconds...</p>"
      "<script>setTimeout(function(){window.location.href='/config';}, 3000);</script></body></html>");
    
    delay(3000);
    ESP.restart();
  } else {
    server.send(400, "text/html", "<h1>Error: Missing parameters</h1>");
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
    "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>Configuration Updated</title></head><body>"
    "<h2>MQTT Configuration Updated</h2><p>Redirecting back to configuration...</p>"
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
    "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>Configuration Updated</title></head><body>"
    "<h2>Authentication Configuration Updated</h2><p>Redirecting back to configuration...</p>"
    "<script>setTimeout(function(){window.location.href='/config';}, 2000);</script></body></html>");
}

void handleSaveServices() {
  // 检查认证
  if (!checkAuthentication()) {
    return;
  }
  
  config.tcpEnabled = server.hasArg("tcpEnabled");
  config.modbusTcpEnabled = server.hasArg("modbusTcpEnabled");
  
  saveConfig();
  
  server.send(200, "text/html", 
    "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>Configuration Updated</title></head><body>"
    "<h2>Network Services Configuration Updated</h2><p>Redirecting back to configuration...</p>"
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
    "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>Reset Complete</title></head><body>"
    "<h2>Configuration Reset to Defaults</h2><p>The device will restart in 3 seconds...</p>"
    "<script>setTimeout(function(){window.location.href='/config';}, 3000);</script></body></html>");
  
  delay(3000);
  ESP.restart();
}

void handleNotFound() {
  server.send(404, "text/plain", "File not found");
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
  
  server.send(200, "application/json", "{\"success\":true,\"message\":\"Please use specific configuration endpoints\"}");
}

void handleRestart() {
  // 检查认证
  if (!checkAuthentication()) {
    return;
  }
  
  server.send(200, "application/json", "{\"success\":true,\"message\":\"Device restarting...\"}");
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
