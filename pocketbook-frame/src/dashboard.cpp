#include "utils.h"
#include "config.h"
#include "dashboard.h"

#include <math.h>
#include <stdio.h>
#include <time.h>

namespace {

constexpr uint32_t kChartPoints = HA_HISTORY_POINTS;
constexpr int32_t kTempGaugeMin = 5;
constexpr int32_t kTempGaugeMax = 35;
constexpr int32_t kHumGaugeMin = 0;
constexpr int32_t kHumGaugeMax = 100;
constexpr int32_t kCo2GaugeMin = 400;
constexpr int32_t kCo2GaugeMax = 2000;

lv_obj_t *clock_label = nullptr;
lv_obj_t *date_label = nullptr;
lv_obj_t *status_label = nullptr;
lv_obj_t *temp_value = nullptr;
lv_obj_t *hum_value = nullptr;
lv_obj_t *co2_value = nullptr;
lv_obj_t *weather_value = nullptr;
lv_obj_t *weather_detail = nullptr;
lv_obj_t *temp_arc = nullptr;
lv_obj_t *hum_arc = nullptr;
lv_obj_t *co2_arc = nullptr;
lv_obj_t *temp_chart = nullptr;
lv_obj_t *co2_chart = nullptr;
lv_chart_series_t *temp_series = nullptr;
lv_chart_series_t *co2_series = nullptr;

String format_number(float value, const char *suffix, int32_t decimals) {
    if (isnan(value)) {
        return "--";
    }
    char buf[32];
    const char *space = (suffix[0] == '%' || suffix[0] == '\0') ? "" : " ";
    snprintf(buf, sizeof(buf), "%.*f%s%s", decimals, value, space, suffix);
    return String(buf);
}

int32_t clamp_gauge(float value, int32_t min_value, int32_t max_value) {
    if (isnan(value)) {
        return min_value;
    }
    const int32_t rounded = static_cast<int32_t>(lroundf(value));
    if (rounded < min_value) {
        return min_value;
    }
    if (rounded > max_value) {
        return max_value;
    }
    return rounded;
}

void style_card(lv_obj_t *card) {
    lv_obj_set_style_bg_color(card, lv_color_white(), 0);
    lv_obj_set_style_border_color(card, lv_color_black(), 0);
    lv_obj_set_style_border_width(card, 6, 0);
    lv_obj_set_style_radius(card, 18, 0);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
}

void add_caption(lv_obj_t *parent, const char *caption) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, caption);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 0);
}

