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

WiFiUDP udpClient;
bool syslogEnabled = false;
Syslog syslog(udpClient, SYSLOG_PROTO_IETF);

AsyncMqttClient mqttClient;

HeatPump heatpump;

#ifdef HARDWARE_V02
// Detects the heatpump by looking for a HIGH signal on HEATPUMP_DETECT_PIN,
// then configures the system for either condition. If no heatpump is detected,
// we assume that we're connected via FTDI to a computer for development, so
// re-route all debug data back out the default UART pins.
bool detectHeatpump() {
    bool detected;

    if (syslogEnabled) syslog.log(LOG_INFO, "Detecting heatpump (HARDWARE_V02)");

    pinMode(HEATPUMP_DETECT_PIN, INPUT);
    pinMode(HEATPUMP_ENABLE_PIN, OUTPUT);

    detected = digitalRead(HEATPUMP_DETECT_PIN) == HIGH;

    if (detected) {
        if (syslogEnabled) syslog.log(LOG_INFO, "Heatpump detected? YES.");
        // Output debug info on GPIO2 via UART1
        if (syslogEnabled) syslog.log(LOG_INFO, "Configuring debug info on UART1");
        DebugSerial = &Serial1;
        Serial1.begin(115200);
        Serial1.setDebugOutput(true);

        // Enable the logic level shifter now that we know there's 5V on the far
        // side and that we'll have something to talk to
        if (syslogEnabled) syslog.log(LOG_INFO, "Enabling logic level shifter");
        digitalWrite(HEATPUMP_ENABLE_PIN, HIGH);
    } else {
        if (syslogEnabled) syslog.log(LOG_INFO, "Heatpump detected? NO.");
        // Make sure the logic level shifter remains disabled, as it won't be
        // getting any 5V power anyway
        if (syslogEnabled) syslog.log(LOG_INFO, "Disabling logic level shifter");
        digitalWrite(HEATPUMP_ENABLE_PIN, LOW);

        // Output debug info on the default serial TX/RX pins via UART0
        if (syslogEnabled) syslog.log(LOG_INFO, "Configuring debug info on default pins of UART0");
        DebugSerial = &Serial;
        Serial.begin(115200);
        Serial.setDebugOutput(true);
    }

    return detected;
}
#endif

#ifdef HARDWARE_V01
bool detectHeatpump() {
    if (syslogEnabled) syslog.log(LOG_INFO, "Detecting heatpump (HARDWARE_V01)");
    DebugSerial = &Serial1;
    Serial1.begin(115200);
    Serial1.setDebugOutput(true);

    if (syslogEnabled) syslog.log(LOG_INFO, "Enabling logic level shifter");
    pinMode(HEATPUMP_ENABLE_PIN, OUTPUT);
    digitalWrite(HEATPUMP_ENABLE_PIN, HIGH);

    return true;
}
#endif

void saveConfigCallback () {
    shouldSaveConfig = true;
}

void heatpumpPacketCallback(byte *packet, int length, char* message) {
    char buffer[256];
    int offset = 0;

    offset = snprintf(buffer, 256, "%s %d bytes: ", message, length);
    for (int i = 0; i < length; i++) {
        offset += snprintf(buffer + offset, 256 - offset, "%02X", *(packet + i));
    }

    if (syslogEnabled) syslog.log(LOG_DEBUG, buffer);
    DebugSerial->println(buffer);
}

