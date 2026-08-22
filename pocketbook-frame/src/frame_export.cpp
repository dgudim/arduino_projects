#include "frame_export.h"
#include "config.h"
#include "web_log.h"

#include <FS.h>
#include <esp_heap_caps.h>
#include <esp_jpeg_enc.h>
#include <soc/usb_wrap_struct.h>
#include <string.h>

void service_background();

namespace {

static_assert(TFT_VER_RES % 8 == 0, "esp_new_jpeg grayscale block mode needs height multiple of 8");

constexpr int32_t kBlockRows = 8;  // grayscale MCU/block height; matches the LVGL tile height
constexpr uint32_t kJpegOutCapBytes = 256 * 1024;  // max compressed JPEG size; encoder writes the whole file here
constexpr uint8_t kJpegQuality = 90;

struct JpegCapture {
    jpeg_enc_handle_t jpeg_enc = nullptr;
    uint8_t *strip = nullptr;
    uint8_t *jpeg_out = nullptr;
    int jpeg_out_len = 0;
    int jpeg_block_size = 0;
    int32_t strip_stride = 0;
    int32_t next_y = 0;
    bool encode_failed = false;
};

void jpeg_release(JpegCapture &capture) {
    if (capture.jpeg_enc != nullptr) {
        jpeg_enc_close(capture.jpeg_enc);
        capture.jpeg_enc = nullptr;
    }
    if (capture.strip != nullptr) {
        jpeg_free_align(capture.strip);
        capture.strip = nullptr;
    }
    if (capture.jpeg_out != nullptr) {
        heap_caps_free(capture.jpeg_out);
        capture.jpeg_out = nullptr;
    }
}

bool encode_block(JpegCapture &capture) {
    const jpeg_error_t encode_status = jpeg_enc_process_with_block(
        capture.jpeg_enc, capture.strip, capture.jpeg_block_size, capture.jpeg_out, kJpegOutCapBytes,
        &capture.jpeg_out_len);
    if (encode_status < JPEG_ERR_OK) {
        capture.encode_failed = true;
        Logger.printf("[jpeg] encode block failed at y=%d status=%d\n", capture.next_y, encode_status);
        return false;
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
        delay(200);
    }
    Logger.printf("[usb] error mounting, timeout waiting for MSC device: %s\n", usb.lastErrorName());
    return false;
}

} // namespace

void frame_export_on_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    // Always-installed LVGL flush callback. Capture state is only set for the export refresh.
    JpegCapture *capture = static_cast<JpegCapture *>(lv_display_get_user_data(disp));
    if (capture != nullptr && capture->strip != nullptr && !capture->encode_failed) {
        const int32_t area_width = lv_area_get_width(area);
        const int32_t area_height = lv_area_get_height(area);
        const uint32_t src_stride = lv_draw_buf_width_to_stride(area_width, lv_display_get_color_format(disp));

        for (int32_t row = 0; row < area_height; row++) {
            const int32_t dst_row = area->y1 + row - capture->next_y;
            if (dst_row < 0 || dst_row >= kBlockRows) {
                continue;
            }
            const int32_t copy_px =
                ((area->x1 + area_width) > TFT_HOR_RES) ? (TFT_HOR_RES - area->x1) : area_width;
            if (area->x1 < 0 || copy_px <= 0) {
                continue;
            }
            memcpy(capture->strip + dst_row * capture->strip_stride + area->x1, px_map + row * src_stride,
                   copy_px);
        }
        if (area->x1 == 0 && area_width == TFT_HOR_RES && area->y1 <= capture->next_y &&
            area->y2 >= capture->next_y + kBlockRows - 1) {
            encode_block(*capture);
            memset(capture->strip, 0xFF, capture->jpeg_block_size);
            capture->next_y += kBlockRows;
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

    JpegCapture capture;
    jpeg_enc_config_t enc_cfg = DEFAULT_JPEG_ENC_CONFIG();
    enc_cfg.width = TFT_HOR_RES;
    enc_cfg.height = TFT_VER_RES;
    enc_cfg.src_type = JPEG_PIXEL_FORMAT_GRAY;
    enc_cfg.subsampling = JPEG_SUBSAMPLE_GRAY;
    enc_cfg.quality = kJpegQuality;
    enc_cfg.rotate = JPEG_ROTATE_0D;
    enc_cfg.task_enable = false;

    if (jpeg_enc_open(&enc_cfg, &capture.jpeg_enc) != JPEG_ERR_OK || capture.jpeg_enc == nullptr) {
        result.message = "JPEG encoder open failed";
        Logger.println("[usb] " + result.message);
        usb_safe_eject_unmount(usb, msc);
        return result;
    }

    capture.jpeg_block_size = jpeg_enc_get_block_size(capture.jpeg_enc);
    if (capture.jpeg_block_size <= 0 || capture.jpeg_block_size % kBlockRows != 0) {
        result.message = "unexpected JPEG block size";
        Logger.printf("[usb] %s got=%d\n", result.message.c_str(), capture.jpeg_block_size);
        jpeg_release(capture);
        usb_safe_eject_unmount(usb, msc);
        return result;
    }
    capture.strip_stride = capture.jpeg_block_size / kBlockRows;

    capture.strip = static_cast<uint8_t *>(jpeg_calloc_align(capture.jpeg_block_size, 16));
    capture.jpeg_out = static_cast<uint8_t *>(heap_caps_malloc(kJpegOutCapBytes, MALLOC_CAP_8BIT));
    if (capture.strip == nullptr || capture.jpeg_out == nullptr) {
        result.message = "no memory for JPEG buffers";
        Logger.println("[usb] " + result.message);
        jpeg_release(capture);
        usb_safe_eject_unmount(usb, msc);
        return result;
    }
    memset(capture.strip, 0xFF, capture.jpeg_block_size);

    lv_display_set_user_data(disp, &capture);
    lv_obj_invalidate(lv_display_get_screen_active(disp));
    lv_refr_now(disp);
    lv_display_set_user_data(disp, nullptr);

    while (!capture.encode_failed && capture.next_y < TFT_VER_RES) {
        encode_block(capture);
        memset(capture.strip, 0xFF, capture.jpeg_block_size);
        capture.next_y += kBlockRows;
    }

    const int32_t data_size = capture.jpeg_out_len;
    if (capture.encode_failed || data_size <= 0) {
        result.message = "JPEG encode failed";
        Logger.printf("[usb] %s size=%d\n", result.message.c_str(), data_size);
        jpeg_release(capture);
        usb_safe_eject_unmount(usb, msc);
        return result;
    }

    msc.remove(FRAME_JPG_PATH);
    File out = msc.open(FRAME_JPG_PATH, FILE_WRITE);
    if (!out) {
        result.message = "JPEG file open failed";
        Logger.println("[usb] " + result.message);
        jpeg_release(capture);
        usb_safe_eject_unmount(usb, msc);
        return result;
    }
    const size_t written = out.write(capture.jpeg_out, data_size);
    out.flush();
    out.close();
    jpeg_release(capture);

    if (written != static_cast<size_t>(data_size)) {
        result.message = "JPEG file write failed";
        Logger.printf("[usb] %s wrote=%u expected=%d\n", result.message.c_str(),
                      static_cast<unsigned>(written), data_size);
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
