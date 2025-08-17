#ifndef CONFIG_H
#define CONFIG_H

// ===== 硬件引脚定义 =====
// 继电器引脚定义 (JDQ0-3)
#define RELAY1_PIN 12  // D6 - JDQ0
#define RELAY2_PIN 13  // D7 - JDQ1  
#define RELAY3_PIN 14  // D5 - JDQ2
#define RELAY4_PIN 16  // D0 - JDQ3

// 串口引脚定义
#define DEBUG_RX 3   // RXD0
#define DEBUG_TX 1   // TXD0
#define RS485_RX 4   // D2 - RS485 RX (GPIO4)
#define RS485_TX 5   // D1 - RS485 TX (GPIO5)

// ===== 网络配置 =====
#define DEFAULT_SSID "SSKJ-4"
#define DEFAULT_PASSWORD "xszn486020zcs"
#define AP_PASSWORD "12345678"

// ===== MQTT配置 =====
#define MQTT_BROKER "192.168.1.100"
#define MQTT_PORT 1883
#define MQTT_TOPIC_BASE "relay/"
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""

// ===== Modbus配置 =====
#define MODBUS_SLAVE_ID 1
#define MODBUS_BAUD 9600

// ===== 存储配置 =====
#define EEPROM_SIZE 512
#define WIFI_SSID_ADDR 0
#define WIFI_PASS_ADDR 64
#define MQTT_SERVER_ADDR 128
#define MQTT_PORT_ADDR 192
#define DEVICE_ID_ADDR 196

// ===== 系统信息 =====
#define FIRMWARE_VERSION "1.0.0"
#define DEVICE_TYPE "JDQ0-3 Relay Control"
#define PROJECT_NAME "RelayCtrl"

// ===== 服务器端口 =====
#define HTTP_PORT 80
#define MODBUS_TCP_PORT 502
#define RAW_TCP_PORT 8080

// ===== 默认设备配置 =====
#define DEFAULT_DEVICE_ID "RelayCtrl_001"

#endif // CONFIG_H
