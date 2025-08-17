#include "config.h"
#include "relay_controller.h"
#include <ArduinoJson.h>

// MQTT处理函数

void reconnectMQTT() {
  // 增加重连间隔到10秒，避免频繁重连影响Web性能
  if (millis() - lastMqttReconnect < 10000) return; 
  
  lastMqttReconnect = millis();
  
  if (mqttClient.connect(dynamicMqttClientId.c_str())) {
    Serial.println("MQTT connected");
    Serial.print("Client ID: ");
    Serial.println(dynamicMqttClientId);
    
    // 订阅控制主题
    String controlTopic = String(MQTT_TOPIC_BASE) + config.deviceId + "/control";
    mqttClient.subscribe(controlTopic.c_str());
    
    String configTopic = String(MQTT_TOPIC_BASE) + config.deviceId + "/config";
    mqttClient.subscribe(configTopic.c_str());
    
    // 发布在线状态
    publishOnlineStatus();
    
    // 发布当前继电器状态
    for (int i = 0; i < 4; i++) {
      publishRelayState(i, relayStates[i]);
    }
    
    Serial.print("Subscribed to: ");
    Serial.println(controlTopic);
  } else {
    Serial.print("MQTT connection failed, rc=");
    Serial.println(mqttClient.state());
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("MQTT message received [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);
  
  String topicStr = String(topic);
  String baseTopic = String(MQTT_TOPIC_BASE) + config.deviceId;
  
  if (topicStr == baseTopic + "/control") {
    handleMqttControl(message);
  } else if (topicStr == baseTopic + "/config") {
    handleMqttConfig(message);
  }
}

void handleMqttControl(String message) {
  DynamicJsonDocument doc(256);
  deserializeJson(doc, message);
  
  if (doc.containsKey("relay") && doc.containsKey("state")) {
    int relay = doc["relay"];
    bool state = doc["state"];
    
    if (relay >= 1 && relay <= 4) {
      setRelay(relay - 1, state);
      Serial.print("MQTT control: Relay ");
      Serial.print(relay);
      Serial.print(" -> ");
      Serial.println(state ? "ON" : "OFF");
    }
  } else if (doc.containsKey("command")) {
    String command = doc["command"];
    
    if (command == "status") {
      publishSystemStatus();
    } else if (command == "restart") {
      Serial.println("MQTT restart command received");
      delay(1000);
      ESP.restart();
    } else if (command == "all_on") {
      for (int i = 0; i < 4; i++) {
        setRelay(i, true);
      }
    } else if (command == "all_off") {
      for (int i = 0; i < 4; i++) {
        setRelay(i, false);
      }
    }
  }
}

void handleMqttConfig(String message) {
  DynamicJsonDocument doc(512);
  deserializeJson(doc, message);
  
  bool configChanged = false;
  
  if (doc.containsKey("deviceId")) {
    strcpy(config.deviceId, doc["deviceId"]);
    configChanged = true;
  }
  if (doc.containsKey("mqttServer")) {
    strcpy(config.mqttServer, doc["mqttServer"]);
    configChanged = true;
  }
  if (doc.containsKey("mqttPort")) {
    config.mqttPort = doc["mqttPort"];
    configChanged = true;
  }
  
  if (configChanged) {
    saveConfig();
    Serial.println("Configuration updated via MQTT");
    
    // 发布配置更新确认
    String topic = String(MQTT_TOPIC_BASE) + config.deviceId + "/status";
    mqttClient.publish(topic.c_str(), "{\"message\":\"config_updated\"}");
  }
}

void publishRelayState(int relay, bool state) {
  if (!mqttClient.connected()) return;
  
  String topic = String(MQTT_TOPIC_BASE) + config.deviceId + "/relay" + String(relay + 1) + "/state";
  String payload = state ? "ON" : "OFF";
  
  mqttClient.publish(topic.c_str(), payload.c_str(), true); // 保留消息
}

void publishSystemStatus() {
  if (!mqttClient.connected()) return;
  
  DynamicJsonDocument doc(512);
  doc["deviceId"] = config.deviceId;
  doc["ip"] = WiFi.localIP().toString();
  doc["wifi"] = WiFi.SSID();
  doc["rssi"] = WiFi.RSSI();
  doc["uptime"] = millis() / 1000;
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["version"] = "1.0.0";
  
  JsonArray relays = doc.createNestedArray("relays");
  for (int i = 0; i < 4; i++) {
    JsonObject relay = relays.createNestedObject();
    relay["id"] = i + 1;
    relay["state"] = relayStates[i] ? "ON" : "OFF";
    relay["pin"] = getRelayPin(i);
  }
  
  String payload;
  serializeJson(doc, payload);
  
  String topic = String(MQTT_TOPIC_BASE) + config.deviceId + "/status";
  mqttClient.publish(topic.c_str(), payload.c_str());
}

void publishOnlineStatus() {
  if (!mqttClient.connected()) return;
  
  String topic = String(MQTT_TOPIC_BASE) + config.deviceId + "/online";
  mqttClient.publish(topic.c_str(), "true", true);
}

void sendHeartbeat() {
  static unsigned long lastHeartbeat = 0;
  
  if (millis() - lastHeartbeat > 30000) { // 每30秒发送一次心跳
    lastHeartbeat = millis();
    
    if (mqttClient.connected()) {
      DynamicJsonDocument doc(256);
      doc["timestamp"] = millis();
      doc["uptime"] = millis() / 1000;
      doc["freeHeap"] = ESP.getFreeHeap();
      doc["rssi"] = WiFi.RSSI();
      
      String payload;
      serializeJson(doc, payload);
      
      String topic = String(MQTT_TOPIC_BASE) + config.deviceId + "/heartbeat";
      mqttClient.publish(topic.c_str(), payload.c_str());
    }
  }
}

int getRelayPin(int relay) {
  switch (relay) {
    case 0: return RELAY1_PIN;
    case 1: return RELAY2_PIN;
    case 2: return RELAY3_PIN;
    case 3: return RELAY4_PIN;
    default: return -1;
  }
}
