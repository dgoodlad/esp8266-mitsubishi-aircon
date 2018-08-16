#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>

#include "CaptivePortal.h"

bool configMode = false;
AsyncWebServer webServer(80);

CaptivePortal *portal;
void captivePortalSetup() {
  portal = new CaptivePortal();
  char* ssid = "espap_xxxxxx";
  sprintf(ssid, "espac_%06X", ESP.getChipId());

  if(portal->start(ssid, &webServer) == E_CAPTIVE_PORTAL_OK) {
    Serial1.printf("Captive Portal enabled on SSID %s\n", ssid);
  } else {
    Serial1.printf("Failed to enable captive portal");
  }
}

void setup() {
  Serial.swap(); // Point Serial UART0 TX/RX to GPIO15/GPIO13
  Serial1.begin(9600, SERIAL_8N1); // Enable serial UART1
  Serial1.setDebugOutput(true);    // Send wifi debug to UART1

  configMode = true;
  captivePortalSetup();
}

void loop() {
  if(configMode) {
    return;
  }
}
