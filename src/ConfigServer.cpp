#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include "ConfigServer.h"
#include "config_html_gz.h"

void ConfigServer::setup(AsyncWebServer *server, Config *config) {
    // GET /config.json
    server->on("/config.json", HTTP_GET, [config](AsyncWebServerRequest *request) {
        AsyncJsonResponse *response = new AsyncJsonResponse();
        response->addHeader("Server", "ESP AC");
        JsonObject& root = response->getRoot();
        config->populateJson(root);
        response->setLength();
        request->send(response);
    });

    // PUT /config.json
    AsyncCallbackJsonWebHandler* updateHandler = new AsyncCallbackJsonWebHandler("/config.json", [config](AsyncWebServerRequest *request, JsonVariant &json) {
        JsonObject& jsonObj = json.as<JsonObject>();
        config->updateFromJson(jsonObj);

        AsyncWebServerResponse *response = request->beginResponse(204, "text/plain", "No Content");
        response->addHeader("Content-Location", "/config.json");
        request->send(response);
    });
    updateHandler->setMethod(HTTP_PUT);
    server->addHandler(updateHandler);

    server->on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html; charset=utf-8", config_html_gz, config_html_gz_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
}