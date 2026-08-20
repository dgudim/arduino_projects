#pragma once

#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define WIFI_CONNECT_TIMEOUT_MS 30000

#define OTA_HOSTNAME "pocketbook-frame"
#define OTA_PASSWORD ""

#define WEB_HTTP_PORT 80
#define WEB_SERIAL_PATH "/webserial"
#define WEB_EXPORT_PATH "/export"
#define PIO_OTA_ENV "lolin_s3_mini_ota"

#define HA_HOST ""
#define HA_PORT 8123
// Long-lived access token from Home Assistant profile settings.
#define HA_TOKEN ""

#define HA_ENTITY_TEMPERATURE "sensor.temperature"
#define HA_ENTITY_HUMIDITY "sensor.humidity"
#define HA_ENTITY_CO2 "sensor.co2"
#define HA_ENTITY_WEATHER "weather.local"

#define TFT_HOR_RES 758
#define TFT_VER_RES 1024

#define FRAME_JPG_PATH "/splash.jpg"
#define USB_MOUNT_PATH "/usb"
#define USB_EXPORT_INTERVAL_MS (3UL * 60UL * 1000UL)
#define USB_EXPORT_COOLDOWN_MS 20000
#define USB_MOUNT_TIMEOUT_MS 30000
#define USB_HOST_SETTLE_MS 1500
