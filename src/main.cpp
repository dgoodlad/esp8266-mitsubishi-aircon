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

// Detects the heatpump by looking for a HIGH signal on HEATPUMP_DETECT_PIN,
// then configures the system for either condition. If no heatpump is detected,
// we assume that we're connected via FTDI to a computer for development, so
// re-route all debug data back out the default UART pins.
bool detectHeatpump() {
    bool detected;

    pinMode(HEATPUMP_DETECT_PIN, INPUT);
    pinMode(HEATPUMP_ENABLE_PIN, OUTPUT);

    detected = digitalRead(HEATPUMP_DETECT_PIN) == HIGH;

    if (detected) {
        // Output debug info on GPIO2 via UART1
        DebugSerial = &Serial1;
        Serial1.begin(115200);
        Serial1.setDebugOutput(true);

        // Enable the logic level shifter now that we know there's 5V on the far
        // side and that we'll have something to talk to
        digitalWrite(HEATPUMP_ENABLE_PIN, HIGH);
    } else {
        // Make sure the logic level shifter remains disabled, as it won't be
        // getting any 5V power anyway
        digitalWrite(HEATPUMP_ENABLE_PIN, LOW);

        // Output debug info on the default serial TX/RX pins via UART0
        DebugSerial = &Serial;
        Serial.begin(115200);
        Serial.setDebugOutput(true);
    }

    return detected;
}

void saveConfigCallback () {
    shouldSaveConfig = true;
}

void heatpumpSettingsChanged() {
    char temperature[4];
    heatpumpSettings settings = heatpump.getSettings();
    snprintf(temperature, 4, "%3.0f", settings.temperature);
    DebugSerial->printf("PUB power state %s\n", settings.power);
    mqttClient.publish(mqtt_topic_power_state, 0, true, settings.power);
    if (heatpump.getPowerSettingBool()) {
        DebugSerial->printf("PUB mode state %s\n", settings.mode);
        mqttClient.publish(mqtt_topic_mode_state, 0, true, settings.mode);
    } else {
        DebugSerial->printf("PUB mode state OFF\n");
        mqttClient.publish(mqtt_topic_mode_state, 0, true, "OFF");
    }
    DebugSerial->printf("PUB temperature %s\n", temperature);
    mqttClient.publish(mqtt_topic_temperature_state, 0, true, temperature);
    DebugSerial->printf("PUB fan state %s\n", settings.fan);
    mqttClient.publish(mqtt_topic_fan_state, 0, true, settings.fan);
    DebugSerial->printf("PUB vane state %s\n", settings.vane);
    mqttClient.publish(mqtt_topic_vane_state, 0, true, settings.vane);
}

void heatpumpStatusChanged(heatpumpStatus status) {
    char temperature[4];
    snprintf(temperature, 4, "%3.0f", status.roomTemperature);
    DebugSerial->printf("PUB temperature %s\n", temperature);
    mqttClient.publish(mqtt_topic_current_temperature_state, 0, true, temperature);
}

void mqttConnect(bool sessionPresent) {
    mqttClient.subscribe(mqtt_topic_power_command, 0);
    mqttClient.subscribe(mqtt_topic_mode_command, 0);
    mqttClient.subscribe(mqtt_topic_temperature_command, 0);
    mqttClient.subscribe(mqtt_topic_fan_command, 0);
    mqttClient.subscribe(mqtt_topic_vane_command, 0);
    mqttClient.publish(mqtt_topic_availability, 2, true, "online");
}

bool validatePowerValue(const char* value) {
    return
        strcmp(value, "ON") == 0 ||
        strcmp(value, "OFF") == 0;
}

bool validateModeValue(const char* value) {
    return
        strcmp(value, "HEAT") == 0 ||
        strcmp(value, "DRY") == 0 ||
        strcmp(value, "COOL") == 0 ||
        strcmp(value, "FAN") == 0 ||
        strcmp(value, "AUTO") == 0;
}

bool validateTemperatureValue(const char* value) {
    float f = atof(value);
    return f > 15.0 && f < 31;
}

bool validateFanValue(const char* value) {
    return
        strcmp(value, "AUTO") == 0 ||
        strcmp(value, "QUIET") == 0 ||
        strcmp(value, "1") == 0 ||
        strcmp(value, "2") == 0 ||
        strcmp(value, "3") == 0 ||
        strcmp(value, "4") == 0;
}

bool validateVaneValue(const char* value) {
    return
        strcmp(value, "AUTO") == 0 ||
        strcmp(value, "1") == 0 ||
        strcmp(value, "2") == 0 ||
        strcmp(value, "3") == 0 ||
        strcmp(value, "4") == 0 ||
        strcmp(value, "5") == 0 ||
        strcmp(value, "SWING") == 0;
}

void upcase(const char* s, size_t s_len, char* buf, size_t buf_len) {
    size_t i = s_len;
    if (buf_len - 1 < i) { i = buf_len - 1; }
    while (*s && i-- > 0) {
        *buf++ = toupper(*s++);
    }
    *buf = '\0';
}

void mqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
    char buffer[6];
    size_t buflen = 6;

    if(strcmp(topic, mqtt_topic_power_command) == 0) {
        upcase(payload, len, buffer, buflen);
        DebugSerial->printf("SET power setting to %s\n", buffer);
        if (validatePowerValue(buffer)) {
            heatpump.setPowerSetting(buffer);
        }
    } else if(strcmp(topic, mqtt_topic_mode_command) == 0) {
        upcase(payload, len, buffer, buflen);
        DebugSerial->printf("SET mode setting to %s\n", buffer);
        if (validateModeValue(buffer)) {
            heatpump.setModeSetting(buffer);
        }
    } else if(strcmp(topic, mqtt_topic_temperature_command) == 0) {
        upcase(payload, len, buffer, buflen);
        DebugSerial->printf("SET temperature to %f\n", atof(buffer));
        if (validateTemperatureValue(buffer)) {
            heatpump.setTemperature(atof(buffer));
        }
    } else if(strcmp(topic, mqtt_topic_fan_command) == 0) {
        upcase(payload, len, buffer, buflen);
        DebugSerial->printf("SET fan speed to %s\n", buffer);
        if (validateFanValue(buffer)) {
            heatpump.setFanSpeed(buffer);
        }
    } else if(strcmp(topic, mqtt_topic_vane_command) == 0) {
        upcase(payload, len, buffer, buflen);
        DebugSerial->printf("SET vane to %s\n", buffer);
        if (validateVaneValue(buffer)) {
            heatpump.setVaneSetting(buffer);
        }
    }
    heatpump.update();
}

void setupClearSettingsButtonHandler() {
    clearSettingsButton.attach(CLEAR_SETTINGS_PIN, INPUT);
}

void loadConfig() {
    if (SPIFFS.begin()) {
        if (SPIFFS.exists(CONFIG_SPIFFS_PATH)) {
            DebugSerial->printf("Found config at %s", CONFIG_SPIFFS_PATH);
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
        DebugSerial->printf("Error mounting SPIFFS");
    }
}

void setupWifiManager() {
    custom_mqtt_host = new WiFiManagerParameter("mqtt_host", "MQTT Host", settings.mqtt_host, MAX_LENGTH_MQTT_HOST);
    custom_mqtt_port = new WiFiManagerParameter("mqtt_port", "MQTT Port", settings.mqtt_port, MAX_LENGTH_MQTT_PORT);
    custom_mqtt_username = new WiFiManagerParameter("mqtt_username", "MQTT Username", settings.mqtt_username, MAX_LENGTH_MQTT_USERNAME);
    custom_mqtt_password = new WiFiManagerParameter("mqtt_password", "MQTT Password", settings.mqtt_password, MAX_LENGTH_MQTT_PASSWORD);
    custom_mqtt_topic_prefix = new WiFiManagerParameter("mqtt_topic_prefis", "MQTT Topic Prefix", settings.mqtt_topic_prefix, MAX_LENGTH_MQTT_TOPIC_PREFIX);

    wifiManager = new WiFiManager(*DebugSerial);

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
    DebugSerial->printf("Starting wifimanager with AP %s", config_ap_name);

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
    json.printTo(*DebugSerial);
    configFile.close();
}

void handleClearSettingsButton() {
    clearSettingsButton.update();
    if (clearSettingsButton.read() == LOW && clearSettingsButton.duration() > 3000) {
        DebugSerial->println("Resetting to factory settings");
        wifiManager->resetSettings();
        SPIFFS.remove(CONFIG_SPIFFS_PATH);
        DebugSerial->println("Restarting...");
        ESP.restart();
        delay(5000);
    }
}

void setup() {
    heatPumpDetected = detectHeatpump();

    DebugSerial->println("\n Starting up...");
    if (heatPumpDetected) {
        DebugSerial->println("Heatpump DETECTED");
    } else {
        DebugSerial->println("Heatpump NOT DETECTED");
    }

    DebugSerial->println("Configuring debounce handler for CLEAR button");
    setupClearSettingsButtonHandler();
    DebugSerial->println("Loading configuration");
    loadConfig();
    DebugSerial->println("Setting up the wifi manager");
    setupWifiManager();

    if (!startWifiManager()) {
        DebugSerial->println("Failed to connect or timed out. Restarting in 3s...");
        delay(3000);
        ESP.restart();
        delay(5000);
    } else {
        DebugSerial->println("Connected to wifi");
    }

    // Successfully connected to wifi

    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else {
            type = "filesystem";
        }

        DebugSerial->println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        DebugSerial->println("\nEnd updating");
        delay(500);
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        if (progress > 0 && total > 0)
            DebugSerial->printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        DebugSerial->printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            DebugSerial->println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            DebugSerial->println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            DebugSerial->println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            DebugSerial->println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            DebugSerial->println("End Failed");
        }
    });
    ArduinoOTA.setPassword(otaPassword);
    ArduinoOTA.begin();

    strncpy(settings.mqtt_host, custom_mqtt_host->getValue(), MAX_LENGTH_MQTT_HOST);
    strncpy(settings.mqtt_port, custom_mqtt_port->getValue(), MAX_LENGTH_MQTT_PORT);
    strncpy(settings.mqtt_username, custom_mqtt_username->getValue(), MAX_LENGTH_MQTT_USERNAME);
    strncpy(settings.mqtt_password, custom_mqtt_password->getValue(), MAX_LENGTH_MQTT_PASSWORD);
    strncpy(settings.mqtt_topic_prefix, custom_mqtt_topic_prefix->getValue(), MAX_LENGTH_MQTT_TOPIC_PREFIX);

    if (shouldSaveConfig) {
        saveConfig();
    }

    snprintf(mqtt_topic_availability, 128, "%s/availability", settings.mqtt_topic_prefix);
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
    mqttClient.setWill(mqtt_topic_availability, 2, true, "offline");
    mqttClient.connect();

    heatpump.setSettingsChangedCallback(heatpumpSettingsChanged);
    heatpump.setStatusChangedCallback(heatpumpStatusChanged);

    if (heatPumpDetected) {
        // Talk to the heatpump on GPIO13/15 via UART0 by telling the heatpump
        // class to call `Serial.swap()`
        heatpump.connect(&Serial, true);
    }
}

void loop() {
    ArduinoOTA.handle();

    if (heatPumpDetected) {
        heatpump.sync();
    }

    handleClearSettingsButton();
}
