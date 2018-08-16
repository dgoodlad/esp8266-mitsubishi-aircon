#include "CaptivePortal.h"
#include <DNSServer.h>

const byte DNS_PORT = 53;
IPAddress IP(192, 168, 1, 1);

CaptivePortal::CaptivePortal() {
    udp = new AsyncUDP;
};

CaptivePortal::~CaptivePortal() {
    delete udp;
};

captive_portal_result CaptivePortal::start(char *name, AsyncWebServer *server) {
    WiFi.disconnect();
    WiFi.mode(WIFI_AP_STA);
    boolean result = WiFi.softAP(name);
    delay(500);

    if(!result) { return E_CAPTIVE_PORTAL_FAIL; }

    setupListener();

    return E_CAPTIVE_PORTAL_OK;
}

void CaptivePortal::stop(AsyncWebServer *server) {
}

void CaptivePortal::setupListener() {
    udp->listen(53);
    udp->onPacket([](AsyncUDPPacket packet) {
        if(packet.length() > 0) {
            int len = packet.length();
            uint8_t *buffer = (uint8_t *)malloc(sizeof(uint8_t) * len);
            memcpy(buffer, packet.data(), sizeof(uint8_t) * len);

            DNSHeader* dnsHeader = (DNSHeader *)buffer;

            bool onlyOneQuestion = ntohs(dnsHeader->QDCount) == 1 && dnsHeader->ANCount == 0 && dnsHeader->NSCount == 0 && dnsHeader->ARCount == 0;

            if (dnsHeader->QR == DNS_QR_QUERY && dnsHeader->OPCode == DNS_OPCODE_QUERY && onlyOneQuestion) {
                int responseLen = len + 16;

                AsyncUDPMessage *message = new AsyncUDPMessage(responseLen);
                dnsHeader->QR = DNS_QR_RESPONSE;
                dnsHeader->ANCount = dnsHeader->QDCount;
                dnsHeader->QDCount = dnsHeader->QDCount;

                message->write(buffer, len);
                message->write((uint8_t)192);
                message->write((uint8_t)0x0c);

                message->write((uint8_t)0); // 0x0001  answer is type A query (host address)
                message->write((uint8_t)1);

                message->write((uint8_t)0); //0x0001 answer is class IN (internet address)
                message->write((uint8_t)1);

                message->write((uint8_t)0); // Set TTL to 0x00000001
                message->write((uint8_t)0);
                message->write((uint8_t)0);
                message->write((uint8_t)1);

                // Length of RData is 4 bytes (because, in this case, RData is IPv4)
                message->write((uint8_t)0);
                message->write((uint8_t)4);

                message->write((uint8_t)packet.localIP()[0]);
                message->write((uint8_t)packet.localIP()[1]);
                message->write((uint8_t)packet.localIP()[2]);
                message->write((uint8_t)packet.localIP()[3]);

                size_t rl = packet.send(*message);

                delete message;
            }

            free(buffer);
        }
    });
}