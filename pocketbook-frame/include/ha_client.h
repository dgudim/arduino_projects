#pragma once

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
    bool ok = false;
    String error;
};

bool ha_fetch(HaSnapshot &out);
