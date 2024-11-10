#include <Arduino.h>
#include <FastLED.h>

#include <ESPAsyncWebServer.h>
#include <WiFi.h>

#include "driver/temp_sensor.h"

const char *ssid = "BIMBA";
const char *password = "reeeeeee";

AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
    <head>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
        html {
            font-family: Arial;
            display: inline-block;
            margin: 0px auto;
            text-align: center;
        }
        h2 { font-size: 3.0rem; }
        p { font-size: 3.0rem; }
        </style>
    </head>
    <body>
        <h2>Lamp server</h2>
        <p>
            <span>Battery raw:</span>
            <span id="bat_raw">%BAT_RAW%</span>
        </p>
        <p>
            <span>Battery calculated:</span>
            <span id="bat_calc">%BAT_CALC%</span>
        </p>
        <p>
            <span>Has external power:</span>
            <span id="ext_power">%EXT_POWER%</span>
        </p>
        <p>
            <span>Charged enabled:</span>
            <span id="chg_enabled">%CHG_ENABLED%</span>
        </p>
        <p>
            <span>Internal temp:</span>
            <span id="temp">%TEMP%</span>
        </p>
    </body>
</html>)rawliteral";

#define NUM_LEDS 22
CRGB leds[NUM_LEDS];

#define BAT_SENSE_PIN A0
#define CHARGE_ENABLE_PIN 4
#define SHAKER_PIN 10
#define WHITE_PWM_PIN A1
#define WS2815_PIN 2
#define TWELVE_VOLT_OUT_ENABLED_PIN 3
#define CHARGE_SENSE_PIN 7
#define BUZZER_PIN 5
#define BUTTON_PIN 6
#define BUTTON2_PIN 21

#define RENDER_LOOP_DELTA 30                                           // 33 Fps
#define CONTROL_LOOP_DELTA 500                                         // Check 2 times per second
#define BATTERY_SMOOTHING_MULT (1.0 / (60'000.0 / CONTROL_LOOP_DELTA)) // Smooth battery percentage over a minute

#define ADC_RESOLUTION 4096
#define BAT_0_PERCENT (0.667 * ADC_RESOLUTION)
#define BAT_100_PERCENT (0.998 * ADC_RESOLUTION)
#define BAT_100_PERCENT_NORM (0.998 * ADC_RESOLUTION - BAT_0_PERCENT)

float battery_percent_smooth = -1;
int tp4056_enabled = 2; // 2 = unknown state

int has_external_power = 2; // 2 = unknown state
int shake_sensor_prev = 2;

#pragma region EFFECTS

#pragma region BREATHING

class BreathingEffect {
    // https://github.com/marmilicious/FastLED_examples/blob/master/breath_effect_v2.ino
  private:
    constexpr static float pulseSpeed = 0.5; // Larger value gives faster pulse.

    const static uint8_t hueA = 15;          // Start hue at valueMin.
    const static uint8_t satA = 230;         // Start saturation at valueMin.
    constexpr static float valueMin = 120.0; // Pulse minimum value (Should be less then valueMax).

    const static uint8_t hueB = 95;          // End hue at valueMax.
    const static uint8_t satB = 255;         // End saturation at valueMax.
    constexpr static float valueMax = 255.0; // Pulse maximum value (Should be larger then valueMin).

    uint8_t hue = hueA;                                                // Do Not Edit
    uint8_t sat = satA;                                                // Do Not Edit
    float val = valueMin;                                              // Do Not Edit
    const static uint8_t hueDelta = hueA - hueB;                       // Do Not Edit
    constexpr static float delta = (valueMax - valueMin) / 2.35040238; // Do Not Edit

  public:
    void reset() {
        hue = hueA;
        sat = satA;
        val = valueMin;
    }

    void iteration() {
        float dV = ((exp(sin(pulseSpeed * millis() / 2000.0 * PI)) - 0.36787944) * delta);
        val = valueMin + dV;
        hue = map(val, valueMin, valueMax, hueA, hueB); // Map hue based on current val
        sat = map(val, valueMin, valueMax, satA, satB); // Map sat based on current val

        for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = CHSV(hue, sat, val);

            // You can experiment with commenting out these dim8_video lines
            // to get a different sort of look.
            leds[i].r = dim8_video(leds[i].r);
            // leds[i].g = dim8_video(leds[i].g);
            // leds[i].b = dim8_video(leds[i].b);
        }

        FastLED.show();
    }
};

#pragma endregion

#pragma endregion

String html_processor(const String &var) {
    // Serial.println(var);
    if (var == "BAT_RAW") {
        return String(analogRead(BAT_SENSE_PIN));
    }
    if (var == "BAT_CALC") {
        return String(battery_percent_smooth);
    }
    if (var == "EXT_POWER") {
        return String(has_external_power);
    }
    if (var == "CHG_ENABLED") {
        return String(tp4056_enabled);
    }
    if (var == "TEMP") {
        return String(get_temp());
    }
    return String();
}

void init_temp_sensor() {
    temp_sensor_config_t temp_sensor = TSENS_CONFIG_DEFAULT();
    temp_sensor.dac_offset = TSENS_DAC_L2; // TSENS_DAC_L2 is default; L4(-40°C ~ 20°C), L2(-10°C ~ 80°C), L1(20°C ~ 100°C), L0(50°C ~ 125°C)
    temp_sensor_set_config(temp_sensor);
    temp_sensor_start();
}

