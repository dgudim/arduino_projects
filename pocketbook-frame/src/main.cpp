#include "config.h"
#include "dashboard.h"
#include "frame_export.h"
#include "ha_client.h"
#include "web_log.h"
#include "webserial_page.h"
#include "wifi_connect.h"

#include <ArduinoOTA.h>
#include <EspUsbHost.h>
#include <WebSerial.h>
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_task_wdt.h>
#include <lvgl.h>

// Partial LVGL framebuffer: a full 758x1024 L8 frame still needs a lot of RAM, so we flush 8-line tiles.
constexpr uint32_t kDrawBufLines = 8;                  // matches JPEG grayscale block height
constexpr uint32_t kPixelBytes = LV_COLOR_DEPTH / 8;   // L8 grayscale = 1 byte
constexpr uint32_t kDrawBufPixels = TFT_HOR_RES * kDrawBufLines;
constexpr uint32_t kDrawBufBytes = kDrawBufPixels * kPixelBytes;

EspUsbHost usb;
EspUsbHostMscFS usbMassStorage;
lv_display_t *display = nullptr;
uint8_t *draw_buf = nullptr;
HaSnapshot ha_data;
FrameExportResult last_export;
volatile bool export_requested = false;
uint32_t last_export_ms = 0;

void service_background() {
    ArduinoOTA.handle();
    WebSerial.loop();
    esp_task_wdt_reset();
    yield();
}

static void setup_ota() {
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    if (OTA_PASSWORD[0] != '\0') {
        ArduinoOTA.setPassword(OTA_PASSWORD);
    }

    ArduinoOTA.onStart([]() {
        Logger.println("[ota] start");
    });
    ArduinoOTA.onEnd([]() {
        Logger.println("[ota] end");
    });
    ArduinoOTA.onProgress([](uint32_t progress, uint32_t total) {
        static uint32_t last_pct = 0;
        const uint32_t pct = total ? (progress * 100) / total : 0;
        if (pct >= last_pct + 10 || pct == 100) {
            last_pct = pct;
            Logger.printf("[ota] %u%%\n", pct);
        }
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Logger.printf("[ota] error %d\n", error);
    });
    ArduinoOTA.begin();
    Logger.printf("[ota] ready as %s.local\n", OTA_HOSTNAME);
}

static String format_status_page() {
    String page;
    page += F("<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' "
              "content='width=device-width, initial-scale=1'><title>" OTA_HOSTNAME "</title>");
    page += F("<style>body{font-family:sans-serif;max-width:720px;margin:24px auto;padding:0 16px;}"
              "a,button{font-size:16px} pre{background:#111;color:#d6ffd6;padding:12px;overflow:auto;}"
              "form{display:inline}</style></head><body>");
    page += F("<h1>" OTA_HOSTNAME "</h1>");
    page += F("<p><a href='" WEB_SERIAL_PATH "'>Web serial</a></p>");
    page += "<p>IP: ";
    page += WiFi.localIP().toString();
    page += "<br>OTA: ";
    page += OTA_HOSTNAME;
    page += ".local";
    page += "<br>Wi-Fi: ";
    page += (WiFi.status() == WL_CONNECTED) ? "connected" : "down";
    page += "</p><h2>Home Assistant</h2><pre>";
    page += "temperature: ";
    page += String(ha_data.temperature);
    page += "\nhumidity: ";
    page += String(ha_data.humidity);
    page += "\nco2: ";
    page += String(ha_data.co2);
    page += "\nweather: ";
    page += ha_data.weather_condition;
    if (ha_data.error.length()) {
        page += "\nerror: ";
        page += ha_data.error;
    }
    page += "</pre><h2>Last USB export</h2><pre>";
    page += last_export.message;
    page += "\nbytes: ";
    page += String(last_export.bytes);
    page += "</pre>";
    page += F("<form method='POST' action='" WEB_EXPORT_PATH "'><button type='submit'>Export frame now</button></form>");
    page += F("<p>PlatformIO OTA: <code>pio run -e " PIO_OTA_ENV " -t upload</code></p>");
    page += F("</body></html>");
    return page;
}

static void setup_lvgl() {
    lv_init();
    lv_tick_set_cb([]() -> uint32_t { return millis(); });
    lv_log_register_print_cb([](lv_log_level_t, const char *buf) { Logger.print(buf); });

    draw_buf = static_cast<uint8_t *>(heap_caps_malloc(kDrawBufBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (draw_buf == nullptr) {
        Logger.println("[lvgl] internal draw buffer alloc failed, trying PSRAM");
        draw_buf = static_cast<uint8_t *>(heap_caps_malloc(kDrawBufBytes, MALLOC_CAP_8BIT));
    }
    if (draw_buf == nullptr) {
        Logger.println("[lvgl] draw buffer alloc failed");
        while (true) {
            service_background();
            delay(1000);
        }
    }

    // Virtual display
    display = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_L8);
    lv_display_set_flush_cb(display, frame_export_on_flush);
    lv_display_set_buffers(display, draw_buf, nullptr, kDrawBufBytes,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Dummy touch device
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, [](lv_indev_t *, lv_indev_data_t *) {});

    dashboard_create(display);
}

static void run_export() {
    ha_fetch(ha_data);
    dashboard_update(ha_data);
    lv_timer_handler();
    last_export = export_lvgl_frame_to_usb(usb, usbMassStorage, display);
    last_export_ms = millis();
}

static void setup_web_server() {
    WebSerial.begin(&server, "/webserial-stock");
    server.on(WEB_SERIAL_PATH, HTTP_GET, [](AsyncWebServerRequest *request) {
        serve_webserial_page(request);
    });
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", format_status_page());
    });
    server.on(WEB_EXPORT_PATH, HTTP_POST, [](AsyncWebServerRequest *request) {
        export_requested = true;
        request->redirect("/");
    });
    server.begin();
    Logger.printf("[http] http://%s/\n", WiFi.localIP().toString().c_str());
    Logger.printf("[http] serial at %s\n", WEB_SERIAL_PATH);
}

void setup() {
    esp_log_set_vprintf(web_log_vprintf);
    Logger.printf("[boot] %s\n", OTA_HOSTNAME);

    setup_lvgl();
    wifi_connect(WIFI_CONNECT_TIMEOUT_MS);
    configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    setup_ota();
    setup_web_server();
}

void loop() {
    service_background();
    wifi_connect();

    const uint32_t now = millis();
    if (export_requested || now - last_export_ms >= USB_EXPORT_INTERVAL_MS) {
        // Guard against double clicks or clicks right after an export
        if (last_export_ms != 0 && now - last_export_ms < USB_EXPORT_COOLDOWN_MS) {
            delay(15);
            return;
        }
        export_requested = false;
        run_export();
    }

    delay(15);
}
