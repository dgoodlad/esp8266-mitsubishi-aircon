#include <Arduino.h>
#include <FS.h>

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <AsyncMqttClient.h>

// WiFiManager requires the stock synchronous web server
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>

#include <ArduinoJson.h>

#include <HeatPump.h>

char mqtt_host[255];
char mqtt_port[6] = "8080";
char mqtt_username[64];
char mqtt_password[64];
char mqtt_topic_prefix[128];

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

char config_ap_name[17];
bool shouldSaveConfig = false;

AsyncMqttClient mqttClient;

HeatPump heatpump;

void saveConfigCallback () {
    shouldSaveConfig = true;
}

void heatpumpSettingsChanged() {
    char temperature[4];
    heatpumpSettings settings = heatpump.getSettings();
    snprintf(temperature, 4, "%3.0f", settings.temperature);
    mqttClient.publish(mqtt_topic_power_state, 0, true, settings.power.c_str());
    mqttClient.publish(mqtt_topic_mode_state, 0, true, settings.mode.c_str());
    mqttClient.publish(mqtt_topic_temperature_state, 0, true, temperature);
    mqttClient.publish(mqtt_topic_fan_state, 0, true, settings.fan.c_str());
    mqttClient.publish(mqtt_topic_vane_state, 0, true, settings.vane.c_str());
}

void heatpumpStatusChanged(heatpumpStatus status) {
    char temperature[4];
    snprintf(temperature, 4, "%3.0f", status.roomTemperature);
    mqttClient.publish(mqtt_topic_current_temperature_state, 0, true, temperature);
}

void mqttConnect(bool sessionPresent) {
    mqttClient.subscribe(mqtt_topic_power_command, 0);
    mqttClient.subscribe(mqtt_topic_mode_command, 0);
    mqttClient.subscribe(mqtt_topic_temperature_command, 0);
    mqttClient.subscribe(mqtt_topic_fan_command, 0);
    mqttClient.subscribe(mqtt_topic_vane_command, 0);
}

void mqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
    if(strcmp(topic, mqtt_topic_power_command)) {
        heatpump.setPowerSetting(payload);
    } else if(strcmp(topic, mqtt_topic_mode_command)) {
        heatpump.setModeSetting(payload);
    } else if(strcmp(topic, mqtt_topic_temperature_command)) {
        heatpump.setTemperature(atof(payload));
    } else if(strcmp(topic, mqtt_topic_fan_command)) {
        heatpump.setFanSpeed(payload);
    } else if(strcmp(topic, mqtt_topic_vane_command)) {
        heatpump.setVaneSetting(payload);
    }
}

void setup() {
    if (SPIFFS.begin()) {
        if (SPIFFS.exists("/config.json")) {
            File configFile = SPIFFS.open("/config.json", "r");
            if (configFile) {
                size_t size = configFile.size();
                std::unique_ptr<char[]> buf(new char[size]);
                configFile.readBytes(buf.get(), size);
                DynamicJsonBuffer jsonBuffer;
                JsonObject& json = jsonBuffer.parseObject(buf.get());
                if (json.success()) {
                    strncpy(mqtt_host, json["mqtt_host"], 255);
                    strncpy(mqtt_port, json["mqtt_port"], 6);
                    strncpy(mqtt_username, json["mqtt_username"], 64);
                    strncpy(mqtt_password, json["mqtt_password"], 64);
                    strncpy(mqtt_topic_prefix, json["mqtt_topic_prefix"], 128);
                } else {
                    // Failed to load json config
                }
                configFile.close();
            }
        }
    } else {
        // Failed to mount FS
    }

    WiFiManagerParameter custom_mqtt_host("mqtt_host", "MQTT Host", mqtt_host, 255);
    WiFiManagerParameter custom_mqtt_port("mqtt_port", "MQTT Port", mqtt_port, 6);
    WiFiManagerParameter custom_mqtt_username("mqtt_username", "MQTT Username", mqtt_username, 255);
    WiFiManagerParameter custom_mqtt_password("mqtt_password", "MQTT Password", mqtt_password, 255);
    WiFiManagerParameter custom_mqtt_topic_prefix("mqtt_topic_prefis", "MQTT Topic Prefix", mqtt_topic_prefix, 128);

    WiFiManager wifiManager;

    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.addParameter(&custom_mqtt_host);
    wifiManager.addParameter(&custom_mqtt_port);
    wifiManager.addParameter(&custom_mqtt_username);
    wifiManager.addParameter(&custom_mqtt_password);
    wifiManager.addParameter(&custom_mqtt_topic_prefix);

    snprintf(config_ap_name, 17, "ESP8266 %08x", ESP.getChipId());

    if (!wifiManager.autoConnect(config_ap_name, "aircon")) {
        delay(3000);
        ESP.reset();
        delay(5000);
    }

    // Successfully connected to wifi

    strncpy(mqtt_host, custom_mqtt_host.getValue(), 255);
    strncpy(mqtt_port, custom_mqtt_port.getValue(), 6);
    strncpy(mqtt_username, custom_mqtt_username.getValue(), 64);
    strncpy(mqtt_password, custom_mqtt_password.getValue(), 64);
    strncpy(mqtt_topic_prefix, custom_mqtt_topic_prefix.getValue(), 128);

    if (shouldSaveConfig) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.createObject();
        json["mqtt_host"] = mqtt_host;
        json["mqtt_port"] = mqtt_port;
        json["mqtt_username"] = mqtt_username;
        json["mqtt_password"] = mqtt_password;
        json["mqtt_topic_prefix"] = mqtt_topic_prefix;

        File configFile = SPIFFS.open("/config.json", "w");
        if (!configFile) {
            // Failed to open config file
        }

        json.printTo(configFile);
        configFile.close();
    }

    snprintf(mqtt_topic_power_state, 128, "%s/power/state", mqtt_topic_prefix);
    snprintf(mqtt_topic_mode_state, 128, "%s/mode/state", mqtt_topic_prefix);
    snprintf(mqtt_topic_temperature_state, 128, "%s/temperature/state", mqtt_topic_prefix);
    snprintf(mqtt_topic_fan_state, 128, "%s/fan/state", mqtt_topic_prefix);
    snprintf(mqtt_topic_vane_state, 128, "%s/vane/state", mqtt_topic_prefix);
    snprintf(mqtt_topic_current_temperature_state, 128, "%s/current_temperature/state", mqtt_topic_prefix);

    snprintf(mqtt_topic_power_command, 128, "%s/power/set", mqtt_topic_prefix);
    snprintf(mqtt_topic_mode_command, 128, "%s/mode/set", mqtt_topic_prefix);
    snprintf(mqtt_topic_temperature_command, 128, "%s/temperature/set", mqtt_topic_prefix);
    snprintf(mqtt_topic_fan_command, 128, "%s/fan/set", mqtt_topic_prefix);
    snprintf(mqtt_topic_vane_command, 128, "%s/vane/set", mqtt_topic_prefix);

    mqttClient.setServer(mqtt_host, atoi(mqtt_port));
    if (strlen(mqtt_username) > 0) {
        mqttClient.setCredentials(mqtt_username, mqtt_password);
    }
    mqttClient.onMessage(mqttMessage);
    mqttClient.onConnect(mqttConnect);
    mqttClient.connect();

    heatpump.setSettingsChangedCallback(heatpumpSettingsChanged);
    heatpump.setStatusChangedCallback(heatpumpStatusChanged);

    #ifdef HEATPUMP_ENABLE
    heatpump.connect(&Serial);
    #endif
}

void loop() {
    #ifdef HEATPUMP_ENABLE
    heatpump.sync();
    #endif
}
