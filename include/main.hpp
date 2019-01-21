#ifndef __MAIN_HPP_
#define __MAIN_HPP_

#include <Arduino.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Syslog.h>
#include <WiFiManager.h>
#include <Bounce2.h>

const char* otaPassword = "aircon";

#ifdef HARDWARE_V01
#define SWAP_PINS false
#endif

#ifdef HARDWARE_V02
#define SWAP_PINS true
#endif

#ifndef SWAP_PINS
#define HARDWARE_V02
#define SWAP_PINS true
#endif

#define DEBUG_BAUD_RATE 115200

// We'll look for a HIGH signal on this pin to decide if the heatpump unit is
// powering us
#define HEATPUMP_DETECT_PIN 4
// We assume a logic-level shift chip's "enable" or OE pin is wired here with a
// pulldown resistor. We'll output HIGH once we're ready to talk to the
// heatpump.
#ifdef HARDWARE_V01
#define HEATPUMP_ENABLE_PIN 12
#elif defined(HARDWARE_V02)
#define HEATPUMP_ENABLE_PIN 5
#endif

#ifndef HARDWARE_V01
// We look for a 3-second LOW value on this pin to trigger a full reset of all
// settings.
#define CLEAR_SETTINGS_PIN 12
#endif

HardwareSerial* DebugSerial = &Serial;

bool heatPumpDetected = false;
bool detectHeatPump();
bool updateHeatpump = false;
unsigned long lastHeatpumpSettingsChange = 0;

#define CONFIG_SPIFFS_PATH "/config.json"


#ifndef WIFIMANAGER_AP_PASSWORD
#define WIFIMANAGER_AP_PASSWORD "espaircon"
#endif

#define MAX_LENGTH_MQTT_HOST 128
#define MAX_LENGTH_MQTT_PORT 6
#define MAX_LENGTH_MQTT_USERNAME 64
#define MAX_LENGTH_MQTT_PASSWORD 64
#define MAX_LENGTH_MQTT_TOPIC_PREFIX 32
#define MAX_LENGTH_SYSLOG_HOST 128
#define MAX_LENGTH_SYSLOG_PORT 6
#define MAX_LENGTH_SYSLOG_DEVICE_HOSTNAME 32
#define MAX_LENGTH_SYSLOG_APP_NAME 32
#define MAX_LENGTH_SYSLOG_LOG_LEVEL 8

typedef struct {
    char mqtt_host[MAX_LENGTH_MQTT_HOST] = "";
    char mqtt_port[MAX_LENGTH_MQTT_PORT] = "8080";
    char mqtt_username[MAX_LENGTH_MQTT_USERNAME] = "";
    char mqtt_password[MAX_LENGTH_MQTT_PASSWORD] = "";
    char mqtt_topic_prefix[MAX_LENGTH_MQTT_TOPIC_PREFIX] = "";
    char syslog_host[MAX_LENGTH_SYSLOG_HOST] = "";
    char syslog_port[MAX_LENGTH_SYSLOG_PORT] = "514";
    char syslog_device_hostname[MAX_LENGTH_SYSLOG_DEVICE_HOSTNAME] = "";
    char syslog_app_name[MAX_LENGTH_SYSLOG_APP_NAME] = "aircon";
    char syslog_log_level[MAX_LENGTH_SYSLOG_LOG_LEVEL] = "INFO";
} Settings;

WiFiManagerParameter *custom_mqtt_host;
WiFiManagerParameter *custom_mqtt_port;
WiFiManagerParameter *custom_mqtt_username;
WiFiManagerParameter *custom_mqtt_password;
WiFiManagerParameter *custom_mqtt_topic_prefix;
WiFiManagerParameter *custom_syslog_host;
WiFiManagerParameter *custom_syslog_port;
WiFiManagerParameter *custom_syslog_device_hostname;
WiFiManagerParameter *custom_syslog_app_name;
WiFiManagerParameter *custom_syslog_log_level;
WiFiManager *wifiManager;

Settings settings;

char mqtt_topic_info[128];

char mqtt_topic_availability[128];
char mqtt_topic_power_state[128];
char mqtt_topic_mode_state[128];
char mqtt_topic_temperature_state[128];
char mqtt_topic_fan_state[128];
char mqtt_topic_vane_state[128];
char mqtt_topic_current_temperature_state[128];

char mqtt_topic_power_command[128];
char mqtt_topic_mode_command[128];
char mqtt_topic_temperature_command[128];
char mqtt_topic_fan_command[128];
char mqtt_topic_vane_command[128];

Bounce clearSettingsButton;
bool shouldStartConfigAP = false;
bool shouldResetSettings = false;

void setupClearSettingsButtonHandler();
void loadConfig();
void setupWifiManager();
bool startWifiManager();
void saveConfig();

void handleClearSettingsButton();

bool validatePowerValue(const char* value);
bool validateModeValue(const char* value);
bool validateTemperatureValue(const char* value);
bool validateFanValue(const char* value);
bool validateVaneValue(const char* value);

void publishSystemBootInfo();
void publishSystemStatus();

#endif // __MAIN_HPP_
