#include "frame_export.h"
#include "config.h"
#include "web_log.h"

#include <FS.h>
#include <JPEGENC.h>
#include <esp_heap_caps.h>
#include <soc/usb_wrap_struct.h>
#include <string.h>

void service_background();

namespace {

constexpr int32_t kMcu = 16;                                             // JPEG 4:2:0 MCU is 16x16; also matches the LVGL tile height
constexpr int32_t kPixelBytes = LV_COLOR_DEPTH / 8;                      // RGB565 = 2 bytes
constexpr int32_t kJpegWidth = ((TFT_HOR_RES + kMcu - 1) / kMcu) * kMcu; // pad width to a whole MCU
constexpr int32_t kStripBytes = kJpegWidth * kMcu * kPixelBytes;         // one MCU-tall RGB565 row

JPEGENC jpg;
JPEGENCODE jpe;
fs::FS *jpeg_fs = nullptr;
File jpeg_file;
uint16_t *strip = nullptr;
bool capturing = false;
int32_t next_y = 0;
int32_t last_error = JPEGE_SUCCESS;

void *jpeg_open(const char *filename) {
    if (jpeg_fs == nullptr) {
        return nullptr;
    }
    jpeg_fs->remove(filename);
    jpeg_file = jpeg_fs->open(filename, FILE_WRITE);
    if (!jpeg_file) {
        return nullptr;
    }
    return &jpeg_file;
}

void jpeg_close(JPEGE_FILE *p) {
    File *f = static_cast<File *>(p->fHandle);
    if (f) {
        f->flush();
        f->close();
    }
}

int32_t jpeg_write(JPEGE_FILE *p, uint8_t *buffer, int32_t length) {
    File *f = static_cast<File *>(p->fHandle);
    if (!f) {
        return 0;
    }
    return f->write(buffer, length);
}

bool encode_mcu_row(uint16_t *row_pixels) {
    const int32_t pitch = kJpegWidth * kPixelBytes;
    for (int32_t x = 0; x < kJpegWidth; x += kMcu) {
        const int32_t rc = jpg.addMCU(&jpe, reinterpret_cast<uint8_t *>(&row_pixels[x]), pitch);
        if (rc != JPEGE_SUCCESS) {
            last_error = rc;
            Logger.printf("[jpeg] addMCU failed at x=%d y=%d rc=%d\n", x, next_y, rc);
            return false;
        }
    }
    service_background();
    return true;
}

void delay_serviced(uint32_t ms) {
    const uint32_t start = millis();
    while (millis() - start < ms) {
        service_background();
        delay(20);
    }
}

// Isolate D+/D- by driving SE0 (both data lines are low), and disabling the analog pads
void usb_dpdm_set_connected(bool connected) {
    if (!connected) {
        Logger.println("[usb] bus detached");

        usb_wrap_test_conf_reg_t test_conf;
        test_conf.val = USB_WRAP.test_conf.val;
        test_conf.test_enable = 1;
        test_conf.test_usb_wrap_oe = 0; // active-low OE: drive SE0 onto the wire
        test_conf.test_tx_dp = 0;
        test_conf.test_tx_dm = 0;
        USB_WRAP.test_conf.val = test_conf.val;
        delay(20);

        USB_WRAP.otg_conf.dp_pullup = 0;
        USB_WRAP.otg_conf.dm_pullup = 0;
        USB_WRAP.otg_conf.dp_pulldown = 0;
        USB_WRAP.otg_conf.dm_pulldown = 0;
        USB_WRAP.otg_conf.pad_pull_override = 1;
        USB_WRAP.otg_conf.pad_enable = 0;

    } else {
        Logger.println("[usb] bus attached");

        USB_WRAP.test_conf.test_enable = 0;
        USB_WRAP.otg_conf.pad_enable = 1;
        USB_WRAP.otg_conf.dp_pullup = 0;
        USB_WRAP.otg_conf.dm_pullup = 0;
        USB_WRAP.otg_conf.dp_pulldown = 1;
        USB_WRAP.otg_conf.dm_pulldown = 1;
        USB_WRAP.otg_conf.pad_pull_override = 1;
    }
    delay_serviced(USB_HOST_SETTLE_MS);
}

bool usb_safe_connect_start(EspUsbHost &usb) {
    if (usb.ready()) {
        usb_dpdm_set_connected(true);
        return true;
    }
    if (!usb.begin()) {
        Logger.printf("[usb] error starting USB host: %s\n", usb.lastErrorName());
        return false;
    }
    Logger.println("[usb] started USB host");
    return true;
}

void usb_safe_eject_unmount(EspUsbHost &usb, EspUsbHostMscFS &msc) {
    if (msc.mounted()) {
        msc.end();
        Logger.println("[usb] unmounted");
    }
    usb_dpdm_set_connected(false);
}

bool usb_safe_mount(EspUsbHost &usb, EspUsbHostMscFS &msc) {
    if (msc.mounted()) {
        return true;
    }

    const uint32_t mount_start = millis();
    while (millis() - mount_start < USB_MOUNT_TIMEOUT_MS) {
        service_background();
        if (usb.mscReady() &&
            msc.begin(usb, USB_MOUNT_PATH, ESP_USB_HOST_ANY_ADDRESS, 0, 4, 1000, true)) {
            Logger.println("[usb] mounted");
            return true;
        }
        Logger.printf("[usb] waiting for MSC (%u devices, last=%s)\n", usb.deviceCount(), usb.lastErrorName());
        delay(200);
    }
    Logger.printf("[usb] error mounting, timeout waiting for MSC device: %s\n", usb.lastErrorName());
    return false;
}

} // namespace

