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

#define HA_HOST "homeassistant.local"
#define HA_PORT 8123
// Long-lived access token from Home Assistant profile settings.
#define HA_TOKEN ""

#define HA_ENTITY_TEMPERATURE "sensor.bedroom_temperature"
#define HA_ENTITY_HUMIDITY "sensor.bedroom_humidity"
#define HA_ENTITY_CO2 "sensor.co2"
#define HA_ENTITY_WEATHER "weather.home"

#define TFT_HOR_RES 1246
#define TFT_VER_RES 1648

#define FRAME_JPG_PATH "/splash.jpg"
#define USB_MOUNT_PATH "/usb"
#define USB_EXPORT_INTERVAL_MS (3UL * 60UL * 1000UL)
#define USB_MOUNT_TIMEOUT_MS 12000
