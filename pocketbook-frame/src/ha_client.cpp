#include "utils.h"
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
#include <stdlib.h>
#include <string.h>
#include <time.h>

static WiFiClient ha_wifi;
static HARestAPI ha(ha_wifi);
static bool ha_started = false;

static bool ha_configured() {
    return HA_URL[0] != '\0' && HA_TOKEN[0] != '\0';
}

static bool ha_ensure() {
    if (!ha_configured()) {
        return false;
    }
    if (!ha_started) {
        String host;
        uint16_t port = 8123;
        if (!parse_http_url(HA_URL, host, port, 8123)) {
            Logger.println("[ha] HA_URL is invalid");
            return false;
        }
        ha.setHAServer(host, port);
        ha.setHAPassword(HA_TOKEN);
        ha.setDebugMode(false);
        ha.setTimeOut(HA_HTTP_TIMEOUT_MS);
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

static float parse_float(const char *value) {
    if (value == nullptr || value[0] == '\0' || strcmp(value, "unknown") == 0 ||
        strcmp(value, "unavailable") == 0) {
        return NAN;
    }
    char *end = nullptr;
    const float parsed = strtof(value, &end);
    if (end == value) {
        return NAN;
    }
    return parsed;
}

static float parse_float(const String &value) {
    return parse_float(value.c_str());
}

static void copy_json_string(JsonVariantConst value, String &out) {
    if (!value.isNull()) {
        out = value.as<String>();
    }
}

static void copy_json_float(JsonVariantConst value, float &out) {
    if (!value.isNull()) {
        out = value.as<float>();
    }
}

static bool ha_get_numeric_sensor(const char *entity_id, JsonDocument &doc, float &value, String &unit) {
    String state;
    if (!ha_get_state(entity_id, state, doc)) {
        return false;
    }
    value = parse_float(state);
    copy_json_string(doc["attributes"]["unit_of_measurement"], unit);
    return true;
}

static bool is_leap_year(int year) {
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

static time_t utc_from_parts(int year, int month, int day, int hour, int minute, int second) {
    static const int month_days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int days = 0;
    for (int y = 1970; y < year; y++) {
        days += is_leap_year(y) ? 366 : 365;
    }
    for (int m = 0; m < month - 1; m++) {
        days += month_days[m];
        if (m == 1 && is_leap_year(year)) {
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

struct HistoryBuckets {
    float sums[HA_HISTORY_POINTS] = {};
    uint16_t counts[HA_HISTORY_POINTS] = {};
    uint32_t samples = 0;
};

static void history_add_sample(HistoryBuckets &buckets, time_t start, time_t span, time_t t, float value) {
    if (span <= 0 || t <= 0 || isnan(value)) {
        return;
    }
    int32_t bucket = static_cast<int32_t>(((int64_t)(t - start) * HA_HISTORY_POINTS) / span);
    if (bucket < 0) {
        bucket = 0;
    }
    if (bucket >= static_cast<int32_t>(HA_HISTORY_POINTS)) {
        bucket = HA_HISTORY_POINTS - 1;
    }
    buckets.sums[bucket] += value;
    buckets.counts[bucket]++;
    buckets.samples++;
}

static void history_finish(const HistoryBuckets &buckets, float *out) {
    float last = NAN;
    for (uint32_t i = 0; i < HA_HISTORY_POINTS; i++) {
        if (buckets.counts[i] > 0) {
            last = buckets.sums[i] / buckets.counts[i];
        }
        out[i] = last;
    }
}

static bool parse_history_stream(Stream &stream, time_t start, time_t end, HaSnapshot &out, uint32_t &temp_samples,
                                 uint32_t &hum_samples) {
    // HA returns [[temp points...], [co2 points...]]. Parse one object at a time:
    // https://arduinojson.org/v7/how-to/deserialize-a-very-large-document/
    stream.setTimeout(HA_HTTP_TIMEOUT_MS);
    if (!stream.find("[")) {
        Logger.println("[ha] history json: expected array");
        return false;
    }

    JsonDocument filter;
    filter["entity_id"] = true;
    filter["state"] = true;
    filter["last_changed"] = true;

    const time_t span = end - start;
    HistoryBuckets temp_buckets;
    HistoryBuckets hum_buckets;
    JsonDocument point;
    uint32_t series_index = 0;

    while (stream.findUntil("[", "]")) {
        HistoryBuckets *buckets = nullptr;
        bool first = true;
        do {
            point.clear();
            const DeserializationError err =
                deserializeJson(point, stream, DeserializationOption::Filter(filter));
            if (err) {
                Logger.printf("[ha] history point json error: %s\n", err.c_str());
                return false;
            }

            if (first) {
                const char *entity_id = point["entity_id"] | "";
                if (strcmp(entity_id, HA_ENTITY_TEMPERATURE) == 0) {
                    buckets = &temp_buckets;
                } else if (strcmp(entity_id, HA_ENTITY_CO2) == 0) {
                    buckets = &hum_buckets;
                } else if (series_index == 0) {
                    buckets = &temp_buckets;
                } else if (series_index == 1) {
                    buckets = &hum_buckets;
                }
                first = false;
            }
            if (buckets != nullptr) {
                history_add_sample(*buckets, start, span, parse_ha_time(point["last_changed"] | ""),
                                   parse_float(point["state"] | ""));
            }
            if ((temp_buckets.samples + hum_buckets.samples) % 64 == 0) {
                yield();
            }
        } while (stream.findUntil(",", "]"));
        series_index++;
    }

    history_finish(temp_buckets, out.temperature_history);
    history_finish(hum_buckets, out.humidity_history);
    temp_samples = temp_buckets.samples;
    hum_samples = hum_buckets.samples;
    return true;
}

static void ha_fetch_history(HaSnapshot &out) {
    if (!clock_is_set()) {
        Logger.println("[ha] history skipped, clock not set");
        return;
    }
    const time_t now = time(nullptr);
    const time_t start = now - static_cast<time_t>(HA_HISTORY_HOURS) * 3600;
    char start_iso[32];
    format_iso_utc(start, start_iso, sizeof(start_iso));

    String path = "/api/history/period/";
    path += start_iso;
    path += "?filter_entity_id=";
    path += HA_ENTITY_TEMPERATURE;
    path += ",";
    path += HA_ENTITY_HUMIDITY;
    path += "&minimal_response&no_attributes";

    String url = HA_URL;
    url += path;

    WiFiClient client;
    HTTPClient http;
    client.setTimeout(HA_HTTP_TIMEOUT_MS);
    http.setTimeout(HA_HTTP_TIMEOUT_MS);
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

    uint32_t temp_samples = 0;
    uint32_t hum_samples = 0;
    const bool parsed = parse_history_stream(http.getStream(), start, now, out, temp_samples, hum_samples);
    http.end();
    if (!parsed) {
        return;
    }
    Logger.printf("[ha] history %dh loaded t=%u co2=%u\n", HA_HISTORY_HOURS, temp_samples, hum_samples);
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

static bool ha_http_post_json(const String &path, const String &body, JsonDocument &filter, JsonDocument &doc) {
    String url = HA_URL;
    url += path;

    WiFiClient client;
    HTTPClient http;
    http.setTimeout(HA_HTTP_TIMEOUT_MS);
    if (!http.begin(client, url)) {
        Logger.printf("[ha] POST begin failed %s\n", path.c_str());
        return false;
    }
    http.addHeader("Authorization", String("Bearer ") + HA_TOKEN);
    http.addHeader("Accept", "application/json");
    http.addHeader("Content-Type", "application/json");
    const int code = http.POST(body);
    if (code != HTTP_CODE_OK) {
        Logger.printf("[ha] POST %s failed %d\n", path.c_str(), code);
        http.end();
        return false;
    }

    const String payload = http.getString();
    http.end();
    const DeserializationError err = deserializeJson(doc, payload, DeserializationOption::Filter(filter));
    if (err) {
        Logger.printf("[ha] forecast json error: %s (%u bytes)\n", err.c_str(), payload.length());
        return false;
    }
    return true;
}

static JsonArrayConst find_forecast_series(JsonVariantConst doc) {
    JsonVariantConst response = doc["service_response"];
    if (response.is<JsonObjectConst>()) {
        JsonArrayConst direct = response[HA_ENTITY_WEATHER]["forecast"].as<JsonArrayConst>();
        if (!direct.isNull()) {
            return direct;
        }
        for (JsonPairConst kv : response.as<JsonObjectConst>()) {
            JsonArrayConst series = kv.value()["forecast"].as<JsonArrayConst>();
            if (!series.isNull()) {
                return series;
            }
        }
    }
    JsonArrayConst fallback = doc[HA_ENTITY_WEATHER]["forecast"].as<JsonArrayConst>();
    if (!fallback.isNull()) {
        return fallback;
    }
    return doc["forecast"].as<JsonArrayConst>();
}

static void fill_forecast(JsonArrayConst series, HaForecastPoint *out, uint8_t max_count, uint8_t &count, bool daily) {
    count = 0;
    const time_t now = clock_is_set() ? time(nullptr) : 0;
    const time_t period = daily ? 86400 : 3600;
    for (JsonObjectConst point : series) {
        if (count >= max_count) {
            break;
        }
        const time_t t = parse_ha_time(point["datetime"] | "");
        if (now > 0 && t > 0 && (t + period) <= now) {
            continue;
        }
        HaForecastPoint &dest = out[count];
        dest.datetime = t;
        dest.condition = point["condition"] | "";
        copy_json_float(point["temperature"], dest.temperature);
        copy_json_float(point["templow"], dest.templow);
        copy_json_float(point["precipitation_probability"], dest.precipitation_probability);
        dest.is_daytime = true;
        if (!point["is_daytime"].isNull()) {
            dest.is_daytime = point["is_daytime"].as<bool>();
        } else if (!daily && t > 0) {
            struct tm local;
            if (localtime_r(&t, &local) != nullptr) {
                dest.is_daytime = local.tm_hour >= 6 && local.tm_hour < 21;
            }
        }
        count++;
    }
}

static bool ha_fetch_forecast_type(const char *type, HaForecastPoint *out, uint8_t max_count, uint8_t &count) {
    String body = "{\"entity_id\":\"";
    body += HA_ENTITY_WEATHER;
    body += "\",\"type\":\"";
    body += type;
    body += "\"}";

    JsonDocument filter;
    filter["service_response"] = true;

    JsonDocument doc;
    if (!ha_http_post_json("/api/services/weather/get_forecasts?return_response=true", body, filter, doc)) {
        return false;
    }

    JsonArrayConst series = find_forecast_series(doc);
    if (series.isNull()) {
        Logger.printf("[ha] %s forecast missing in response\n", type);
        return false;
    }
    fill_forecast(series, out, max_count, count, strcmp(type, "daily") == 0);
    return count > 0;
}

static void ha_fetch_forecasts(HaSnapshot &out) {
    ha_fetch_forecast_type("daily", out.daily, HA_FORECAST_DAILY_COUNT, out.daily_count);
    ha_fetch_forecast_type("hourly", out.hourly, HA_FORECAST_HOURLY_COUNT, out.hourly_count);
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
    ha_get_numeric_sensor(HA_ENTITY_TEMPERATURE, doc, out.temperature, out.temperature_unit);
    ha_get_numeric_sensor(HA_ENTITY_HUMIDITY, doc, out.humidity, out.humidity_unit);
    ha_get_numeric_sensor(HA_ENTITY_CO2, doc, out.co2, out.co2_unit);

    String state;
    if (ha_get_state(HA_ENTITY_WEATHER, state, doc)) {
        out.weather_condition = friendly_condition(state);
        JsonObject weather_attrs = doc["attributes"].as<JsonObject>();
        copy_json_float(weather_attrs["temperature"], out.weather_temperature);
        copy_json_float(weather_attrs["humidity"], out.weather_humidity);
        copy_json_float(weather_attrs["wind_speed"], out.wind_speed);
        copy_json_string(weather_attrs["wind_speed_unit"], out.wind_speed_unit);
        if (out.temperature_unit == "°C") {
            copy_json_string(weather_attrs["temperature_unit"], out.temperature_unit);
        }
    }

    ha_fetch_forecasts(out);
    ha_fetch_history(out);

    out.ok = true;
    Logger.printf("[ha] t=%.1f h=%.0f co2=%.0f weather=%s daily=%u hourly=%u\n",
                  out.temperature, out.humidity, out.co2, out.weather_condition.c_str(),
                  out.daily_count, out.hourly_count);
    return true;
}