void heatpumpSettingsChanged() {
    char temperature[4];
    heatpumpSettings settings = heatpump.getSettings();
    if (syslogEnabled) syslog.log(LOG_INFO, "heatpumpSettingsChanged callback");
    snprintf(temperature, 4, "%3.0f", settings.temperature);
    if (syslogEnabled) syslog.logf(LOG_DEBUG, "PUB power state %s to %s", settings.power, mqtt_topic_power_state);
    DebugSerial->printf("PUB power state %s\n", settings.power);
    mqttClient.publish(mqtt_topic_power_state, 0, true, settings.power);
    if (heatpump.getPowerSettingBool()) {
        if (syslogEnabled) syslog.logf(LOG_DEBUG, "PUB mode state %s to %s", settings.mode, mqtt_topic_mode_state);
        DebugSerial->printf("PUB mode state %s\n", settings.mode);
        mqttClient.publish(mqtt_topic_mode_state, 0, true, settings.mode);
    } else {
        if (syslogEnabled) syslog.logf(LOG_DEBUG, "PUB mode state OFF to %s", mqtt_topic_mode_state);
        DebugSerial->printf("PUB mode state OFF\n");
        mqttClient.publish(mqtt_topic_mode_state, 0, true, "OFF");
    }
    if (syslogEnabled) syslog.logf(LOG_DEBUG, "PUB temperature %s to %s", temperature, mqtt_topic_temperature_state);
    DebugSerial->printf("PUB temperature %s\n", temperature);
    mqttClient.publish(mqtt_topic_temperature_state, 0, true, temperature);
    if (syslogEnabled) syslog.logf(LOG_DEBUG, "PUB fan state %s to %s", settings.fan, mqtt_topic_fan_state);
    DebugSerial->printf("PUB fan state %s\n", settings.fan);
    mqttClient.publish(mqtt_topic_fan_state, 0, true, settings.fan);
    if (syslogEnabled) syslog.logf(LOG_DEBUG, "PUB vane state %s to %s", settings.vane, mqtt_topic_vane_state);
    DebugSerial->printf("PUB vane state %s\n", settings.vane);
    mqttClient.publish(mqtt_topic_vane_state, 0, true, settings.vane);
}

void heatpumpStatusChanged(heatpumpStatus status) {
    char temperature[4];
    if (syslogEnabled) syslog.log(LOG_INFO, "heatpumpStatusChanged callback");
    snprintf(temperature, 4, "%3.0f", status.roomTemperature);
    if (syslogEnabled) syslog.logf(LOG_DEBUG, "PUB room temperature %s to %s", temperature, mqtt_topic_current_temperature_state);
    DebugSerial->printf("PUB temperature %s\n", temperature);
    mqttClient.publish(mqtt_topic_current_temperature_state, 0, true, temperature);
}

void mqttConnect(bool sessionPresent) {
    if (syslogEnabled) syslog.log(LOG_INFO, "mqttConnect callback");
    mqttClient.subscribe(mqtt_topic_power_command, 0);
    mqttClient.subscribe(mqtt_topic_mode_command, 0);
    mqttClient.subscribe(mqtt_topic_temperature_command, 0);
    mqttClient.subscribe(mqtt_topic_fan_command, 0);
    mqttClient.subscribe(mqtt_topic_vane_command, 0);
    mqttClient.publish(mqtt_topic_availability, 2, true, "online");
    publishSystemBootInfo();
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

    if (syslogEnabled) syslog.log(LOG_INFO, "mqttMessage callback");

    if(strcmp(topic, mqtt_topic_power_command) == 0) {
        upcase(payload, len, buffer, buflen);
        if (syslogEnabled) syslog.logf(LOG_INFO, "SET power setting to %s", buffer);
        DebugSerial->printf("SET power setting to %s\n", buffer);
        if (validatePowerValue(buffer)) {
            heatpump.setPowerSetting(buffer);
            updateHeatpump = true;
            lastHeatpumpSettingsChange = millis();
        }
    } else if(strcmp(topic, mqtt_topic_mode_command) == 0) {
        upcase(payload, len, buffer, buflen);
        if (syslogEnabled) syslog.logf(LOG_INFO, "SET mode setting to %s", buffer);
        DebugSerial->printf("SET mode setting to %s\n", buffer);
        if (validateModeValue(buffer)) {
            heatpump.setModeSetting(buffer);
            updateHeatpump = true;
            lastHeatpumpSettingsChange = millis();
        }
    } else if(strcmp(topic, mqtt_topic_temperature_command) == 0) {
        upcase(payload, len, buffer, buflen);
        if (syslogEnabled) syslog.logf(LOG_INFO, "SET temperature to %f", atof(buffer));
        DebugSerial->printf("SET temperature to %f\n", atof(buffer));
        if (validateTemperatureValue(buffer)) {
            heatpump.setTemperature(atof(buffer));
            updateHeatpump = true;
            lastHeatpumpSettingsChange = millis();
        }
    } else if(strcmp(topic, mqtt_topic_fan_command) == 0) {
        upcase(payload, len, buffer, buflen);
        if (syslogEnabled) syslog.logf(LOG_INFO, "SET fan speed to %s", buffer);
        DebugSerial->printf("SET fan speed to %s\n", buffer);
        if (validateFanValue(buffer)) {
            heatpump.setFanSpeed(buffer);
            updateHeatpump = true;
            lastHeatpumpSettingsChange = millis();
        }
    } else if(strcmp(topic, mqtt_topic_vane_command) == 0) {
        upcase(payload, len, buffer, buflen);
        if (syslogEnabled) syslog.logf(LOG_INFO, "SET vane to %s", buffer);
        DebugSerial->printf("SET vane to %s\n", buffer);
        if (validateVaneValue(buffer)) {
            heatpump.setVaneSetting(buffer);
            updateHeatpump = true;
            lastHeatpumpSettingsChange = millis();
        }
    }
}

