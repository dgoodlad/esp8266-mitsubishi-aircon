#ifndef CONFIG_SERVER_h
#define CONFIG_SERVER_h

#include <ESPAsyncWebServer.h>
#include "Config.h"

class ConfigServer {
    public:
    static void setup(AsyncWebServer *server, Config *config);
};

#endif