lv_obj_t *make_card(lv_obj_t *parent, const char *caption, int32_t width, int32_t height, lv_obj_t **value_out) {
    lv_obj_t *card = lv_obj_create(parent);
    style_card(card);
    lv_obj_set_size(card, width, height);

    lv_obj_t *value = lv_label_create(card);
    lv_label_set_text(value, "--");
    lv_obj_set_style_text_font(value, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(value, lv_color_black(), 0);
    lv_label_set_long_mode(value, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(value, lv_pct(100));
    lv_obj_align(value, LV_ALIGN_TOP_MID, 0, 8);
    *value_out = value;

    add_caption(card, caption);
    return card;
}

lv_obj_t *make_gauge_card(lv_obj_t *parent, const char *caption, int32_t width, int32_t height, int32_t min_value,
                          int32_t max_value, lv_obj_t **value_out, lv_obj_t **arc_out) {
    lv_obj_t *card = lv_obj_create(parent);
    style_card(card);
    lv_obj_set_size(card, width, height);

    add_caption(card, caption);

    const int32_t arc_size = height - 56;
    lv_obj_t *arc = lv_arc_create(card);
    lv_obj_set_size(arc, arc_size, arc_size);
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270);
    lv_arc_set_range(arc, min_value, max_value);
    lv_arc_set_value(arc, min_value);
    lv_obj_remove_style(arc, nullptr, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(arc, 14, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 14, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0xBBBBBB), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_black(), LV_PART_INDICATOR);
    lv_obj_align(arc, LV_ALIGN_TOP_MID, 0, 0);
    *arc_out = arc;

    lv_obj_t *value = lv_label_create(arc);
    lv_label_set_text(value, "--");
    lv_obj_set_style_text_font(value, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(value, lv_color_black(), 0);
    lv_obj_center(value);
    *value_out = value;
    return card;
}

lv_obj_t *make_chart(lv_obj_t *parent, const char *title, int32_t x, int32_t y, int32_t width, int32_t plot_height,
                     int32_t min_value, int32_t max_value, lv_chart_series_t **series_out) {
    lv_obj_t *title_label = lv_label_create(parent);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(title_label, x, y);

    lv_obj_t *chart = lv_chart_create(parent);
    lv_obj_set_size(chart, width, plot_height);
    lv_obj_set_pos(chart, x, y + 26);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, kChartPoints);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_axis_range(chart, LV_CHART_AXIS_PRIMARY_Y, min_value, max_value);
    lv_chart_set_div_line_count(chart, 3, 6);
    lv_obj_set_style_bg_color(chart, lv_color_white(), 0);
    lv_obj_set_style_border_color(chart, lv_color_black(), 0);
    lv_obj_set_style_border_width(chart, 4, 0);
    lv_obj_set_style_radius(chart, 12, 0);
    lv_obj_set_style_pad_all(chart, 10, 0);
    lv_obj_set_style_line_color(chart, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_set_style_line_width(chart, 3, LV_PART_ITEMS);
    lv_obj_set_style_size(chart, 0, 0, LV_PART_INDICATOR);
    lv_obj_clear_flag(chart, LV_OBJ_FLAG_CLICKABLE);

    lv_chart_series_t *series = lv_chart_add_series(chart, lv_color_black(), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_all_values(chart, series, LV_CHART_POINT_NONE);
    *series_out = series;
    return chart;
}

void update_gauge(lv_obj_t *label, lv_obj_t *arc, float value, const char *unit, int32_t decimals, int32_t min_value,
                  int32_t max_value) {
    lv_label_set_text(label, format_number(value, unit, decimals).c_str());
    lv_arc_set_value(arc, clamp_gauge(value, min_value, max_value));
}

void set_chart_history(lv_obj_t *chart, lv_chart_series_t *series, const float *history) {
    if (chart == nullptr || series == nullptr) {
        return;
    }
    lv_chart_set_x_start_point(chart, series, 0);
    for (uint32_t i = 0; i < HA_HISTORY_POINTS; i++) {
        const int32_t value = isnan(history[i]) ? LV_CHART_POINT_NONE : static_cast<int32_t>(lroundf(history[i]));
        lv_chart_set_series_value_by_id(chart, series, i, value);
    }
    lv_chart_refresh(chart);
}

}  // namespace

void dashboard_create(lv_display_t *disp) {
    lv_obj_t *scr = lv_display_get_screen_active(disp);
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_text_color(scr, lv_color_black(), 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    const int32_t pad = 32;
    const int32_t gap = 20;
    const int32_t header_h = 120;
    const int32_t chart_title_h = 26;
    const int32_t chart_plot_h = 140;
    const int32_t charts_block = chart_title_h + chart_plot_h + gap + chart_title_h + chart_plot_h;
    const int32_t cards_top = pad + header_h;
    const int32_t cards_h_total = TFT_VER_RES - pad - charts_block - gap - cards_top;
    const int32_t card_h = (cards_h_total - gap) / 2;
    const int32_t card_w = (TFT_HOR_RES - pad * 2 - gap) / 2;
    const int32_t chart_w = TFT_HOR_RES - pad * 2;
    const int32_t charts_top = cards_top + card_h * 2 + gap * 2;

    clock_label = lv_label_create(scr);
    lv_obj_set_style_text_font(clock_label, &lv_font_montserrat_48, 0);
    lv_label_set_text(clock_label, "--:--");
    lv_obj_align(clock_label, LV_ALIGN_TOP_LEFT, pad, 28);

    date_label = lv_label_create(scr);
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_28, 0);
    lv_label_set_text(date_label, "Waiting for time");
    lv_obj_align(date_label, LV_ALIGN_TOP_LEFT, pad, 84);

    status_label = lv_label_create(scr);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_16, 0);
    lv_label_set_text(status_label, "Home Assistant");
    lv_obj_align(status_label, LV_ALIGN_TOP_RIGHT, -pad, 40);

    lv_obj_t *temp_card =
        make_gauge_card(scr, "Temperature", card_w, card_h, kTempGaugeMin, kTempGaugeMax, &temp_value, &temp_arc);
    lv_obj_set_pos(temp_card, pad, cards_top);

    lv_obj_t *hum_card =
        make_gauge_card(scr, "Humidity", card_w, card_h, kHumGaugeMin, kHumGaugeMax, &hum_value, &hum_arc);
    lv_obj_set_pos(hum_card, pad + card_w + gap, cards_top);

    lv_obj_t *co2_card =
        make_gauge_card(scr, "CO2", card_w, card_h, kCo2GaugeMin, kCo2GaugeMax, &co2_value, &co2_arc);
    lv_obj_set_pos(co2_card, pad, cards_top + card_h + gap);

    lv_obj_t *weather_card = make_card(scr, "Weather", card_w, card_h, &weather_value);
    lv_obj_set_pos(weather_card, pad + card_w + gap, cards_top + card_h + gap);

    weather_detail = lv_label_create(weather_card);
    lv_obj_set_style_text_font(weather_detail, &lv_font_montserrat_16, 0);
    lv_label_set_text(weather_detail, "");
    lv_obj_align(weather_detail, LV_ALIGN_CENTER, 0, 20);

    char temp_title[40];
    char co2_title[40];
    snprintf(temp_title, sizeof(temp_title), "Temperature · %dh", HA_HISTORY_HOURS);
    snprintf(co2_title, sizeof(co2_title), "CO2 · %dh", HA_HISTORY_HOURS);
    temp_chart = make_chart(scr, temp_title, pad, charts_top, chart_w, chart_plot_h, kTempGaugeMin, kTempGaugeMax,
                            &temp_series);
    co2_chart = make_chart(scr, co2_title, pad, charts_top + chart_title_h + chart_plot_h + gap, chart_w, chart_plot_h,
                           kCo2GaugeMin, kCo2GaugeMax, &co2_series);
}

void dashboard_update(const HaSnapshot &data) {
    update_gauge(temp_value, temp_arc, data.temperature, data.temperature_unit.c_str(), 1, kTempGaugeMin, kTempGaugeMax);
    update_gauge(hum_value, hum_arc, data.humidity, data.humidity_unit.c_str(), 0, kHumGaugeMin, kHumGaugeMax);
    update_gauge(co2_value, co2_arc, data.co2, data.co2_unit.c_str(), 0, kCo2GaugeMin, kCo2GaugeMax);

    set_chart_history(temp_chart, temp_series, data.temperature_history);
    set_chart_history(co2_chart, co2_series, data.co2_history);

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

    if (!clock_is_set()) {
        return;
    }
    struct tm t;
    time_t now = time(nullptr);
    if (localtime_r(&now, &t) == nullptr) {
        return;
    }
    
    char clock_buf[16];
    char date_buf[48];
    
    strftime(clock_buf, sizeof(clock_buf), "%H:%M", &t);
    strftime(date_buf, sizeof(date_buf), "%A, %d %b", &t);
    lv_label_set_text(clock_label, clock_buf);
    lv_label_set_text(date_label, date_buf);
}