void setupClearSettingsButtonHandler() {
    #ifdef CLEAR_SETTINGS_PIN
    if (syslogEnabled) syslog.logf("Setting up clear-settings pin %d", CLEAR_SETTINGS_PIN);
    clearSettingsButton.attach(CLEAR_SETTINGS_PIN, INPUT_PULLUP);
    #endif
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
                    if (json.containsKey("mqtt_host"))
                        strncpy(settings.mqtt_host, json["mqtt_host"], MAX_LENGTH_MQTT_HOST);
                    if (json.containsKey("mqtt_port"))
                        strncpy(settings.mqtt_port, json["mqtt_port"], MAX_LENGTH_MQTT_PORT);
                    if (json.containsKey("mqtt_username"))
                        strncpy(settings.mqtt_username, json["mqtt_username"], MAX_LENGTH_MQTT_USERNAME);
                    if (json.containsKey("mqtt_password"))
                        strncpy(settings.mqtt_password, json["mqtt_password"], MAX_LENGTH_MQTT_PASSWORD);
                    if (json.containsKey("mqtt_topic_prefix"))
                        strncpy(settings.mqtt_topic_prefix, json["mqtt_topic_prefix"], MAX_LENGTH_MQTT_TOPIC_PREFIX);
                    if (json.containsKey("syslog_host"))
                        strncpy(settings.syslog_host, json["syslog_host"], MAX_LENGTH_SYSLOG_HOST);
                    if (json.containsKey("syslog_port"))
                        strncpy(settings.syslog_port, json["syslog_port"], MAX_LENGTH_SYSLOG_PORT);
                    if (json.containsKey("syslog_device_hostname"))
                        strncpy(settings.syslog_device_hostname, json["syslog_device_hostname"], MAX_LENGTH_SYSLOG_DEVICE_HOSTNAME);
                    if (json.containsKey("syslog_app_name"))
                        strncpy(settings.syslog_app_name, json["syslog_app_name"], MAX_LENGTH_SYSLOG_APP_NAME);
                    if (json.containsKey("syslog_log_level"))
                        strncpy(settings.syslog_log_level, json["syslog_log_level"], MAX_LENGTH_SYSLOG_LOG_LEVEL);
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
    wifiManager = new WiFiManager(*DebugSerial);

    custom_mqtt_host = new WiFiManagerParameter("mqtt_host", "MQTT Host", settings.mqtt_host, MAX_LENGTH_MQTT_HOST);
    custom_mqtt_port = new WiFiManagerParameter("mqtt_port", "MQTT Port", settings.mqtt_port, MAX_LENGTH_MQTT_PORT);
    custom_mqtt_username = new WiFiManagerParameter("mqtt_username", "MQTT Username", settings.mqtt_username, MAX_LENGTH_MQTT_USERNAME);
    custom_mqtt_password = new WiFiManagerParameter("mqtt_password", "MQTT Password", settings.mqtt_password, MAX_LENGTH_MQTT_PASSWORD);
    custom_mqtt_topic_prefix = new WiFiManagerParameter("mqtt_topic_prefis", "MQTT Topic Prefix", settings.mqtt_topic_prefix, MAX_LENGTH_MQTT_TOPIC_PREFIX);
    custom_syslog_host = new WiFiManagerParameter("syslog_host", "SYSLOG Host", settings.syslog_host, MAX_LENGTH_SYSLOG_HOST);
    custom_syslog_port = new WiFiManagerParameter("syslog_port", "SYSLOG Port", settings.syslog_port, MAX_LENGTH_SYSLOG_PORT);
    custom_syslog_device_hostname = new WiFiManagerParameter("syslog_device_hostname", "Syslog Device Hostname", settings.syslog_device_hostname, MAX_LENGTH_SYSLOG_DEVICE_HOSTNAME);
    custom_syslog_app_name = new WiFiManagerParameter("syslog_app_name", "Syslog App Name", settings.syslog_app_name, MAX_LENGTH_SYSLOG_APP_NAME);
    custom_syslog_log_level = new WiFiManagerParameter("syslog_log_level", "Syslog Log Level", settings.syslog_log_level, MAX_LENGTH_SYSLOG_LOG_LEVEL);

    // Configure custom parameters for wifimanager to collect in the
    // configuration page
    wifiManager->addParameter(custom_mqtt_host);
    wifiManager->addParameter(custom_mqtt_port);
    wifiManager->addParameter(custom_mqtt_username);
    wifiManager->addParameter(custom_mqtt_password);
    wifiManager->addParameter(custom_mqtt_topic_prefix);
    wifiManager->addParameter(custom_syslog_host);
    wifiManager->addParameter(custom_syslog_port);
    wifiManager->addParameter(custom_syslog_device_hostname);
    wifiManager->addParameter(custom_syslog_app_name);
    wifiManager->addParameter(custom_syslog_log_level);
    wifiManager->setSaveParamsCallback(saveConfigCallback);

    // Timeout after 5 minutes so that a "blip" in the wifi doesn't leave the
    // device hung in captive portal mode
    wifiManager->setConfigPortalTimeout(5 * 60);

    // Enable WifiManager's debug output
    //wifiManager->setDebugOutput(true);

    // Restore built-in ESP8266 persistent wifi settings
    wifiManager->setRestorePersistent(true);

    // Automatically attempt to reconnect wifi
    wifiManager->setWiFiAutoReconnect(true);

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
    json["syslog_host"] = settings.syslog_host;
    json["syslog_port"] = settings.syslog_port;
    json["syslog_device_hostname"] = settings.syslog_device_hostname;
    json["syslog_app_name"] = settings.syslog_app_name;
    json["syslog_log_level"] = settings.syslog_log_level;

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
        if (syslogEnabled) syslog.log(LOG_WARNING, "Resetting to factory settings");
        DebugSerial->println("Resetting to factory settings");
        wifiManager->resetSettings();
        SPIFFS.remove(CONFIG_SPIFFS_PATH);
        if (syslogEnabled) syslog.log(LOG_WARNING, "Restarting...");
        DebugSerial->println("Restarting...");
        ESP.restart();
        delay(5000);
    }
}

