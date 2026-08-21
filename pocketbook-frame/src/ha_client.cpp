#include "config.h"
#include "ha_client.h"
#include "web_log.h"

#include <ArduinoJson.h>
#include <HARestAPI.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static WiFiClient ha_wifi;
static HARestAPI ha(ha_wifi);
static bool ha_started = false;

static bool ha_configured() {
    return HA_TOKEN[0] != '\0';
}

static bool ha_ensure() {
    if (!ha_configured()) {
        return false;
    }
    if (!ha_started) {
        ha.setHAServer(HA_HOST, HA_PORT);
        ha.setHAPassword(HA_TOKEN);
        ha.setDebugMode(false);
        ha.setTimeOut(5000);
        ha_started = true;
    }
    return true;
}

static bool parse_state_doc(const String &payload, JsonDocument &doc) {
    if (payload.isEmpty()) {
        return false;
    }
    const DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Logger.printf("[ha] json error: %s\n", err.c_str());
        return false;
    }
    return true;
}

static bool ha_get_state(const char *entity_id, String &state, JsonDocument &doc) {
    String path = "/api/states/";
    path += entity_id;
    if (!parse_state_doc(ha.sendGetHA(path), doc)) {
        Logger.printf("[ha] GET %s failed\n", entity_id);
        return false;
    }
    state = doc["state"].as<String>();
    return state.length() > 0 && state != "unknown" && state != "unavailable";
}

static float parse_float(const String &value) {
    if (value.length() == 0) {
        return NAN;
    }
    return value.toFloat();
}

static time_t utc_from_parts(int year, int month, int day, int hour, int minute, int second) {
    static const int month_days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int days = 0;
    for (int y = 1970; y < year; y++) {
        days += (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 366 : 365;
    }
    for (int m = 0; m < month - 1; m++) {
        days += month_days[m];
        if (m == 1 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))) {
            days++;
        }
    }
    days += day - 1;
    return static_cast<time_t>(days) * 86400 + hour * 3600 + minute * 60 + second;
}

static time_t parse_ha_time(const char *text) {
    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
    if (text == nullptr || sscanf(text, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6) {
        return 0;
    }
    time_t utc = utc_from_parts(year, month, day, hour, minute, second);
    const char *tpos = strchr(text, 'T');
    if (tpos == nullptr || strchr(tpos, 'Z') != nullptr) {
        return utc;
    }
    const char *offset = strpbrk(tpos + 1, "+-");
    if (offset == nullptr) {
        return utc;
    }
    int offset_hour = 0, offset_min = 0;
    sscanf(offset, "%d:%d", &offset_hour, &offset_min);
    if (offset_hour < 0) {
        offset_min = -offset_min;
    }
    return utc - (offset_hour * 3600 + offset_min * 60);
}

static void format_iso_utc(time_t t, char *buf, size_t buf_len) {
    struct tm utc;
    gmtime_r(&t, &utc);
    strftime(buf, buf_len, "%Y-%m-%dT%H:%M:%SZ", &utc);
}

static void downsample_history(JsonArray series, time_t start, time_t end, float *out) {
    float sums[HA_HISTORY_POINTS] = {};
    uint16_t counts[HA_HISTORY_POINTS] = {};
    const time_t span = end - start;
    if (span <= 0) {
        return;
    }

    for (JsonObject point : series) {
        const float value = parse_float(point["state"].as<String>());
        if (isnan(value)) {
            continue;
        }
        const time_t t = parse_ha_time(point["last_changed"] | "");
        if (t <= 0) {
            continue;
        }
        int32_t bucket = static_cast<int32_t>(((int64_t)(t - start) * HA_HISTORY_POINTS) / span);
        if (bucket < 0) {
            bucket = 0;
        }
        if (bucket >= static_cast<int32_t>(HA_HISTORY_POINTS)) {
            bucket = HA_HISTORY_POINTS - 1;
        }
        sums[bucket] += value;
        counts[bucket]++;
    }

    float last = NAN;
    for (uint32_t i = 0; i < HA_HISTORY_POINTS; i++) {
        if (counts[i] > 0) {
            last = sums[i] / counts[i];
        }
        out[i] = last;
    }
}

static void ha_fetch_history(HaSnapshot &out) {
    const time_t now = time(nullptr);
    if (now < 1700000000) {
        Logger.println("[ha] history skipped, clock not set");
        return;
    }
    const time_t start = now - static_cast<time_t>(HA_HISTORY_HOURS) * 3600;
    char start_iso[32];
    format_iso_utc(start, start_iso, sizeof(start_iso));

    String path = "/api/history/period/";
    path += start_iso;
    path += "?filter_entity_id=";
    path += HA_ENTITY_TEMPERATURE;
    path += ",";
    path += HA_ENTITY_CO2;
    path += "&minimal_response&no_attributes";

    String url = "http://";
    url += HA_HOST;
    url += ":";
    url += String(HA_PORT);
    url += path;

    WiFiClient client;
    HTTPClient http;
    http.setTimeout(15000);
    if (!http.begin(client, url)) {
        Logger.println("[ha] history begin failed");
        return;
    }
    http.addHeader("Authorization", String("Bearer ") + HA_TOKEN);
    http.addHeader("Accept", "application/json");
    const int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Logger.printf("[ha] history GET failed %d\n", code);
        http.end();
        return;
    }

    JsonDocument filter;
    filter[0][0]["entity_id"] = true;
    filter[0][0]["state"] = true;
    filter[0][0]["last_changed"] = true;

    JsonDocument doc;
    const DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
    http.end();
    if (err) {
        Logger.printf("[ha] history json error: %s\n", err.c_str());
        return;
    }

    JsonArray root = doc.as<JsonArray>();
    uint32_t series_index = 0;
    for (JsonArray series : root) {
        const char *entity_id = series.size() ? series[0]["entity_id"] | "" : "";
        float *dest = nullptr;
        if (strcmp(entity_id, HA_ENTITY_TEMPERATURE) == 0) {
            dest = out.temperature_history;
        } else if (strcmp(entity_id, HA_ENTITY_CO2) == 0) {
            dest = out.co2_history;
        } else if (series_index == 0) {
            dest = out.temperature_history;
        } else if (series_index == 1) {
            dest = out.co2_history;
        }
        series_index++;
        if (dest != nullptr) {
            downsample_history(series, start, now, dest);
        }
    }
    Logger.printf("[ha] history %dh loaded\n", HA_HISTORY_HOURS);
}

