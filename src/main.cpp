#include "main.hpp"

#include <Arduino.h>
#include <FS.h>

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <AsyncMqttClient.h>

#include <WiFiManager.h>

#include <ArduinoJson.h>

#include <HeatPump.h>

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
    Serial.printf("PUB power state %s\n", settings.power.c_str());
    mqttClient.publish(mqtt_topic_power_state, 0, true, settings.power.c_str());
    Serial.printf("PUB mode state %s\n", settings.mode.c_str());
    mqttClient.publish(mqtt_topic_mode_state, 0, true, settings.mode.c_str());
    Serial.printf("PUB temperature %s\n", temperature);
    mqttClient.publish(mqtt_topic_temperature_state, 0, true, temperature);
    Serial.printf("PUB fan state %s\n", settings.fan.c_str());
    mqttClient.publish(mqtt_topic_fan_state, 0, true, settings.fan.c_str());
    Serial.printf("PUB vane state %s\n", settings.vane.c_str());
    mqttClient.publish(mqtt_topic_vane_state, 0, true, settings.vane.c_str());
}

void heatpumpStatusChanged(heatpumpStatus status) {
    char temperature[4];
    snprintf(temperature, 4, "%3.0f", status.roomTemperature);
    Serial.printf("PUB temperature %s\n", temperature);
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
        Serial.printf("SET power setting to %s\n", payload);
        heatpump.setPowerSetting(payload);
    } else if(strcmp(topic, mqtt_topic_mode_command)) {
        Serial.printf("SET mode setting to %s\n", payload);
        heatpump.setModeSetting(payload);
    } else if(strcmp(topic, mqtt_topic_temperature_command)) {
        Serial.printf("SET temperature to %f\n", atof(payload));
        heatpump.setTemperature(atof(payload));
    } else if(strcmp(topic, mqtt_topic_fan_command)) {
        Serial.printf("SET fan speed to %s\n", payload);
        heatpump.setFanSpeed(payload);
    } else if(strcmp(topic, mqtt_topic_vane_command)) {
        Serial.printf("SET vane to %s\n", payload);
        heatpump.setVaneSetting(payload);
    }
}

void setupButtonHandler() {
    button.attach(BUTTON_PIN, BUTTON_MODE);
}

void loadConfig() {
    if (SPIFFS.begin()) {
        if (SPIFFS.exists(CONFIG_SPIFFS_PATH)) {
            Serial.printf("Found config at %s", CONFIG_SPIFFS_PATH);
            File configFile = SPIFFS.open(CONFIG_SPIFFS_PATH, "r");
            if (configFile) {
                size_t size = configFile.size();
                std::unique_ptr<char[]> buf(new char[size]);
                configFile.readBytes(buf.get(), size);
                DynamicJsonBuffer jsonBuffer;
                JsonObject& json = jsonBuffer.parseObject(buf.get());
                if (json.success()) {
                    strncpy(settings.mqtt_host, json["mqtt_host"], MAX_LENGTH_MQTT_HOST);
                    strncpy(settings.mqtt_port, json["mqtt_port"], MAX_LENGTH_MQTT_PORT);
                    strncpy(settings.mqtt_username, json["mqtt_username"], MAX_LENGTH_MQTT_USERNAME);
                    strncpy(settings.mqtt_password, json["mqtt_password"], MAX_LENGTH_MQTT_PASSWORD);
                    strncpy(settings.mqtt_topic_prefix, json["mqtt_topic_prefix"], MAX_LENGTH_MQTT_TOPIC_PREFIX);
                } else {
                    // Failed to load json config
                }
                configFile.close();
            }
        }
    } else {
        // Failed to mount FS
        Serial.printf("Error mounting SPIFFS");
    }
}

void setupWifiManager() {
    custom_mqtt_host = new WiFiManagerParameter("mqtt_host", "MQTT Host", settings.mqtt_host, MAX_LENGTH_MQTT_HOST);
    custom_mqtt_port = new WiFiManagerParameter("mqtt_port", "MQTT Port", settings.mqtt_port, MAX_LENGTH_MQTT_PORT);
    custom_mqtt_username = new WiFiManagerParameter("mqtt_username", "MQTT Username", settings.mqtt_username, MAX_LENGTH_MQTT_USERNAME);
    custom_mqtt_password = new WiFiManagerParameter("mqtt_password", "MQTT Password", settings.mqtt_password, MAX_LENGTH_MQTT_PASSWORD);
    custom_mqtt_topic_prefix = new WiFiManagerParameter("mqtt_topic_prefis", "MQTT Topic Prefix", settings.mqtt_topic_prefix, MAX_LENGTH_MQTT_TOPIC_PREFIX);

    wifiManager = new WiFiManager();

    wifiManager->setSaveConfigCallback(saveConfigCallback);
    wifiManager->addParameter(custom_mqtt_host);
    wifiManager->addParameter(custom_mqtt_port);
    wifiManager->addParameter(custom_mqtt_username);
    wifiManager->addParameter(custom_mqtt_password);
    wifiManager->addParameter(custom_mqtt_topic_prefix);

    // Dark theme
    wifiManager->setClass("invert");
}