void publishSystemBootInfo() {
    char buffer[256];
    rst_info *reset_info = ESP.getResetInfoPtr();
    snprintf(buffer, 256, "Reset reason: %s", ESP.getResetReason().c_str());
    mqttClient.publish(mqtt_topic_info, 0, false, buffer);
    if (reset_info->reason == REASON_EXCEPTION_RST) {
        snprintf(buffer, 256, "Fatal exception: (%d):\n", reset_info->exccause);
        if (syslogEnabled) syslog.log(LOG_INFO, buffer);
        mqttClient.publish(mqtt_topic_info, 0, false, buffer);
        snprintf(buffer, 256, "epc1=0x%08x, epc2=0x%08x, epc3=0x%08x, excvaddr=0x%08x, depc=0x%08x",
                 reset_info->epc1,
                 reset_info->epc2,
                 reset_info->epc3,
                 reset_info->excvaddr,
                 reset_info->depc);
        if (syslogEnabled) syslog.log(LOG_INFO, buffer);
        mqttClient.publish(mqtt_topic_info, 0, false, buffer);
    }
    snprintf(buffer, 256, "Chip ID: %08x", ESP.getChipId());
    if (syslogEnabled) syslog.log(LOG_INFO, buffer);
    mqttClient.publish(mqtt_topic_info, 0, false, buffer);
    snprintf(buffer, 256, "Core: %s\nSDK: %s\nCPU Frequency: %d MHz", ESP.getCoreVersion().c_str(), ESP.getSdkVersion(), ESP.getCpuFreqMHz());
    if (syslogEnabled) syslog.log(LOG_INFO, buffer);
    mqttClient.publish(mqtt_topic_info, 0, false, buffer);
    snprintf(buffer, 256, "Sketch size: %d\nSketch free: %d\nSketch MD5: %s", ESP.getSketchSize(), ESP.getFreeSketchSpace(), ESP.getSketchMD5().c_str());
    if (syslogEnabled) syslog.log(LOG_INFO, buffer);
    mqttClient.publish(mqtt_topic_info, 0, false, buffer);
}

