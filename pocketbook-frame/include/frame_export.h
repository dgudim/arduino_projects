#pragma once

#include <Arduino.h>
#include <EspUsbHost.h>
#include <lvgl.h>

struct FrameExportResult {
    bool ok = false;
    uint32_t bytes = 0;
    String message;
};

void frame_export_on_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
FrameExportResult export_lvgl_frame_to_usb(EspUsbHost &usb, EspUsbHostMscFS &msc, lv_display_t *disp);
