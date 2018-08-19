#include "Config.h"
#include <ArduinoJson.hpp>

Config::Config() {
    ssid = "";
    psk = "";

    mqtt_host = "";
    mqtt_port = 8883;
    mqtt_username = "";
    mqtt_password = "";
    mqtt_use_ssl = true;
    mqtt_topic = "esp-ac";
};

Config::Config(ArduinoJson::JsonObject& json) {
    ssid = json["ssid"];
    psk = json["psk"];

    mqtt_host = json["mqttHost"];
    mqtt_port = json["mqttPort"] | 8883;
    mqtt_username = json["mqttUsername"];
    mqtt_password = json["mqttPassword"];
    mqtt_use_ssl = json["mqttUseSSL"] | true;
    mqtt_topic = json["mqttTopic"] | "esp-ac";
}

Config::~Config() {};

ArduinoJson::JsonObject& Config::asJson() {
    ArduinoJson::StaticJsonBuffer<CONFIG_JSON_BUFFER_SIZE> buffer;
    ArduinoJson::JsonObject& json = buffer.createObject();

    populateJson(json);
    return json;
}

void Config::populateJson(ArduinoJson::JsonObject& json) {
    json["wifiSSID"] = ssid;
    json["wifiPSK"] = psk;
    json["mqttHost"] = mqtt_host;
    json["mqttPort"] = mqtt_port;
    json["mqttUsername"] = mqtt_username;
    json["mqttPassword"] = mqtt_password;
    json["mqttUseSSL"] = mqtt_use_ssl;
    json["mqttTopic"] = mqtt_topic;
}

void Config::updateFromJson(ArduinoJson::JsonObject& json) {
    ssid = json["wifiSSID"];
    psk = json["wifiPSK"];
    mqtt_host = json["mqttHost"];
    mqtt_port = json["mqttPort"];
    mqtt_username = json["mqttUsername"];
    mqtt_password = json["mqttPassword"];
    mqtt_use_ssl = json["mqttUseSSL"];
    mqtt_topic = json["mqttTopic"];
}