void publishSystemStatus() {
    char buffer[256];
    snprintf(buffer, 256, "Uptime: %d mins\nFree heap: %d", (int) (millis() / 60000), ESP.getFreeHeap());
    if (syslogEnabled) syslog.log(LOG_INFO, buffer);
    mqttClient.publish(mqtt_topic_info, 0, false, buffer);
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

        if (syslogEnabled) syslog.logf(LOG_WARNING, "OTA Update starting (%s)", type.c_str());
        DebugSerial->println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        if (syslogEnabled) syslog.log(LOG_WARNING, "OTA Update: COMPLETE");
        DebugSerial->println("\nEnd updating");
        delay(500);
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        if (progress > 0 && total > 0) {
            if (syslogEnabled) syslog.logf(LOG_WARNING, "OTA Update Progress: %u%%", (progress / (total / 100)));
            DebugSerial->printf("Progress: %u%%\r", (progress / (total / 100)));
        }
    });
    ArduinoOTA.onError([](ota_error_t error) {
        DebugSerial->printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            if (syslogEnabled) syslog.log(LOG_ERR, "OTA Update Error: Auth Failed");
            DebugSerial->println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            if (syslogEnabled) syslog.log(LOG_ERR, "OTA Update Error: Begin Failed");
            DebugSerial->println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            if (syslogEnabled) syslog.log(LOG_ERR, "OTA Update Error: Connect Failed");
            DebugSerial->println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            if (syslogEnabled) syslog.log(LOG_ERR, "OTA Update Error: Receive Failed");
            DebugSerial->println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            if (syslogEnabled) syslog.log(LOG_ERR, "OTA Update Error: End Failed");
            DebugSerial->println("End Failed");
        }
    });
    ArduinoOTA.setPassword(otaPassword);
    ArduinoOTA.begin();

    if (syslogEnabled) syslog.log(LOG_INFO, "Loading config");

    strncpy(settings.mqtt_host, custom_mqtt_host->getValue(), MAX_LENGTH_MQTT_HOST);
    strncpy(settings.mqtt_port, custom_mqtt_port->getValue(), MAX_LENGTH_MQTT_PORT);
    strncpy(settings.mqtt_username, custom_mqtt_username->getValue(), MAX_LENGTH_MQTT_USERNAME);
    strncpy(settings.mqtt_password, custom_mqtt_password->getValue(), MAX_LENGTH_MQTT_PASSWORD);
    strncpy(settings.mqtt_topic_prefix, custom_mqtt_topic_prefix->getValue(), MAX_LENGTH_MQTT_TOPIC_PREFIX);
    strncpy(settings.syslog_host, custom_syslog_host->getValue(), MAX_LENGTH_SYSLOG_HOST);
    strncpy(settings.syslog_port, custom_syslog_port->getValue(), MAX_LENGTH_SYSLOG_PORT);
    strncpy(settings.syslog_device_hostname, custom_syslog_device_hostname->getValue(), MAX_LENGTH_SYSLOG_DEVICE_HOSTNAME);
    strncpy(settings.syslog_app_name, custom_syslog_app_name->getValue(), MAX_LENGTH_SYSLOG_APP_NAME);
    strncpy(settings.syslog_log_level, custom_syslog_log_level->getValue(), MAX_LENGTH_SYSLOG_LOG_LEVEL);

    // Fire up syslog if configured
    if (strncmp(settings.syslog_host, "", MAX_LENGTH_SYSLOG_HOST) != 0) {
        syslogEnabled = true;
        syslog.server(settings.syslog_host, atoi(settings.syslog_port));
        syslog.deviceHostname(settings.syslog_device_hostname);
        syslog.appName(settings.syslog_app_name);
        syslog.defaultPriority(LOG_LOCAL0);
        if (strncmp(settings.syslog_log_level, "EMERG", MAX_LENGTH_SYSLOG_LOG_LEVEL) != 0) {
            syslog.logMask(LOG_UPTO(LOG_EMERG));
        } else if (strncmp(settings.syslog_log_level, "ALERT", MAX_LENGTH_SYSLOG_LOG_LEVEL) != 0) {
            syslog.logMask(LOG_UPTO(LOG_ALERT));
        } else if (strncmp(settings.syslog_log_level, "CRIT", MAX_LENGTH_SYSLOG_LOG_LEVEL) != 0) {
            syslog.logMask(LOG_UPTO(LOG_CRIT));
        } else if (strncmp(settings.syslog_log_level, "ERR", MAX_LENGTH_SYSLOG_LOG_LEVEL) != 0) {
            syslog.logMask(LOG_UPTO(LOG_ERR));
        } else if (strncmp(settings.syslog_log_level, "ERR", MAX_LENGTH_SYSLOG_LOG_LEVEL) != 0) {
            syslog.logMask(LOG_UPTO(LOG_ERR));
        } else if (strncmp(settings.syslog_log_level, "WARNING", MAX_LENGTH_SYSLOG_LOG_LEVEL) != 0) {
            syslog.logMask(LOG_UPTO(LOG_WARNING));
        } else if (strncmp(settings.syslog_log_level, "NOTICE", MAX_LENGTH_SYSLOG_LOG_LEVEL) != 0) {
            syslog.logMask(LOG_UPTO(LOG_NOTICE));
        } else if (strncmp(settings.syslog_log_level, "INFO", MAX_LENGTH_SYSLOG_LOG_LEVEL) != 0) {
            syslog.logMask(LOG_UPTO(LOG_INFO));
        } else if (strncmp(settings.syslog_log_level, "DEBUG", MAX_LENGTH_SYSLOG_LOG_LEVEL) != 0) {
            syslog.logMask(LOG_UPTO(LOG_DEBUG));
        }
    }

    if (shouldSaveConfig) {
        saveConfig();
    }

    snprintf(mqtt_topic_info, 128, "%s/info", settings.mqtt_topic_prefix);
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

    if (syslogEnabled) syslog.logf(LOG_INFO, "mqttClient.setServer: %s %d", settings.mqtt_host, atoi(settings.mqtt_port));
    mqttClient.setServer(settings.mqtt_host, atoi(settings.mqtt_port));
    if (strlen(settings.mqtt_username) > 0) {
        if (syslogEnabled) syslog.log(LOG_INFO, "mqttClient.setCredentials: xxx xxx");
        mqttClient.setCredentials(settings.mqtt_username, settings.mqtt_password);
    }
    mqttClient.onMessage(mqttMessage);
    mqttClient.onConnect(mqttConnect);
    mqttClient.setWill(mqtt_topic_availability, 2, true, "offline");
    if (syslogEnabled) syslog.log(LOG_INFO, "mqttClient.connect()");
    mqttClient.connect();

    #ifdef PACKET_DEBUG
    heatpump.setPacketCallback(heatpumpPacketCallback);
    #endif
    heatpump.setSettingsChangedCallback(heatpumpSettingsChanged);
    heatpump.setStatusChangedCallback(heatpumpStatusChanged);

    if (heatPumpDetected) {
        if (syslogEnabled) syslog.log(LOG_INFO, "Heatpump: DETECTED. Connecting!");
        heatpump.connect(&Serial, SWAP_PINS);
    }
}

unsigned long lastSystemStatusTime = 0;

void loop() {
    if (syslogEnabled) syslog.log(LOG_DEBUG, "ArduinoOTA.handle()");
    ArduinoOTA.handle();

    if (heatPumpDetected) {
        if (updateHeatpump && millis() - lastHeatpumpSettingsChange > 500) {
            updateHeatpump = false;
            if (syslogEnabled) syslog.log(LOG_DEBUG, "heatpump.update()");
            heatpump.update();
            delay(100);
        }
        if (syslogEnabled) syslog.log(LOG_DEBUG, "heatpump.sync()");
        heatpump.sync();
    }

    if (millis() - lastSystemStatusTime > 60000) {
        lastSystemStatusTime = millis();
        if (syslogEnabled) syslog.log(LOG_DEBUG, "publishSystemStatus()");
        publishSystemStatus();
    }

    #ifdef CLEAR_SETTINGS_PIN
    handleClearSettingsButton();
    #endif
}
