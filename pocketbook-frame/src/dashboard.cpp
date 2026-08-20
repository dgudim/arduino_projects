#include "dashboard.h"
#include "config.h"

#include <math.h>
#include <stdio.h>
#include <time.h>

namespace {

lv_obj_t *clock_label = nullptr;
lv_obj_t *date_label = nullptr;
lv_obj_t *status_label = nullptr;
lv_obj_t *temp_value = nullptr;
lv_obj_t *hum_value = nullptr;
lv_obj_t *co2_value = nullptr;
lv_obj_t *weather_value = nullptr;
lv_obj_t *weather_detail = nullptr;

String format_number(float value, const char *suffix, int32_t decimals) {
    if (isnan(value)) {
        return "--";
    }
    char buf[32];
    const char *space = (suffix[0] == '%' || suffix[0] == '\0') ? "" : " ";
    snprintf(buf, sizeof(buf), "%.*f%s%s", decimals, value, space, suffix);
    return String(buf);
}

lv_obj_t *make_card(lv_obj_t *parent, const char *caption, lv_obj_t **value_out) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_style_bg_color(card, lv_color_white(), 0);
    lv_obj_set_style_border_color(card, lv_color_black(), 0);
    lv_obj_set_style_border_width(card, 6, 0);
    lv_obj_set_style_radius(card, 18, 0);
    lv_obj_set_style_pad_all(card, 28, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *value = lv_label_create(card);
    lv_label_set_text(value, "--");
    lv_obj_set_style_text_font(value, &lv_font_montserrat_42, 0);
    lv_obj_set_style_text_color(value, lv_color_black(), 0);
    lv_label_set_long_mode(value, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(value, lv_pct(100));
    lv_obj_align(value, LV_ALIGN_TOP_MID, 0, 12);
    *value_out = value;

    lv_obj_t *label = lv_label_create(card);
    lv_label_set_text(label, caption);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 0);
    return card;
}

}  // namespace

void dashboard_create(lv_display_t *disp) {
    lv_obj_t *scr = lv_display_get_screen_active(disp);
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_text_color(scr, lv_color_black(), 0);
    lv_obj_set_style_pad_all(scr, 0, 0);

    const int32_t pad = 36;
    const int32_t gap = 24;
    const int32_t top_h = 150;
    const int32_t card_w = (TFT_HOR_RES - pad * 2 - gap) / 2;
    const int32_t card_h = (TFT_VER_RES - pad * 2 - top_h - gap) / 2;

    clock_label = lv_label_create(scr);
    lv_obj_set_style_text_font(clock_label, &lv_font_montserrat_48, 0);
    lv_label_set_text(clock_label, "--:--");
    lv_obj_align(clock_label, LV_ALIGN_TOP_LEFT, pad, 36);

    date_label = lv_label_create(scr);
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_28, 0);
    lv_label_set_text(date_label, "Waiting for time");
    lv_obj_align(date_label, LV_ALIGN_TOP_LEFT, pad, 96);

    status_label = lv_label_create(scr);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_16, 0);
    lv_label_set_text(status_label, "Home Assistant");
    lv_obj_align(status_label, LV_ALIGN_TOP_RIGHT, -pad, 48);

    lv_obj_t *temp_card = make_card(scr, "Temperature", &temp_value);
    lv_obj_set_size(temp_card, card_w, card_h);
    lv_obj_set_pos(temp_card, pad, pad + top_h);

    lv_obj_t *hum_card = make_card(scr, "Humidity", &hum_value);
    lv_obj_set_size(hum_card, card_w, card_h);
    lv_obj_set_pos(hum_card, pad + card_w + gap, pad + top_h);

    lv_obj_t *co2_card = make_card(scr, "CO2", &co2_value);
    lv_obj_set_size(co2_card, card_w, card_h);
    lv_obj_set_pos(co2_card, pad, pad + top_h + card_h + gap);

    lv_obj_t *weather_card = make_card(scr, "Weather", &weather_value);
    lv_obj_set_size(weather_card, card_w, card_h);
    lv_obj_set_pos(weather_card, pad + card_w + gap, pad + top_h + card_h + gap);

    weather_detail = lv_label_create(weather_card);
    lv_obj_set_style_text_font(weather_detail, &lv_font_montserrat_16, 0);
    lv_label_set_text(weather_detail, "");
    lv_obj_align(weather_detail, LV_ALIGN_CENTER, 0, 36);
}

void dashboard_update(const HaSnapshot &data) {
    lv_label_set_text(temp_value, format_number(data.temperature, data.temperature_unit.c_str(), 1).c_str());
    lv_label_set_text(hum_value, format_number(data.humidity, data.humidity_unit.c_str(), 0).c_str());
    lv_label_set_text(co2_value, format_number(data.co2, data.co2_unit.c_str(), 0).c_str());

    if (data.weather_condition.length() > 0) {
        lv_label_set_text(weather_value, data.weather_condition.c_str());
    } else {
        lv_label_set_text(weather_value, "--");
    }

    String detail;
    if (!isnan(data.weather_temperature)) {
        detail += format_number(data.weather_temperature, data.temperature_unit.c_str(), 0);
    }
    if (!isnan(data.wind_speed)) {
        if (detail.length()) {
            detail += "  ";
        }
        detail += format_number(data.wind_speed, data.wind_speed_unit.c_str(), 0);
    }
    lv_label_set_text(weather_detail, detail.c_str());

    if (data.ok) {
        lv_label_set_text(status_label, "Home Assistant");
    } else if (data.error.length()) {
        lv_label_set_text(status_label, data.error.c_str());
    }

    time_t now = time(nullptr);
    struct tm t;
    if (now < 1700000000 || localtime_r(&now, &t) == nullptr) {
        return;
    }
    char clock_buf[16];
    char date_buf[48];
    strftime(clock_buf, sizeof(clock_buf), "%H:%M", &t);
    strftime(date_buf, sizeof(date_buf), "%A, %d %b", &t);
    lv_label_set_text(clock_label, clock_buf);
    lv_label_set_text(date_label, date_buf);
}
