#include "utils.h"
#include "config.h"
#include "dashboard.h"
#include "weather_icons.h"

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
constexpr int32_t kForecastIconSize = 48;
constexpr int32_t kChartYScaleW = 48;
constexpr int32_t kChartTitleH = 24;
constexpr int32_t kChartXLabelH = 18;

lv_obj_t *clock_label = nullptr;
lv_obj_t *date_label = nullptr;
lv_obj_t *status_label = nullptr;
lv_obj_t *temp_value = nullptr;
lv_obj_t *hum_value = nullptr;
lv_obj_t *co2_value = nullptr;
lv_obj_t *temp_arc = nullptr;
lv_obj_t *hum_arc = nullptr;
lv_obj_t *co2_arc = nullptr;
lv_obj_t *temp_chart = nullptr;
lv_obj_t *co2_chart = nullptr;
lv_chart_series_t *temp_series = nullptr;
lv_chart_series_t *co2_series = nullptr;

struct ForecastSlot {
    lv_obj_t *column = nullptr;
    lv_obj_t *when = nullptr;
    lv_obj_t *icon = nullptr;
    lv_obj_t *temp = nullptr;
    lv_obj_t *templow = nullptr;
};

ForecastSlot daily_slots[HA_FORECAST_DAILY_COUNT];
ForecastSlot hourly_slots[HA_FORECAST_HOURLY_COUNT];

