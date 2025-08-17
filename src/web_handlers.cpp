#include "relay_controller.h"
#include <ArduinoJson.h>

// Web服务器处理函数

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">";
  html += "<title>ESP8266 Relay Controller</title>";
  html += "<style>body{font-family:Arial;margin:20px;background:#f0f0f0}";
  html += ".container{max-width:800px;margin:0 auto;background:white;padding:20px;border-radius:10px}";
  html += ".relay-card{border:1px solid #ddd;border-radius:8px;padding:15px;margin:10px;display:inline-block}";
  html += ".btn{padding:10px 20px;border:none;border-radius:5px;cursor:pointer;margin:5px}";
  html += ".btn-on{background:#4CAF50;color:white}.btn-off{background:#f44336;color:white}";
  html += "</style></head><body>";
  html += "<div class=\"container\"><h1>ESP8266 Relay Controller</h1>";
  
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
  DynamicJsonDocument doc(512);
  
  doc["deviceId"] = config.deviceId;
  doc["mqttServer"] = config.mqttServer;
  doc["mqttPort"] = config.mqttPort;
  doc["wifiSSID"] = config.ssid;
  // 不返回密码
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleSetConfig() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(512);
    deserializeJson(doc, server.arg("plain"));
    
    if (doc.containsKey("deviceId")) {
      strcpy(config.deviceId, doc["deviceId"]);
    }
    if (doc.containsKey("mqttServer")) {
      strcpy(config.mqttServer, doc["mqttServer"]);
    }
    if (doc.containsKey("mqttPort")) {
      config.mqttPort = doc["mqttPort"];
    }
    if (doc.containsKey("wifiSSID")) {
      strcpy(config.ssid, doc["wifiSSID"]);
    }
    if (doc.containsKey("wifiPassword")) {
      strcpy(config.password, doc["wifiPassword"]);
    }
    
    saveConfig();
    
    server.send(200, "application/json", "{\"success\":true,\"message\":\"配置已保存，重启后生效\"}");
  } else {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"缺少配置数据\"}");
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
      server.send(200, "application/json", "{\"success\":true,\"message\":\"TCP设置已更新\"}");
    } else if (protocol == "modbusTcp") {
      enableModbusTCP(enabled);
      server.send(200, "application/json", "{\"success\":true,\"message\":\"Modbus TCP设置已更新\"}");
    } else {
      server.send(400, "application/json", "{\"success\":false,\"message\":\"未知的协议类型\"}");
    }
  } else {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"缺少请求数据\"}");
  }
}