bool startWifiManager() {
    char config_ap_name[17];
    snprintf(config_ap_name, 17, "ESP8266 %08x", ESP.getChipId());
    Serial.printf("Starting wifimanager with AP %s", config_ap_name);

    return wifiManager->autoConnect(config_ap_name, WIFIMANAGER_AP_PASSWORD);
}

void saveConfig() {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_host"] = settings.mqtt_host;
    json["mqtt_port"] = settings.mqtt_port;
    json["mqtt_username"] = settings.mqtt_username;
    json["mqtt_password"] = settings.mqtt_password;
    json["mqtt_topic_prefix"] = settings.mqtt_topic_prefix;

    File configFile = SPIFFS.open(CONFIG_SPIFFS_PATH, "w");
    if (!configFile) {
        // Failed to open config file
    }

    json.printTo(configFile);
    json.printTo(Serial);
    configFile.close();
}

void handleButton() {
    button.update();
    if (button.read() == LOW && button.duration() > 3000) {
        Serial.println("Resetting to factory settings");
        wifiManager->resetSettings();
        SPIFFS.remove(CONFIG_SPIFFS_PATH);
        Serial.println("Restarting...");
        ESP.restart();
        delay(5000);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    delay(3000);
    Serial.println("\n Starting up...");

    setupButtonHandler();
    loadConfig();
    setupWifiManager();

    if (!startWifiManager()) {
        Serial.println("Failed to connect or timed out");
        delay(3000);
        ESP.restart();
        delay(5000);
    } else {
        Serial.println("Connected to wifi");
    }

    // Successfully connected to wifi

    strncpy(settings.mqtt_host, custom_mqtt_host->getValue(), MAX_LENGTH_MQTT_HOST);
    strncpy(settings.mqtt_port, custom_mqtt_port->getValue(), MAX_LENGTH_MQTT_PORT);
    strncpy(settings.mqtt_username, custom_mqtt_username->getValue(), MAX_LENGTH_MQTT_USERNAME);
    strncpy(settings.mqtt_password, custom_mqtt_password->getValue(), MAX_LENGTH_MQTT_PASSWORD);
    strncpy(settings.mqtt_topic_prefix, custom_mqtt_topic_prefix->getValue(), MAX_LENGTH_MQTT_TOPIC_PREFIX);

    if (shouldSaveConfig) {
        saveConfig();
    }

    snprintf(mqtt_topic_power_state, 128, "%s/power/state", settings.mqtt_topic_prefix);
    snprintf(mqtt_topic_mode_state, 128, "%s/mode/state", settings.mqtt_topic_prefix);
    snprintf(mqtt_topic_temperature_state, 128, "%s/temperature/state", settings.mqtt_topic_prefix);
    snprintf(mqtt_topic_fan_state, 128, "%s/fan/state", settings.mqtt_topic_prefix);
    snprintf(mqtt_topic_vane_state, 128, "%s/vane/state", settings.mqtt_topic_prefix);
    snprintf(mqtt_topic_current_temperature_state, 128, "%s/current_temperature/state", settings.mqtt_topic_prefix);

    snprintf(mqtt_topic_power_command, 128, "%s/power/set", settings.mqtt_topic_prefix);
    snprintf(mqtt_topic_mode_command, 128, "%s/mode/set", settings.mqtt_topic_prefix);
    snprintf(mqtt_topic_temperature_command, 128, "%s/temperature/set", settings.mqtt_topic_prefix);
    snprintf(mqtt_topic_fan_command, 128, "%s/fan/set", settings.mqtt_topic_prefix);
    snprintf(mqtt_topic_vane_command, 128, "%s/vane/set", settings.mqtt_topic_prefix);

    mqttClient.setServer(settings.mqtt_host, atoi(settings.mqtt_port));
    if (strlen(settings.mqtt_username) > 0) {
        mqttClient.setCredentials(settings.mqtt_username, settings.mqtt_password);
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

    handleButton();
}
