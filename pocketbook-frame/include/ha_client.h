#pragma once

#include "config.h"

#include <Arduino.h>
#include <math.h>
#include <time.h>

struct HaForecastPoint {
    time_t datetime = 0;
    String condition;
    float temperature = NAN;
    float templow = NAN;
    float precipitation_probability = NAN;
    bool is_daytime = true;
};

struct HaSnapshot {
    float temperature = NAN;
    float humidity = NAN;
    float co2 = NAN;
    String temperature_unit = "°C";
    String humidity_unit = "%";
    String co2_unit = "ppm";

    String weather_condition;
    float weather_temperature = NAN;
    float weather_humidity = NAN;
    float wind_speed = NAN;
    String wind_speed_unit = "km/h";
    HaForecastPoint daily[HA_FORECAST_DAILY_COUNT];
    HaForecastPoint hourly[HA_FORECAST_HOURLY_COUNT];
    uint8_t daily_count = 0;
    uint8_t hourly_count = 0;
    float temperature_history[HA_HISTORY_POINTS];
    float humidity_history[HA_HISTORY_POINTS];
    bool ok = false;
    String error;

    HaSnapshot() {
        for (uint32_t i = 0; i < HA_HISTORY_POINTS; i++) {
            temperature_history[i] = NAN;
            humidity_history[i] = NAN;
        }
    }
};

bool ha_fetch(HaSnapshot &out);
