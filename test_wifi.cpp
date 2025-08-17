#include <ESP8266WiFi.h>

const char* ssid = "SSKJ-4G";
const char* password = "xszn486020zcs";

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println();
  Serial.println("=== 最简单的WiFi连接测试 ===");
  Serial.print("ESP8266 MAC: ");
  Serial.println(WiFi.macAddress());
  
  // 扫描网络
  Serial.println("扫描WiFi网络...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  int n = WiFi.scanNetworks();
  Serial.print("发现 ");
  Serial.print(n);
  Serial.println(" 个网络:");
  
  for (int i = 0; i < n; i++) {
    Serial.print("  ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" (");
    Serial.print(WiFi.RSSI(i));
    Serial.print("dBm) ");
    Serial.print(WiFi.encryptionType(i) == ENC_TYPE_NONE ? "[开放]" : "[加密]");
    if (WiFi.SSID(i) == ssid) {
      Serial.print(" ← 目标网络!");
    }
    Serial.println();
  }
  
  // 连接WiFi
  Serial.println();
  Serial.print("连接到: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    attempts++;
    Serial.print("尝试 ");
    Serial.print(attempts);
    Serial.print("/20 - 状态: ");
    Serial.print(WiFi.status());
    Serial.print(" (");
    
    switch(WiFi.status()) {
      case WL_IDLE_STATUS: Serial.print("空闲"); break;
      case WL_NO_SSID_AVAIL: Serial.print("网络不可用"); break;
      case WL_SCAN_COMPLETED: Serial.print("扫描完成"); break;
      case WL_CONNECTED: Serial.print("已连接"); break;
      case WL_CONNECT_FAILED: Serial.print("连接失败"); break;
      case WL_CONNECTION_LOST: Serial.print("连接丢失"); break;
      case WL_DISCONNECTED: Serial.print("已断开"); break;
      default: Serial.print("未知"); break;
    }
    Serial.println(")");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("✅ WiFi连接成功!");
    Serial.print("IP地址: ");
    Serial.println(WiFi.localIP());
    Serial.print("网关: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("DNS: ");
    Serial.println(WiFi.dnsIP());
    Serial.print("信号强度: ");
    Serial.print(WiFi.RSSI());
    Serial.println("dBm");
  } else {
    Serial.println();
    Serial.println("❌ WiFi连接失败!");
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("在线 - IP: ");
    Serial.print(WiFi.localIP());
    Serial.print(" 信号: ");
    Serial.print(WiFi.RSSI());
    Serial.println("dBm");
  } else {
    Serial.println("离线 - 尝试重连...");
    WiFi.reconnect();
  }
  delay(5000);
}