float get_temp() {
    float temp = 0;
    temp_sensor_read_celsius(&temp);
    return temp;
}

void beep(int duration_ms, int delay_ms) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(duration_ms);
    digitalWrite(BUZZER_PIN, LOW);
    delay(delay_ms);
}

void fill(CRGB col, bool show) {
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = col;
    }
    if (show) {
        FastLED.show();
    }
}

BreathingEffect breathing_effect;

void setup() {
    pinMode(BAT_SENSE_PIN, INPUT);

    pinMode(CHARGE_ENABLE_PIN, OUTPUT);
    pinMode(WHITE_PWM_PIN, OUTPUT);
    pinMode(WS2815_PIN, OUTPUT);
    pinMode(TWELVE_VOLT_OUT_ENABLED_PIN, OUTPUT);
    pinMode(CHARGE_SENSE_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    pinMode(SHAKER_PIN, INPUT_PULLUP);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(BUTTON2_PIN, INPUT_PULLUP);

    delay(5000); // Sanity check, startup delay

    // Boot chime
    beep(40, 500);
    beep(40, 100);
    beep(40, 100);
    beep(40, 100);
    beep(40, 100);
    beep(200, 40);

    FastLED.addLeds<WS2815, WS2815_PIN, BRG>(leds, NUM_LEDS);
    FastLED.setBrightness(100);
    FastLED.clear();
    FastLED.show();

    analogWrite(WHITE_PWM_PIN, 0);                   // TODO: Handle setting to 0 when no external power, handle full led strip shutdown by setting control pin to INPUT
    digitalWrite(TWELVE_VOLT_OUT_ENABLED_PIN, HIGH); // Enable initially to make sure that strip has power

    WiFi.begin(ssid, password);
    breathing_effect.reset();
    while (WiFi.status() != WL_CONNECTED) {
        breathing_effect.iteration();
        delay(RENDER_LOOP_DELTA / 3);
    }

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html, html_processor);
    });

    server.begin();

    fill(CRGB::Green, true);
    delay(1500);
    FastLED.clear();

    init_temp_sensor();
}

int to_valid_led_index(int index) {
    return constrain(index, 0, NUM_LEDS);
}

void grad_sweep_anim(bool up = false) {
    FastLED.clear();

    int start = up ? NUM_LEDS - 1 : 0;
    int end = up ? -1 : NUM_LEDS;

    int step = up ? -1 : 1;

    for (int gradient_center = start; gradient_center != end; gradient_center += step) {
        int gradient_start = to_valid_led_index(gradient_center - 3);
        int gradient_end = to_valid_led_index(gradient_center + 3);
        int gradient_length = gradient_end - gradient_start;
        fill(CRGB::Black, false);
        for (int i = gradient_start; i < gradient_end; i++) {
            leds[i] = blend(CRGB::Red, CRGB::Black, abs(i - gradient_center) / 3.0);
        }
        FastLED.show();
        delay(RENDER_LOOP_DELTA * 2); // Take it slow
    }
}

void handle_power_switching() {
    int charger_connected = digitalRead(CHARGE_SENSE_PIN);
    if (charger_connected != has_external_power) {
        if (charger_connected) {
            beep(100, 10);
            beep(100, 10);
            beep(100, 1000);
            digitalWrite(TWELVE_VOLT_OUT_ENABLED_PIN, LOW);
            grad_sweep_anim(true);
        } else {
            beep(150, 10);
            beep(150, 1000);
            digitalWrite(TWELVE_VOLT_OUT_ENABLED_PIN, HIGH);
            grad_sweep_anim(false);
        }
        has_external_power = charger_connected;
    }
}

void handle_charging() {
    int battery_analog_raw = analogRead(BAT_SENSE_PIN);
    float battery_percent_measured = constrain(battery_analog_raw - BAT_0_PERCENT, 0.0, BAT_100_PERCENT_NORM) / BAT_100_PERCENT_NORM * 100;
    if (battery_percent_smooth < 0) {
        battery_percent_smooth = battery_percent_measured; // First reading after startup
    } else {
        battery_percent_smooth = battery_percent_measured * BATTERY_SMOOTHING_MULT + battery_percent_smooth * (1 - BATTERY_SMOOTHING_MULT);
    }

    if (!has_external_power) {
        digitalWrite(CHARGE_ENABLE_PIN, LOW);
        tp4056_enabled = false;

        if (battery_percent_smooth < 15) {
            // TODO: WARN
        }

        return;
    }

    if (battery_percent_smooth < 90) {
        if (!tp4056_enabled) {
            digitalWrite(CHARGE_ENABLE_PIN, HIGH);
            beep(10, 10);
            beep(10, 10);
            beep(10, 10);
            tp4056_enabled = true;
        }
    } else {
        digitalWrite(CHARGE_ENABLE_PIN, LOW);
        tp4056_enabled = false;
    }
}

void detect_shaking() {
    int shaker_enabled = digitalRead(SHAKER_PIN);
    if (shaker_enabled != shake_sensor_prev) {
        shake_sensor_prev = shaker_enabled;
    }
}

void loop() {
    FastLED.setBrightness(battery_percent_smooth / 100.0 * 230 + 10);

    handle_power_switching();
    handle_charging();
    detect_shaking();

    if (has_external_power) {
        fill(CRGB::Green, true);
    } else {
        fill(CRGB::Red, true);
    }

    delay(CONTROL_LOOP_DELTA);
}