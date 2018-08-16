#ifndef CAPTIVE_PORTAL_h
#define CAPTIVE_PORTAL_h

#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncUDP.h>

#define captive_portal_result uint8_t
#define E_CAPTIVE_PORTAL_OK   0x00
#define E_CAPTIVE_PORTAL_FAIL 0x01

class CaptivePortal {
 public:
  CaptivePortal();
  ~CaptivePortal();
  captive_portal_result start(char *name, AsyncWebServer *server);
  void stop(AsyncWebServer *server);
 private:
  AsyncUDP *udp;
  void setupListener();
};

#endif