String format_number(float value, const char *suffix, int32_t decimals) {
    if (isnan(value)) {
        return "--";
    }
    char buf[32];
    const char *space = (suffix[0] != '\0' && suffix[0] != '%' && (unsigned char)suffix[0] < 0x80) ? " " : "";
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

void style_plain(lv_obj_t *obj) {
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_pad_gap(obj, 2, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t *make_forecast_slot(lv_obj_t *parent, ForecastSlot *slot, bool show_low) {
    lv_obj_t *column = lv_obj_create(parent);
    style_plain(column);
    lv_obj_set_flex_flow(column, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(column, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_flex_grow(column, 1);
    lv_obj_set_height(column, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(column, 2, 0);

    lv_obj_t *when = lv_label_create(column);
    lv_label_set_text(when, "--");
    lv_obj_set_style_text_font(when, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(when, lv_color_hex(0x555555), 0);
    lv_label_set_long_mode(when, LV_LABEL_LONG_CLIP);

    lv_obj_t *icon = lv_image_create(column);
    lv_obj_set_size(icon, kForecastIconSize, kForecastIconSize);
    lv_image_set_src(icon, weather_icon_for_condition("cloudy", false));

    lv_obj_t *temp = lv_label_create(column);
    lv_label_set_text(temp, "--");
    lv_obj_set_style_text_font(temp, &lv_font_montserrat_24, 0);

    lv_obj_t *templow = lv_label_create(column);
    lv_label_set_text(templow, "");
    lv_obj_set_style_text_font(templow, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(templow, lv_color_hex(0x666666), 0);
    if (!show_low) {
        lv_obj_add_flag(templow, LV_OBJ_FLAG_HIDDEN);
    }

    slot->column = column;
    slot->when = when;
    slot->icon = icon;
    slot->temp = temp;
    slot->templow = templow;
    return column;
}

lv_obj_t *make_forecast_row(lv_obj_t *parent, ForecastSlot *slots, uint8_t count, bool show_low) {
    lv_obj_t *row = lv_obj_create(parent);
    style_plain(row);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 0);
    lv_obj_set_flex_grow(row, 1);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    for (uint8_t i = 0; i < count; i++) {
        make_forecast_slot(row, &slots[i], show_low);
    }
    return row;
}

lv_obj_t *make_forecast_card(lv_obj_t *parent, int32_t width, int32_t height) {
    lv_obj_t *card = lv_obj_create(parent);
    style_card(card);
    lv_obj_set_size(card, width, height);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(card, 16, 0);
    lv_obj_set_style_pad_row(card, 6, 0);

    lv_obj_t *daily_title = lv_label_create(card);
    lv_label_set_text(daily_title, "Daily");
    lv_obj_set_style_text_font(daily_title, &lv_font_montserrat_16, 0);
    lv_obj_set_width(daily_title, lv_pct(100));

    make_forecast_row(card, daily_slots, HA_FORECAST_DAILY_COUNT, true);

    lv_obj_t *hourly_title = lv_label_create(card);
    lv_label_set_text(hourly_title, "Hourly");
    lv_obj_set_style_text_font(hourly_title, &lv_font_montserrat_16, 0);
    lv_obj_set_width(hourly_title, lv_pct(100));
    lv_obj_set_style_pad_top(hourly_title, 8, 0);

    make_forecast_row(card, hourly_slots, HA_FORECAST_HOURLY_COUNT, false);
    return card;
}

void set_temp_label(lv_obj_t *label, float value) {
    if (isnan(value)) {
        lv_label_set_text(label, "--");
        return;
    }
    char buf[12];
    snprintf(buf, sizeof(buf), "%.0f°", value);
    lv_label_set_text(label, buf);
}

void update_forecast_slots(ForecastSlot *slots, uint8_t slot_count, const HaForecastPoint *points, uint8_t count,
                           bool daily) {
    for (uint8_t i = 0; i < slot_count; i++) {
        ForecastSlot &slot = slots[i];
        if (i >= count) {
            lv_obj_add_flag(slot.column, LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        lv_obj_remove_flag(slot.column, LV_OBJ_FLAG_HIDDEN);

        const HaForecastPoint &point = points[i];
        char when_buf[8] = "--";
        struct tm local;
        if (point.datetime > 0 && localtime_r(&point.datetime, &local) != nullptr) {
            strftime(when_buf, sizeof(when_buf), daily ? "%a" : "%H:%M", &local);
        }
        lv_label_set_text(slot.when, when_buf);
        lv_image_set_src(slot.icon, weather_icon_for_condition(point.condition.c_str(), !point.is_daytime));
        set_temp_label(slot.temp, point.temperature);
        if (daily) {
            set_temp_label(slot.templow, point.templow);
            lv_obj_remove_flag(slot.templow, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(slot.templow, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

lv_obj_t *make_gauge_card(lv_obj_t *parent, const char *caption, int32_t width, int32_t height, int32_t min_value,
                          int32_t max_value, lv_obj_t **value_out, lv_obj_t **arc_out) {
    lv_obj_t *card = lv_obj_create(parent);
    style_card(card);
    lv_obj_set_size(card, width, height);

    add_caption(card, caption);

    const int32_t arc_size = LV_MIN(width - 28, height - 52);
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

lv_obj_t *make_chart(lv_obj_t *parent, const char *title, int32_t x, int32_t y, int32_t width, int32_t height,
                     int32_t min_value, int32_t max_value, lv_chart_series_t **series_out) {
    const int32_t plot_h = height - kChartTitleH - kChartXLabelH;
    const int32_t plot_w = width - kChartYScaleW;

    lv_obj_t *title_label = lv_label_create(parent);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(title_label, x + kChartYScaleW, y);

    lv_obj_t *scale = lv_scale_create(parent);
    lv_obj_set_size(scale, kChartYScaleW, plot_h);
    lv_obj_set_pos(scale, x, y + kChartTitleH);
    lv_scale_set_mode(scale, LV_SCALE_MODE_VERTICAL_LEFT);
    lv_scale_set_range(scale, min_value, max_value);
    lv_scale_set_total_tick_count(scale, 3);
    lv_scale_set_major_tick_every(scale, 1);
    lv_scale_set_label_show(scale, true);
    lv_obj_set_style_pad_ver(scale, 10, 0);
    lv_obj_set_style_pad_right(scale, 4, 0);
    lv_obj_set_style_text_font(scale, &lv_font_montserrat_14, LV_PART_INDICATOR);
    lv_obj_set_style_text_font(scale, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(scale, lv_color_black(), LV_PART_INDICATOR);
    lv_obj_set_style_line_color(scale, lv_color_black(), LV_PART_INDICATOR);
    lv_obj_set_style_line_width(scale, 2, LV_PART_INDICATOR);
    lv_obj_set_style_length(scale, 6, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(scale, 0, LV_PART_ITEMS);
    lv_obj_set_style_line_width(scale, 2, LV_PART_MAIN);

    lv_obj_t *chart = lv_chart_create(parent);
    lv_obj_set_size(chart, plot_w, plot_h);
    lv_obj_set_pos(chart, x + kChartYScaleW, y + kChartTitleH);
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

    char start_label[12];
    snprintf(start_label, sizeof(start_label), "-%dh", HA_HISTORY_HOURS);
    lv_obj_t *x_start = lv_label_create(parent);
    lv_label_set_text(x_start, start_label);
    lv_obj_set_style_text_font(x_start, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(x_start, lv_color_hex(0x555555), 0);
    lv_obj_set_pos(x_start, x + kChartYScaleW, y + kChartTitleH + plot_h);

    lv_obj_t *x_end = lv_label_create(parent);
    lv_label_set_text(x_end, "now");
    lv_obj_set_style_text_font(x_end, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(x_end, lv_color_hex(0x555555), 0);
    lv_obj_align_to(x_end, chart, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 0);
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
    const int32_t cards_top = pad + header_h;
    const int32_t inner_w = TFT_HOR_RES - pad * 2;
    const int32_t content_h = TFT_VER_RES - pad - cards_top;
    const int32_t gauge_h = 196;
    const int32_t chart_block_h = 220;
    const int32_t forecast_h = content_h - gauge_h - chart_block_h - gap * 2;
    const int32_t gauge_w = (inner_w - gap * 2) / 3;
    const int32_t chart_w = (inner_w - gap) / 2;
    const int32_t forecast_top = cards_top + gauge_h + gap;
    const int32_t charts_top = forecast_top + forecast_h + gap;

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
        make_gauge_card(scr, "Temperature", gauge_w, gauge_h, kTempGaugeMin, kTempGaugeMax, &temp_value, &temp_arc);
    lv_obj_set_pos(temp_card, pad, cards_top);

    lv_obj_t *hum_card =
        make_gauge_card(scr, "Humidity", gauge_w, gauge_h, kHumGaugeMin, kHumGaugeMax, &hum_value, &hum_arc);
    lv_obj_set_pos(hum_card, pad + gauge_w + gap, cards_top);

    lv_obj_t *co2_card =
        make_gauge_card(scr, "CO2", gauge_w, gauge_h, kCo2GaugeMin, kCo2GaugeMax, &co2_value, &co2_arc);
    lv_obj_set_pos(co2_card, pad + (gauge_w + gap) * 2, cards_top);

    lv_obj_t *forecast_card = make_forecast_card(scr, inner_w, forecast_h);
    lv_obj_set_pos(forecast_card, pad, forecast_top);

    char temp_title[40];
    char co2_title[40];
    snprintf(temp_title, sizeof(temp_title), "Temperature · %dh", HA_HISTORY_HOURS);
    snprintf(co2_title, sizeof(co2_title), "CO2 · %dh", HA_HISTORY_HOURS);
    temp_chart = make_chart(scr, temp_title, pad, charts_top, chart_w, chart_block_h, kTempGaugeMin, kTempGaugeMax,
                            &temp_series);
    co2_chart = make_chart(scr, co2_title, pad + chart_w + gap, charts_top, chart_w, chart_block_h, kCo2GaugeMin,
                           kCo2GaugeMax, &co2_series);
}

void dashboard_update(const HaSnapshot &data) {
    update_gauge(temp_value, temp_arc, data.temperature, "°", 1, kTempGaugeMin, kTempGaugeMax);
    update_gauge(hum_value, hum_arc, data.humidity, "%", 0, kHumGaugeMin, kHumGaugeMax);
    update_gauge(co2_value, co2_arc, data.co2, "", 0, kCo2GaugeMin, kCo2GaugeMax);

    set_chart_history(temp_chart, temp_series, data.temperature_history);
    set_chart_history(co2_chart, co2_series, data.co2_history);

    update_forecast_slots(daily_slots, HA_FORECAST_DAILY_COUNT, data.daily, data.daily_count, true);
    update_forecast_slots(hourly_slots, HA_FORECAST_HOURLY_COUNT, data.hourly, data.hourly_count, false);

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
