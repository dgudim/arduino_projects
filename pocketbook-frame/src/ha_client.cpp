#include "ha_client.h"
#include "config.h"
#include "web_log.h"

#include <ArduinoJson.h>
#include <HARestAPI.h>
#include <WiFi.h>
#include <ctype.h>
#include <math.h>

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

    out.ok = true;
    Logger.printf("[ha] t=%.1f h=%.0f co2=%.0f weather=%s\n",
                  out.temperature, out.humidity, out.co2, out.weather_condition.c_str());
    return true;
}
