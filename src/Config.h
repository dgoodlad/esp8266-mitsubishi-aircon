#ifndef CONFIG_h
#define CONFIG_h

#include <ArduinoJson.hpp>

#define CONFIG_JSON_BUFFER_SIZE 200

class Config {
    public:
      Config();
      Config(ArduinoJson::JsonObject& json);
      ~Config();

      ArduinoJson::JsonObject& asJson();
      void populateJson(ArduinoJson::JsonObject& root);
      void updateFromJson(ArduinoJson::JsonObject& root);

      // Wifi Config
      const char* ssid;
      const char* psk;

      // MQTT Config
      const char* mqtt_host;
      int mqtt_port;
      const char* mqtt_username;
      const char* mqtt_password;
      bool mqtt_use_ssl;
      const char* mqtt_topic;
};

#endif