static String friendly_condition(const String &raw) {
    if (raw == "clear-night") return "Clear night";
    if (raw == "partlycloudy") return "Partly cloudy";
    if (raw == "lightning-rainy") return "Storms";
    if (raw == "snowy-rainy") return "Sleet";
    if (raw == "windy-variant") return "Windy";
    String out = raw;
    if (out.length() > 0) {
        out.setCharAt(0, toupper(out.charAt(0)));
        out.replace("-", " ");
    }
    return out;
}

bool ha_fetch(HaSnapshot &out) {
    out = HaSnapshot{};
    if (WiFi.status() != WL_CONNECTED) {
        out.error = "wifi down";
        return false;
    }
    if (!ha_configured()) {
        out.error = "HA_TOKEN is empty";
        static bool warned = false;
        if (!warned) {
            Logger.println("[ha] set HA_TOKEN in include/config.h");
            warned = true;
        }
        return false;
    }
    if (!ha_ensure()) {
        out.error = "HA client init failed";
        return false;
    }

    JsonDocument doc;
    String state;

    if (ha_get_state(HA_ENTITY_TEMPERATURE, state, doc)) {
        out.temperature = parse_float(state);
        if (!doc["attributes"]["unit_of_measurement"].isNull()) {
            out.temperature_unit = doc["attributes"]["unit_of_measurement"].as<String>();
        }
    }
    if (ha_get_state(HA_ENTITY_HUMIDITY, state, doc)) {
        out.humidity = parse_float(state);
        if (!doc["attributes"]["unit_of_measurement"].isNull()) {
            out.humidity_unit = doc["attributes"]["unit_of_measurement"].as<String>();
        }
    }
    if (ha_get_state(HA_ENTITY_CO2, state, doc)) {
        out.co2 = parse_float(state);
        if (!doc["attributes"]["unit_of_measurement"].isNull()) {
            out.co2_unit = doc["attributes"]["unit_of_measurement"].as<String>();
        }
    }

    if (ha_get_state(HA_ENTITY_WEATHER, state, doc)) {
        out.weather_condition = friendly_condition(state);
        JsonObject weather_attrs = doc["attributes"].as<JsonObject>();
        if (!weather_attrs["temperature"].isNull()) {
            out.weather_temperature = weather_attrs["temperature"].as<float>();
        }
        if (!weather_attrs["humidity"].isNull()) {
            out.weather_humidity = weather_attrs["humidity"].as<float>();
        }
        if (!weather_attrs["wind_speed"].isNull()) {
            out.wind_speed = weather_attrs["wind_speed"].as<float>();
        }
        if (!weather_attrs["wind_speed_unit"].isNull()) {
            out.wind_speed_unit = weather_attrs["wind_speed_unit"].as<String>();
        }
        if (!weather_attrs["temperature_unit"].isNull() && out.temperature_unit == "°C") {
            out.temperature_unit = weather_attrs["temperature_unit"].as<String>();
        }
    }

    ha_fetch_history(out);

    out.ok = true;
    Logger.printf("[ha] t=%.1f h=%.0f co2=%.0f weather=%s\n",
                  out.temperature, out.humidity, out.co2, out.weather_condition.c_str());
    return true;
}
