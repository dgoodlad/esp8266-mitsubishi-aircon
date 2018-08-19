#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>

#include "CaptivePortal.h"
#include "ConfigServer.h"

bool configMode = false;
AsyncWebServer webServer(80);

Config *config;

CaptivePortal *portal;
void captivePortalSetup() {
  portal = new CaptivePortal();
  char* ssid = "espap_xxxxxx";
  sprintf(ssid, "espac_%06X", ESP.getChipId());

  if(portal->start(ssid, &webServer) == E_CAPTIVE_PORTAL_OK) {
    Serial.printf("Captive Portal enabled on SSID %s\n", ssid);
  } else {
    Serial.printf("Failed to enable captive portal");
  }
}

ConfigServer *configServer;
void configServerSetup() {
  configServer = new ConfigServer();
  configServer->setup(&webServer, config);
}

void setup() {
  Serial.begin(9600, SERIAL_8N1); // Enable serial UART1
  Serial.setDebugOutput(true);    // Send wifi debug to UART1

  config = new Config();

  configMode = true;
  captivePortalSetup();
  configServerSetup();
  webServer.begin();
}

void loop() {
  if(configMode) {
    return;
  }
}
