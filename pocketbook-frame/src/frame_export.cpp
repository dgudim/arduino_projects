#include "frame_export.h"
#include "config.h"
#include "web_log.h"

#include <FS.h>
#include <JPEGENC.h>
#include <esp_heap_caps.h>
#include <string.h>

void service_background();

namespace {

constexpr int32_t kMcu = 16;  // JPEG 4:2:0 MCU is 16x16; also matches the LVGL tile height
constexpr int32_t kPixelBytes = LV_COLOR_DEPTH / 8;  // RGB565 = 2 bytes
constexpr int32_t kJpegWidth = ((TFT_HOR_RES + kMcu - 1) / kMcu) * kMcu;  // pad width to a whole MCU
constexpr int32_t kStripBytes = kJpegWidth * kMcu * kPixelBytes;  // one MCU-tall RGB565 row

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

void usb_shutdown(EspUsbHost &usb, EspUsbHostMscFS &msc, bool mounted) {
    if (mounted) {
        msc.end();
        Logger.println("[usb] unmounted");
    }
    usb.end();
    Logger.println("[usb] disconnected");
}

}  // namespace

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

    Logger.println("[usb] starting host");
    if (!usb.begin()) {
        result.message = usb.lastErrorName();
        Logger.printf("[usb] begin failed: %s\n", result.message.c_str());
        return result;
    }

    const uint32_t mount_start = millis();
    bool mounted = false;
    while (millis() - mount_start < USB_MOUNT_TIMEOUT_MS) {
        service_background();
        if (msc.begin(usb, USB_MOUNT_PATH)) {
            mounted = true;
            break;
        }
        delay(250);
    }

    if (!mounted) {
        result.message = "mount failed: ";
        result.message += usb.lastErrorName();
        Logger.println("[usb] " + result.message);
        usb_shutdown(usb, msc, false);
        return result;
    }
    Logger.printf("[usb] mounted, writing %s\n", FRAME_JPG_PATH);

    strip = static_cast<uint16_t *>(heap_caps_malloc(kStripBytes, MALLOC_CAP_8BIT));
    if (strip == nullptr) {
        result.message = "no memory for JPEG strip";
        Logger.println("[usb] " + result.message);
        usb_shutdown(usb, msc, true);
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
        usb_shutdown(usb, msc, true);
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
        usb_shutdown(usb, msc, true);
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
        usb_shutdown(usb, msc, true);
        return result;
    }

    result.ok = true;
    result.bytes = data_size;
    result.message = "wrote ";
    result.message += FRAME_JPG_PATH;
    Logger.printf("[usb] wrote %s (%d bytes)\n", FRAME_JPG_PATH, data_size);
    usb_shutdown(usb, msc, true);
    return result;
}
