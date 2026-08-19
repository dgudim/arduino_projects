#include <EspUsbHost.h>
#include <WebServer.h>
#include <WiFi.h>
#include <lvgl.h>

#define TFT_HOR_RES 1246
#define TFT_VER_RES 1648
#define TFT_ROTATION LV_DISPLAY_ROTATION_90

/*LVGL draws into this buffer */
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES)
uint16_t draw_buf[DRAW_BUF_SIZE];

/* LVGL calls it when a rendered image needs to copied to the display*/
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    /*Copy `px map` to the `area`*/

    /*For example ("my_..." functions needs to be implemented by you)
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);

    my_set_window(area->x1, area->y1, w, h);
    my_draw_bitmaps(px_map, w * h);
     */

    /*Call it to tell LVGL you are ready*/
    lv_display_flush_ready(disp);
}

/*Read the touchpad*/
// Unused
void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
}

static uint32_t my_tick(void) {
    return millis();
}

EspUsbHost usb;
EspUsbHostMscFS usbMassStorage;

String html_message = "";

static void printRootEntries(fs::FS &fs) {
    File root = fs.open("/");
    if (!root || !root.isDirectory()) {
        return;
    }

    Serial.println("Root entries:");
    while (true) {
        File entry = root.openNextFile();
        if (!entry) {
            break;
        }

        html_message += entry.isDirectory() ? "DIR " : "FILE ";
        html_message += entry.name();

        entry.close();
    }
    root.close();
}

static void writeReadDeleteProbe(fs::FS &fs) {
    const char *filePath = "/splash.bmp";

    const char *message = "EspUsbHost MSC FAT write probe\n";
    File file = fs.open(filePath, FILE_WRITE);
    if (!file) {
        return;
    }
    const size_t written = file.print(message);
    file.close();

    file = fs.open(filePath, FILE_READ);
    if (!file) {
        return;
    }
    char buffer[64] = {};
    const size_t readBytes = file.readBytes(buffer, sizeof(buffer) - 1);
    file.close();

    if (fs.remove(filePath)) {

    } else {
        html_message += "\nfs remove failed";
    }
}

const char *ssid = "IDDQD";
const char *password = "3Doodler its supper 3d";
WebServer server(80);

void handleRoot() {
    server.send(200, "text/html", html_message);
}

void setup() {

    lv_init();
    lv_tick_set_cb(my_tick);

    lv_display_t * disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_DIRECT);

    /*Initialize the (dummy) input device driver*/
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
    lv_indev_set_read_cb(indev, my_touchpad_read);

    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text( label, "Hello Arduino, I'm LVGL!" );
    lv_obj_align( label, LV_ALIGN_CENTER, 0, 0 );

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    server.on("/", handleRoot);
    server.begin();

    usbMassStorage.setSkipSyncCache(true);
}

static uint32_t lastMountAttemptMs = 0;

void loop() {
    delay(10000);
    server.handleClient();

    lv_timer_handler();

    if (!usb.begin()) {
        html_message += usb.lastErrorName();
    }

    if (!usbMassStorage.mounted()) {
        const uint32_t now = millis();

        // en: Retry mounting at a low rate so other loop work can continue.
        if (now - lastMountAttemptMs >= 1000) {
            lastMountAttemptMs = now;

            if (usbMassStorage.begin(usb, "/usb")) {
                printRootEntries(usbMassStorage);
                writeReadDeleteProbe(usbMassStorage);
            } else {
                html_message += "\nmounting failed";
            }
            usbMassStorage.end();
        }
    }

    usb.end();

    delay(10000);
}