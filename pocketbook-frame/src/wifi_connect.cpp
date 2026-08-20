#include "wifi_connect.h"
#include "config.h"
#include "web_log.h"

#include <WiFi.h>

void wifi_connect(uint32_t wait_ms) {
    static bool configured = false;
    static uint32_t last_try_ms = 0;

    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    const uint32_t now = millis();
    if (configured && now - last_try_ms < 10000) {
        return;
    }

    if (!configured) {
        WiFi.mode(WIFI_STA);
        WiFi.setHostname(OTA_HOSTNAME);
        WiFi.setSleep(false);
        configured = true;
        Logger.printf("[wifi] connecting to %s", WIFI_SSID);
    } else {
        Logger.println("[wifi] reconnecting");
        WiFi.disconnect();
    }

    last_try_ms = now;
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    if (wait_ms == 0) {
        return;
    }

    const uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < wait_ms) {
        delay(500);
        Logger.print(".");
        yield();
    }
    Logger.println();
    if (WiFi.status() == WL_CONNECTED) {
        Logger.printf("[wifi] %s\n", WiFi.localIP().toString().c_str());
    } else {
        Logger.println("[wifi] connect timed out, will keep retrying");
    }
}