void frame_export_on_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    // Always-installed LVGL flush callback. Ignore redraws that are not the JPEG capture pass.
    if (capturing && strip != nullptr && last_error == JPEGE_SUCCESS) {
        const int32_t w = lv_area_get_width(area);
        const int32_t h = lv_area_get_height(area);
        const uint32_t src_stride = lv_draw_buf_width_to_stride(w, lv_display_get_color_format(disp));

        // LVGL flushes 16-line tiles. Copy the rows that overlap the current 16-high MCU strip.
        for (int32_t row = 0; row < h; row++) {
            const int32_t dst_row = area->y1 + row - next_y;
            if (dst_row < 0 || dst_row >= kMcu) {
                continue;
            }
            // Encoder width is padded to a multiple of 16; do not copy past the real display edge.
            const int32_t copy_px = ((area->x1 + w) > TFT_HOR_RES) ? (TFT_HOR_RES - area->x1) : w;
            if (area->x1 < 0 || copy_px <= 0) {
                continue;
            }
            memcpy(strip + dst_row * kJpegWidth + area->x1, px_map + row * src_stride,
                   copy_px * kPixelBytes);
        }

        // 4:2:0 MCU is 16x16. Encode when this flush completed the full-width strip at next_y.
        if (area->x1 == 0 && w == TFT_HOR_RES && area->y1 <= next_y && area->y2 >= next_y + kMcu - 1) {
            encode_mcu_row(strip);
            memset(strip, 0xFF, kStripBytes);
            next_y += kMcu;
        }
    }

    lv_display_flush_ready(disp);
}

FrameExportResult export_lvgl_frame_to_usb(EspUsbHost &usb, EspUsbHostMscFS &msc, lv_display_t *disp) {
    FrameExportResult result;

    if (!usb_safe_connect_start(usb)) {
        result.message = "starting USB failed: ";
        result.message += usb.lastErrorName();
        return result;
    }

    if (!usb_safe_mount(usb, msc)) {
        result.message = "mount failed: ";
        result.message += usb.lastErrorName();
        usb_safe_eject_unmount(usb, msc);
        return result;
    }
    Logger.printf("[usb] Writing %s\n", FRAME_JPG_PATH);

    strip = static_cast<uint16_t *>(heap_caps_malloc(kStripBytes, MALLOC_CAP_8BIT));
    if (strip == nullptr) {
        result.message = "no memory for JPEG strip";
        Logger.println("[usb] " + result.message);
        usb_safe_eject_unmount(usb, msc);
        return result;
    }
    memset(strip, 0xFF, kStripBytes);

    jpeg_fs = &msc;
    last_error = jpg.open(FRAME_JPG_PATH, jpeg_open, jpeg_close, nullptr, jpeg_write, nullptr);
    if (last_error != JPEGE_SUCCESS || !jpeg_file) {
        result.message = "JPEG open failed";
        Logger.printf("[usb] %s rc=%d\n", result.message.c_str(), last_error);
        heap_caps_free(strip);
        strip = nullptr;
        jpeg_fs = nullptr;
        usb_safe_eject_unmount(usb, msc);
        return result;
    }

    last_error = jpg.encodeBegin(&jpe, TFT_HOR_RES, TFT_VER_RES, JPEGE_PIXEL_RGB565, JPEGE_SUBSAMPLE_420,
                                 JPEGE_Q_HIGH);
    if (last_error != JPEGE_SUCCESS) {
        result.message = "JPEG encodeBegin failed";
        Logger.printf("[usb] %s rc=%d\n", result.message.c_str(), last_error);
        jpg.close();
        heap_caps_free(strip);
        strip = nullptr;
        jpeg_fs = nullptr;
        usb_safe_eject_unmount(usb, msc);
        return result;
    }

    next_y = 0;
    capturing = true;
    lv_obj_invalidate(lv_display_get_screen_active(disp));
    lv_refr_now(disp);
    capturing = false;

    while (last_error == JPEGE_SUCCESS && next_y < TFT_VER_RES) {
        encode_mcu_row(strip);
        memset(strip, 0xFF, kStripBytes);
        next_y += kMcu;
    }

    const int32_t data_size = jpg.close();
    heap_caps_free(strip);
    strip = nullptr;
    jpeg_fs = nullptr;

    if (last_error != JPEGE_SUCCESS || data_size <= 0) {
        result.message = "JPEG encode failed";
        Logger.printf("[usb] %s rc=%d size=%d\n", result.message.c_str(), last_error, data_size);
        usb_safe_eject_unmount(usb, msc);
        return result;
    }

    result.ok = true;
    result.bytes = data_size;
    result.message = "wrote ";
    result.message += FRAME_JPG_PATH;
    Logger.printf("[usb] wrote %s (%d bytes)\n", FRAME_JPG_PATH, data_size);
    usb_safe_eject_unmount(usb, msc);
    return result;
}
