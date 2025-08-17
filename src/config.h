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
#define RS485_DE 2   // D4 - RS485 DE/RE (GPIO2) - 方向控制引脚

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

// EEPROM地址分配
#define WIFI_SSID_ADDR 0          // 32字节 - WiFi网络名称
#define WIFI_PASS_ADDR 32         // 64字节 - WiFi密码
#define MQTT_SERVER_ADDR 96       // 64字节 - MQTT服务器地址
#define MQTT_PORT_ADDR 160        // 2字节 - MQTT端口
#define MQTT_TOPIC_ADDR 162       // 64字节 - MQTT主题前缀
#define MQTT_USERNAME_ADDR 226    // 32字节 - MQTT用户名
#define MQTT_PASSWORD_ADDR 258    // 32字节 - MQTT密码
#define DEVICE_ID_ADDR 290        // 32字节 - 设备ID
#define WEB_USERNAME_ADDR 322     // 16字节 - Web用户名
#define WEB_PASSWORD_ADDR 338     // 16字节 - Web密码
#define RAW_TCP_PORT_ADDR 354     // 2字节 - 原始TCP端口
#define MODBUS_TCP_PORT_ADDR 356  // 2字节 - Modbus TCP端口
#define MODBUS_SLAVE_ID_ADDR 358  // 1字节 - Modbus从机地址
#define MODBUS_BAUD_RATE_ADDR 359 // 4字节 - Modbus波特率
#define MQTT_ENABLED_ADDR 363     // 1字节 - MQTT启用状态
#define TCP_ENABLED_ADDR 364      // 1字节 - TCP启用状态
#define MODBUS_TCP_ENABLED_ADDR 365 // 1字节 - Modbus TCP启用状态
#define WEB_AUTH_ENABLED_ADDR 366 // 1字节 - Web认证启用状态
#define CONFIG_VALID_ADDR 511     // 1字节 - 配置有效性标志

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
