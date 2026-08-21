#pragma once

#include "config.h"

#include <Arduino.h>
#include <math.h>

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
    float temperature_history[HA_HISTORY_POINTS];
    float co2_history[HA_HISTORY_POINTS];
    bool ok = false;
    String error;

    HaSnapshot() {
        for (uint32_t i = 0; i < HA_HISTORY_POINTS; i++) {
            temperature_history[i] = NAN;
            co2_history[i] = NAN;
        }
    }
};

bool ha_fetch(HaSnapshot &out);
