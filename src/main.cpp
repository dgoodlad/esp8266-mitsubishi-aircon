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

char mqtt_host[255];
char mqtt_port[6] = "8080";
char mqtt_username[64];
char mqtt_password[64];

char config_ap_name[17];
bool shouldSaveConfig = false;

void saveConfigCallback () {
    shouldSaveConfig = true;
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

    WiFiManager wifiManager;

    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.addParameter(&custom_mqtt_host);
    wifiManager.addParameter(&custom_mqtt_port);
    wifiManager.addParameter(&custom_mqtt_username);
    wifiManager.addParameter(&custom_mqtt_password);

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

    if (shouldSaveConfig) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.createObject();
        json["mqtt_host"] = mqtt_host;
        json["mqtt_port"] = mqtt_port;
        json["mqtt_username"] = mqtt_username;
        json["mqtt_password"] = mqtt_password;

        File configFile = SPIFFS.open("/config.json", "w");
        if (!configFile) {
            // Failed to open config file
        }

        json.printTo(configFile);
        configFile.close();
    }
}

void loop() {

}
