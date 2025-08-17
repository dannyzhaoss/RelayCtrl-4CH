#include "config.h"
#include "relay_controller.h"
#include <ArduinoJson.h>

// Web服务器处理函数

void handleRoot() {
  // 检查认证
  if (!checkAuthentication()) {
    return;
  }
  
  String html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">";
  html += "<title>" PROJECT_NAME " Relay Controller</title>";
  html += "<style>body{font-family:Arial;margin:20px;background:#f0f0f0}";
  html += ".container{max-width:800px;margin:0 auto;background:white;padding:20px;border-radius:10px}";
  html += ".relay-card{border:1px solid #ddd;border-radius:8px;padding:15px;margin:10px;display:inline-block}";
  html += ".btn{padding:10px 20px;border:none;border-radius:5px;cursor:pointer;margin:5px}";
  html += ".btn-on{background:#4CAF50;color:white}.btn-off{background:#f44336;color:white}";
  html += "</style></head><body>";
  html += "<div class=\"container\"><h1>" PROJECT_NAME " Relay Controller</h1>";
  
  // 添加继电器控制卡片
  for (int i = 1; i <= 4; i++) {
    html += "<div class=\"relay-card\"><h3>Relay " + String(i) + " (JDQ" + String(i-1) + ")</h3>";
    html += "<div id=\"relay" + String(i) + "-status\">Status: Unknown</div>";
    html += "<button class=\"btn btn-on\" onclick=\"controlRelay(" + String(i) + ", true)\">ON</button>";
    html += "<button class=\"btn btn-off\" onclick=\"controlRelay(" + String(i) + ", false)\">OFF</button>";
    html += "</div>";
  }
  
  html += "<div style=\"margin-top:20px;padding:15px;background:#e7f3ff;border-radius:5px\">";
  html += "<h3>System Info</h3>";
  html += "<div>Device ID: <span id=\"deviceId\">-</span></div>";
  html += "<div>IP Address: <span id=\"ipAddress\">-</span></div>";
  html += "<div>WiFi: <span id=\"wifiSSID\">-</span></div>";
  html += "<div>Uptime: <span id=\"uptime\">-</span></div>";
  html += "<div>Free Memory: <span id=\"freeHeap\">-</span></div>";
  html += "<div>MQTT: <span id=\"mqttStatus\">-</span></div>";
  html += "</div>";
  
  // 协议控制部分
  html += "<div style=\"margin-top:20px;padding:15px;background:#fff3cd;border-radius:5px\">";
  html += "<h3>Protocol Control</h3>";
  html += "<div style=\"margin:10px 0\">";
  html += "<label>MQTT: </label>";
  html += "<button id=\"mqttBtn\" class=\"btn\" onclick=\"toggleProtocol('mqtt')\">-</button>";
  html += "<span id=\"mqttStatusText\" style=\"margin-left:10px\">-</span>";
  html += "</div>";
  html += "<div style=\"margin:10px 0\">";
  html += "<label>Raw TCP (Port 8080): </label>";
  html += "<button id=\"tcpBtn\" class=\"btn\" onclick=\"toggleProtocol('tcp')\">-</button>";
  html += "<span id=\"tcpStatusText\" style=\"margin-left:10px\">-</span>";
  html += "</div>";
  html += "<div style=\"margin:10px 0\">";
  html += "<label>Modbus TCP (Port 502): </label>";
  html += "<button id=\"modbusTcpBtn\" class=\"btn\" onclick=\"toggleProtocol('modbusTcp')\">-</button>";
  html += "<span id=\"modbusTcpStatusText\" style=\"margin-left:10px\">-</span>";
  html += "</div>";
  html += "</div>";
  
  html += "<div style=\"text-align:center;margin-top:20px\">";
  html += "<button class=\"btn\" style=\"background:#2196F3;color:white\" onclick=\"refreshStatus()\">Refresh</button>";
  html += "<button class=\"btn\" style=\"background:#FF9800;color:white\" onclick=\"window.open('/update','_blank')\">OTA Update</button>";
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
  html += "elem.style.backgroundColor=state?'#d4edda':'#f8d7da';";
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
  html += "text.style.color=enabled?'green':'red';";
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
  html += "setInterval(refreshStatus,8000);"; // 增加刷新间隔到8秒
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
  json += "\"tcpEnabled\":" + String(config.tcpEnabled ? "true" : "false") + ",";
  json += "\"modbusTcpEnabled\":" + String(config.modbusTcpEnabled ? "true" : "false") + ",";
  json += "\"firmwareVersion\":\"" FIRMWARE_VERSION "\",";
  json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"uptime\":" + String(millis() / 1000);
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleSetConfig() {
  // 检查认证
  if (!checkAuthentication()) {
    return;
  }
  
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
    
    // 检查MQTT用户名配置
    if (doc.containsKey("mqttUsername")) {
      String newUsername = doc["mqttUsername"];
      if (newUsername != String(config.mqttUsername)) {
        strncpy(config.mqttUsername, newUsername.c_str(), sizeof(config.mqttUsername) - 1);
        config.mqttUsername[sizeof(config.mqttUsername) - 1] = '\0';
        changed = true;
      }
    }
    
    // 检查MQTT密码配置
    if (doc.containsKey("mqttPassword")) {
      String newPassword = doc["mqttPassword"];
      if (newPassword != String(config.mqttPassword)) {
        strncpy(config.mqttPassword, newPassword.c_str(), sizeof(config.mqttPassword) - 1);
        config.mqttPassword[sizeof(config.mqttPassword) - 1] = '\0';
        changed = true;
      }
    }
    
    // 检查Web认证用户名
    if (doc.containsKey("webUsername")) {
      String newUsername = doc["webUsername"];
      if (newUsername != String(config.webUsername)) {
        strncpy(config.webUsername, newUsername.c_str(), sizeof(config.webUsername) - 1);
        config.webUsername[sizeof(config.webUsername) - 1] = '\0';
        changed = true;
      }
    }
    
    // 检查Web认证密码
    if (doc.containsKey("webPassword")) {
      String newPassword = doc["webPassword"];
      if (newPassword != String(config.webPassword)) {
        strncpy(config.webPassword, newPassword.c_str(), sizeof(config.webPassword) - 1);
        config.webPassword[sizeof(config.webPassword) - 1] = '\0';
        changed = true;
      }
    }
    
    // 检查Web认证启用状态
    if (doc.containsKey("webAuthEnabled")) {
      bool newEnabled = doc["webAuthEnabled"];
      if (newEnabled != config.webAuthEnabled) {
        config.webAuthEnabled = newEnabled;
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
  // 检查认证
  if (!checkAuthentication()) {
    return;
  }
  
  String html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">";
  html += "<title>" PROJECT_NAME " Configuration</title>";
  html += "<style>";
  // ThingsBoard风格CSS
  html += "body{font-family:'Roboto',Arial,sans-serif;margin:0;background:#f4f4f4;color:#333}";
  html += ".header{background:#305680;color:white;padding:15px 20px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}";
  html += ".header h1{margin:0;font-size:24px;font-weight:300}";
  html += ".container{max-width:1200px;margin:0 auto;padding:20px}";
  html += ".nav-bar{background:white;padding:10px 20px;margin-bottom:20px;border-radius:4px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}";
  html += ".nav-bar a{color:#305680;text-decoration:none;margin-right:20px;font-weight:500}";
  html += ".nav-bar a:hover{color:#1976d2}";
  html += ".config-section{background:white;border-radius:4px;padding:20px;margin:15px 0;box-shadow:0 2px 4px rgba(0,0,0,0.1)}";
  html += ".section-title{color:#305680;font-size:18px;font-weight:500;margin:0 0 15px 0;padding-bottom:8px;border-bottom:2px solid #e0e0e0}";
  html += ".form-group{margin:15px 0;display:flex;align-items:center}";
  html += ".form-label{width:180px;font-weight:500;color:#666}";
  html += ".form-control{padding:8px 12px;border:1px solid #ddd;border-radius:4px;width:250px;font-size:14px}";
  html += ".form-control:focus{border-color:#305680;outline:none;box-shadow:0 0 5px rgba(48,86,128,0.3)}";
  html += ".btn{padding:10px 20px;border:none;border-radius:4px;cursor:pointer;margin:5px;font-size:14px;font-weight:500;transition:all 0.3s}";
  html += ".btn-primary{background:#305680;color:white}";
  html += ".btn-primary:hover{background:#1976d2}";
  html += ".btn-success{background:#4caf50;color:white}";
  html += ".btn-success:hover{background:#45a049}";
  html += ".btn-warning{background:#ff9800;color:white}";
  html += ".btn-warning:hover{background:#f57c00}";
  html += ".btn-danger{background:#f44336;color:white}";
  html += ".btn-danger:hover{background:#d32f2f}";
  html += ".status{padding:8px 12px;border-radius:4px;font-size:13px;font-weight:500}";
  html += ".status-on{background:#e8f5e8;color:#2e7d32;border:1px solid #4caf50}";
  html += ".status-off{background:#fde7e7;color:#c62828;border:1px solid #f44336}";
  html += ".info-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:15px}";
  html += ".info-item{background:#f8f9fa;padding:12px;border-radius:4px;border-left:4px solid #305680}";
  html += ".info-label{font-size:12px;color:#666;margin-bottom:4px}";
  html += ".info-value{font-size:14px;font-weight:500;color:#333}";
  html += "</style></head><body>";
  
  // Header
  html += "<div class=\"header\">";
  html += "<h1>" PROJECT_NAME " Configuration Panel</h1>";
  html += "</div>";
  
  // Navigation
  html += "<div class=\"nav-bar\">";
  html += "<a href=\"/\">← Dashboard</a>";
  html += "<a href=\"/config\">Configuration</a>";
  html += "</div>";
  
  html += "<div class=\"container\">";
  
  // WiFi配置
  html += "<div class=\"config-section\">";
  html += "<h3 class=\"section-title\">WiFi Configuration</h3>";
  html += "<div class=\"info-grid\">";
  html += "<div class=\"info-item\">";
  html += "<div class=\"info-label\">Current SSID</div>";
  html += "<div class=\"info-value\">" + WiFi.SSID() + "</div>";
  html += "</div>";
  html += "<div class=\"info-item\">";
  html += "<div class=\"info-label\">IP Address</div>";
  html += "<div class=\"info-value\">" + WiFi.localIP().toString() + "</div>";
  html += "</div>";
  html += "<div class=\"info-item\">";
  html += "<div class=\"info-label\">Signal Strength</div>";
  html += "<div class=\"info-value\">" + String(WiFi.RSSI()) + " dBm</div>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  
  // MQTT配置
  html += "<div class=\"config-section\">";
  html += "<h3 class=\"section-title\">MQTT Configuration</h3>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">Status:</label>";
  html += "<span class=\"status " + String(config.mqttEnabled ? "status-on" : "status-off") + "\">";
  html += config.mqttEnabled ? "Enabled" : "Disabled";
  html += "</span>";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">Server:</label>";
  html += "<input type=\"text\" class=\"form-control\" id=\"mqttServer\" value=\"" + String(config.mqttServer) + "\">";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">Port:</label>";
  html += "<input type=\"number\" class=\"form-control\" id=\"mqttPort\" value=\"" + String(config.mqttPort) + "\">";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">Topic Prefix:</label>";
  html += "<input type=\"text\" class=\"form-control\" id=\"mqttTopic\" value=\"" + String(config.mqttTopic) + "\" placeholder=\"relay/\">";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">Username:</label>";
  html += "<input type=\"text\" class=\"form-control\" id=\"mqttUsername\" value=\"" + String(config.mqttUsername) + "\" placeholder=\"Optional\">";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">Password:</label>";
  html += "<input type=\"password\" class=\"form-control\" id=\"mqttPassword\" value=\"" + String(config.mqttPassword) + "\" placeholder=\"Optional\">";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">Client ID:</label>";
  html += "<span class=\"info-value\">" + String(config.deviceId) + "</span>";
  html += "</div>";
  html += "<button class=\"btn btn-primary\" onclick=\"updateMqttConfig()\">Update MQTT Configuration</button>";
  html += "</div>";
  
  // Web认证配置
  html += "<div class=\"config-section\">";
  html += "<h3 class=\"section-title\">Web Authentication</h3>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">Enable Auth:</label>";
  html += "<span class=\"status " + String(config.webAuthEnabled ? "status-on" : "status-off") + "\">";
  html += config.webAuthEnabled ? "Enabled" : "Disabled";
  html += "</span>";
  html += "<button class=\"btn " + String(config.webAuthEnabled ? "btn-danger" : "btn-success") + "\" onclick=\"toggleAuth()\">";
  html += config.webAuthEnabled ? "Disable" : "Enable";
  html += "</button>";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">Username:</label>";
  html += "<input type=\"text\" class=\"form-control\" id=\"webUsername\" value=\"" + String(config.webUsername) + "\">";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">Password:</label>";
  html += "<input type=\"password\" class=\"form-control\" id=\"webPassword\" value=\"" + String(config.webPassword) + "\">";
  html += "</div>";
  html += "<button class=\"btn btn-primary\" onclick=\"updateAuthConfig()\">Update Authentication</button>";
  html += "</div>";
  
  // TCP服务器配置
  html += "<div class=\"config-section\">";
  html += "<h3 class=\"section-title\">Network Services</h3>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">Raw TCP (" + String(RAW_TCP_PORT) + "):</label>";
  html += "<span class=\"status " + String(config.tcpEnabled ? "status-on" : "status-off") + "\">";
  html += config.tcpEnabled ? "Enabled" : "Disabled";
  html += "</span>";
  html += "<button class=\"btn " + String(config.tcpEnabled ? "btn-warning" : "btn-success") + "\" onclick=\"toggleProtocol('tcp')\">";
  html += config.tcpEnabled ? "Disable" : "Enable";
  html += "</button>";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label class=\"form-label\">Modbus TCP (" + String(MODBUS_TCP_PORT) + "):</label>";
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
  html += "<h3 class=\"section-title\">System Information</h3>";
  html += "<div class=\"info-grid\">";
  html += "<div class=\"info-item\">";
  html += "<div class=\"info-label\">Device ID</div>";
  html += "<div class=\"info-value\">" + String(config.deviceId) + "</div>";
  html += "</div>";
  html += "<div class=\"info-item\">";
  html += "<div class=\"info-label\">MAC Address</div>";
  html += "<div class=\"info-value\">" + WiFi.macAddress() + "</div>";
  html += "</div>";
  html += "<div class=\"info-item\">";
  html += "<div class=\"info-label\">Firmware Version</div>";
  html += "<div class=\"info-value\">" FIRMWARE_VERSION "</div>";
  html += "</div>";
  html += "<div class=\"info-item\">";
  html += "<div class=\"info-label\">Free Memory</div>";
  html += "<div class=\"info-value\">" + String(ESP.getFreeHeap()) + " bytes</div>";
  html += "</div>";
  html += "<div class=\"info-item\">";
  html += "<div class=\"info-label\">Uptime</div>";
  html += "<div class=\"info-value\">" + String(millis() / 1000) + " seconds</div>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  
  // JavaScript
  html += "<script>";
  html += "function showMessage(msg, isSuccess) {";
  html += "  const style = isSuccess ? 'background:#4caf50;color:white' : 'background:#f44336;color:white';";
  html += "  const div = document.createElement('div');";
  html += "  div.style.cssText = `position:fixed;top:20px;right:20px;padding:15px;border-radius:4px;z-index:1000;${style}`;";
  html += "  div.textContent = msg;";
  html += "  document.body.appendChild(div);";
  html += "  setTimeout(() => div.remove(), 3000);";
  html += "}";
  html += "function updateMqttConfig() {";
  html += "  const data = {";
  html += "    mqttServer: document.getElementById('mqttServer').value,";
  html += "    mqttPort: parseInt(document.getElementById('mqttPort').value),";
  html += "    mqttTopic: document.getElementById('mqttTopic').value,";
  html += "    mqttUsername: document.getElementById('mqttUsername').value,";
  html += "    mqttPassword: document.getElementById('mqttPassword').value";
  html += "  };";
  html += "  fetch('/api/config', {";
  html += "    method: 'POST',";
  html += "    headers: {'Content-Type': 'application/json'},";
  html += "    body: JSON.stringify(data)";
  html += "  }).then(response => response.json())";
  html += "    .then(data => {";
  html += "      showMessage(data.message || 'Configuration updated', data.success);";
  html += "      if(data.success) setTimeout(() => location.reload(), 1500);";
  html += "    });";
  html += "}";
  html += "function updateAuthConfig() {";
  html += "  const data = {";
  html += "    webUsername: document.getElementById('webUsername').value,";
  html += "    webPassword: document.getElementById('webPassword').value";
  html += "  };";
  html += "  fetch('/api/config', {";
  html += "    method: 'POST',";
  html += "    headers: {'Content-Type': 'application/json'},";
  html += "    body: JSON.stringify(data)";
  html += "  }).then(response => response.json())";
  html += "    .then(data => {";
  html += "      showMessage(data.message || 'Authentication updated', data.success);";
  html += "      if(data.success) setTimeout(() => location.reload(), 1500);";
  html += "    });";
  html += "}";
  html += "function toggleAuth() {";
  html += "  const enabled = " + String(config.webAuthEnabled ? "false" : "true") + ";";
  html += "  fetch('/api/config', {";
  html += "    method: 'POST',";
  html += "    headers: {'Content-Type': 'application/json'},";
  html += "    body: JSON.stringify({webAuthEnabled: enabled})";
  html += "  }).then(response => response.json())";
  html += "    .then(data => {";
  html += "      showMessage(data.message || 'Authentication toggled', data.success);";
  html += "      if(data.success) setTimeout(() => location.reload(), 1500);";
  html += "    });";
  html += "}";
  html += "function toggleProtocol(protocol) {";
  html += "  fetch('/api/protocol', {";
  html += "    method: 'POST',";
  html += "    headers: {'Content-Type': 'application/json'},";
  html += "    body: JSON.stringify({protocol: protocol, enabled: !getCurrentState(protocol)})";
  html += "  }).then(response => response.json())";
  html += "    .then(data => {";
  html += "      showMessage(data.message || 'Protocol toggled', data.success);";
  html += "      if(data.success) setTimeout(() => location.reload(), 1500);";
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
