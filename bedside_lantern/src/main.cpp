#include <Arduino.h> 

#include <FastLED.h>

#include <ESPAsyncWebServer.h>
#include <WiFi.h>

#include "driver/temp_sensor.h"

#define EB_DEB_TIME 10    // таймаут гашения дребезга кнопки (кнопка)
#define EB_CLICK_TIME 500 // таймаут ожидания кликов (кнопка)
#define EB_HOLD_TIME 600  // таймаут удержания (кнопка)
#define EB_STEP_TIME 200  // таймаут импульсного удержания (кнопка)
#define EB_FAST_TIME 30   // таймаут быстрого поворота (энкодер)
#include <EncButton.h>

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
            <span>Current brightness:</span>
            <span id="brightness">%BRIGHT%</span>
        </p>
        <p>
            <span>Current brightness (white):</span>
            <span id="brightness_w">%BRIGHT_W%</span>
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

#define RENDER_LOOP_DELTA_MS 30 // 33 Fps

#define ADC_RESOLUTION 4096
#define BAT_0_PERCENT (0.667 * ADC_RESOLUTION)
#define BAT_100_PERCENT (0.998 * ADC_RESOLUTION)
#define BAT_100_PERCENT_NORM (0.998 * ADC_RESOLUTION - BAT_0_PERCENT)

#define WIFI_CONNECTION_TIMEOUT_MS 30'000
bool wifi_connection_timeout_reached = false;
bool webserver_created = false;
bool webserver_active = false;

float battery_percent_smooth = -1;
int tp4056_enabled = 2; // 2 = unknown state

int has_external_power = 2; // 2 = unknown state
int shake_sensor_prev = 2;

bool ws_enabled = false;
bool white_enabled = false;

int target_brightness = 100;
float current_brightness = 0;

float target_white_brightness = 0;
float current_white_brightness = 0;

int control_loop_delta_ms = 10; // 100 times per second by default

#pragma region UTILS

// A - current value
// B - target value
// dt - delta time in seconds
// h - half-life (time until half-way, in seconds)
// https://mastodon.social/@acegikmo/111931613710775864
// https://www.youtube.com/watch?v=LSNQuFEDOyQ
float lerp_smooth(float a, float b, float dt, float h) {
    return b + (a - b) * exp2f(-dt / h);
}

#pragma endregion

#pragma region EFFECTS

class BreathingEffect {
    // https://github.com/marmilicious/FastLED_examples/blob/master/breath_effect_v2.ino
  private:
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

    void iteration(float pulseSpeed) {
        float dV = (exp(sin(pulseSpeed * millis() / 2000.0 * PI)) - 0.36787944) * delta;
        val = valueMin + dV;
        hue = map(val, valueMin, valueMax, hueA, hueB); // Map hue based on current val
        sat = map(val, valueMin, valueMax, satA, satB); // Map sat based on current val

        for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = CHSV(hue, sat, val);

            // You can experiment with commenting out these dim8_video lines
            // to get a different sort of look.
            leds[i].r = dim8_video(leds[i].r);
            leds[i].g = dim8_video(leds[i].g);
            leds[i].b = dim8_video(leds[i].b);
        }

        FastLED.show();
    }
};

void fill(CRGB col, bool show) {
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = col;
    }
    if (show) {
        FastLED.show();
    }
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
        delay(RENDER_LOOP_DELTA_MS * 2); // Take it slow
    }
}

BreathingEffect breathing_effect;

#pragma endregion

#pragma region temp n stuff

float get_internal_temp() {
    float temp = 0;
    temp_sensor_read_celsius(&temp);
    return temp;
}

void init_temp_sensor() {
    temp_sensor_config_t temp_sensor = TSENS_CONFIG_DEFAULT();
    temp_sensor.dac_offset = TSENS_DAC_L2; // TSENS_DAC_L2 is default; L4(-40°C ~ 20°C), L2(-10°C ~ 80°C), L1(20°C ~ 100°C), L0(50°C ~ 125°C)
    temp_sensor_set_config(temp_sensor);
    temp_sensor_start();
}

#pragma endregion

#pragma region WEB

const char *home_ssid = "BIMBA";
const char *home_password = "reeeeeee";

const char *work_ssid = "BS2Lab_2G";
const char *work_password = "gTMr48s.hSt8Z5CsRP";

AsyncWebServer server(80);

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
        return String(get_internal_temp());
    }
    if (var == "BRIGHT") {
        return String(current_brightness) + "(target: " + String(target_brightness) + ")";
    }
    if (var == "BRIGHT_W") {
        return String(current_white_brightness) + "(target: " + String(target_white_brightness) + ")";
    }
    return String();
}

bool try_connect_to_wifi(const char *__ssid, const char *__passphrase) {

    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin(__ssid, __passphrase);

    int start_time = millis();
    while (WiFi.status() != WL_CONNECTED) {
        breathing_effect.iteration(1.5);
        if (millis() - start_time > WIFI_CONNECTION_TIMEOUT_MS) {
            return false;
        }
    }

    return true;
}

void handle_wifi_and_web() {

    if (WiFi.status() == WL_CONNECTED) {
        return;
    } else if (webserver_active) {
        webserver_active = false;
        server.end();
    }

    if (wifi_connection_timeout_reached) {
        return;
    }

    if (try_connect_to_wifi(home_ssid, home_password) || try_connect_to_wifi(work_ssid, work_password)) {

        if (!webserver_created) {
            server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
                request->send_P(200, "text/html", index_html, html_processor);
            });
            webserver_created = true;
        }

        if (!webserver_active) {
            server.begin(); // Should never be active here, but check just in case
            webserver_active = true;
        }

        fill(CRGB::Green, true);
        delay(1500);
        fill(CRGB::Black, true);
        delay(1500);

    } else {
        wifi_connection_timeout_reached = true;

        fill(CRGB::Red, true);
        delay(1500);
        fill(CRGB::Black, true);
        delay(1500);

        if (webserver_active) {
            webserver_active = false;
            server.end();
        }

        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
    }
}

#pragma endregion

#pragma region SOUND

enum BeepType {
    BOOT,
    LOW_BATTERY,
    START_CHARGING,
    STOP_CHARGING,
    CHARGER_CONNECTED,
    CHARGER_DISCONNECTED,
    WIFI_CONNECTED,
    WIFI_DISCONNECTED
};

void beep(int duration_ms, int delay_ms) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(duration_ms);
    digitalWrite(BUZZER_PIN, LOW);
    delay(delay_ms);
}

void beep_by_type(BeepType type) {
    switch (type) {
    case BeepType::LOW_BATTERY:
        beep(1000, 1000);
        beep(1000, 1000);
        beep(1000, 0);
        break;

    case BeepType::BOOT:
        beep(40, 500);
        beep(40, 500);
        beep(40, 100);
        beep(40, 100);
        beep(40, 100);
        beep(40, 100);
        beep(200, 0);
        break;

    case BeepType::START_CHARGING:
        beep(10, 10);
        beep(10, 10);
        beep(10, 0);
        break;

    case BeepType::STOP_CHARGING:
        beep(10, 30);
        beep(10, 30);
        beep(10, 0);
        break;

    case BeepType::CHARGER_CONNECTED:
        beep(100, 10);
        beep(100, 10);
        beep(100, 0);
        break;

    case BeepType::CHARGER_DISCONNECTED:
        beep(100, 30);
        beep(100, 30);
        beep(100, 0);
        break;

    case BeepType::WIFI_CONNECTED:
        beep(300, 10);
        beep(300, 10);
        beep(300, 0);
        break;

    case BeepType::WIFI_DISCONNECTED:
        beep(300, 30);
        beep(300, 30);
        beep(300, 0);
        break;

    default:
        beep(1000, 0);
        break;
    }
}

#pragma endregion

#pragma region Hardware control

#pragma region Strip brightness

void set_brightness(int target) {
    target_brightness = constrain(target, 0, 255);
}

void set_white_brightness(int target) {
    target_white_brightness = constrain(target, 0, 255);
}

void update_brightness(float dt) {

    if (current_brightness != target_brightness) {
        current_brightness = lerp_smooth(current_brightness, target_brightness, dt, 0.7);
    }

    if (current_white_brightness != target_white_brightness) {
        current_white_brightness = lerp_smooth(current_white_brightness, target_white_brightness, dt, 0.7);
    }

    if (current_brightness > 0) {
        if (!ws_enabled) {
            ws_enabled = true;
        }
        if (current_brightness != target_brightness) {
            FastLED.setBrightness(current_brightness);
            FastLED.show();
        }
    } else {
        if (ws_enabled) {
            ws_enabled = false;
            current_brightness = 0;
            FastLED.setBrightness(0);
            FastLED.show();
        }
    }

    if (current_white_brightness > 0 && has_external_power) {
        if (!white_enabled) {
            white_enabled = true;
        }
        if (current_white_brightness != target_white_brightness) {
            ledcWrite(WHITE_PWM_PIN, current_white_brightness / 4096.0 * 1024);
        }
    } else {
        if (white_enabled) {
            white_enabled = false;
            current_white_brightness = 0;
            ledcWrite(WHITE_PWM_PIN, 0);
        }
    }
}

#pragma endregion

void handle_power_switching() {
    int charger_connected = digitalRead(CHARGE_SENSE_PIN);
    if (charger_connected != has_external_power) {
        if (charger_connected) {
            beep_by_type(BeepType::CHARGER_CONNECTED);
            digitalWrite(TWELVE_VOLT_OUT_ENABLED_PIN, LOW);
            grad_sweep_anim(true);
        } else {
            beep_by_type(BeepType::CHARGER_DISCONNECTED);
            digitalWrite(TWELVE_VOLT_OUT_ENABLED_PIN, HIGH);
            grad_sweep_anim(false);
        }
        has_external_power = charger_connected;
    }
}

void handle_charging(float dt) {
    int battery_analog_raw = analogRead(BAT_SENSE_PIN);
    float battery_percent_measured = constrain(battery_analog_raw - BAT_0_PERCENT, 0.0, BAT_100_PERCENT_NORM) / BAT_100_PERCENT_NORM * 100;
    if (battery_percent_smooth < 0) {
        battery_percent_smooth = battery_percent_measured; // First reading after startup
    } else {
        battery_percent_smooth = lerp_smooth(battery_percent_smooth, battery_percent_measured, dt, 20);
    }

    if (!has_external_power) {
        if (tp4056_enabled) {
            digitalWrite(CHARGE_ENABLE_PIN, LOW);
            tp4056_enabled = false;
            beep_by_type(BeepType::STOP_CHARGING);
        }

        if (battery_percent_smooth < 15) {
            beep_by_type(BeepType::LOW_BATTERY);
        }

        return;
    }

    if (!tp4056_enabled) {
        digitalWrite(CHARGE_ENABLE_PIN, HIGH);
        beep_by_type(BeepType::START_CHARGING);
        tp4056_enabled = true;
    }

    // if (battery_percent_smooth < 90) {
    //     if (!tp4056_enabled) {
    //         digitalWrite(CHARGE_ENABLE_PIN, HIGH);
    //         beep_by_type(BeepType::START_CHARGING);
    //         tp4056_enabled = true;
    //     }
    // } else {
    //     digitalWrite(CHARGE_ENABLE_PIN, LOW);
    //     tp4056_enabled = false;
    // }
}

#pragma region Buttons and shaker

ButtonT<BUTTON_PIN> bottom_button;
ButtonT<BUTTON2_PIN> pin_button;

void detect_shaking() {
    int shaker_enabled = digitalRead(SHAKER_PIN);
    if (shaker_enabled != shake_sensor_prev) {
        shake_sensor_prev = shaker_enabled;
    }
}

void handle_button() {
    pin_button.tick();
    bottom_button.tick();

    if (pin_button.click() || bottom_button.click()) {
        beep(30, 0);
        if (target_brightness > 0) {
            set_brightness(0);
            set_white_brightness(0);
        } else {
            set_brightness(255);
            set_white_brightness(255);
        }
        fill(CRGB::White, true);
    }
}

#pragma endregion

#pragma endregion

#pragma region INIT

void init_gpio() {
    pinMode(BAT_SENSE_PIN, INPUT);

    pinMode(CHARGE_ENABLE_PIN, OUTPUT);

    pinMode(WHITE_PWM_PIN, OUTPUT);
    ledcSetup(WHITE_PWM_PIN, 15000, 12);

    pinMode(WS2815_PIN, OUTPUT);
    pinMode(TWELVE_VOLT_OUT_ENABLED_PIN, OUTPUT);
    pinMode(CHARGE_SENSE_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    pinMode(SHAKER_PIN, INPUT_PULLUP);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(BUTTON2_PIN, INPUT_PULLUP);

    
}

void init_leds() {
    FastLED.addLeds<WS2815, WS2815_PIN, BRG>(leds, NUM_LEDS);
    current_brightness = target_brightness;
    FastLED.clear();
}

void setup() {

    init_gpio();

    delay(5000); // Sanity check, startup delay

    // Boot chime
    beep_by_type(BeepType::BOOT);

    init_leds();
    init_temp_sensor();
}

#pragma endregion

int prev_millis = millis();

void loop() {
    int ms = millis();
    float delta = (ms - prev_millis) / 1000.0;

    handle_power_switching();
    handle_charging(delta);

    update_brightness(delta);

    handle_wifi_and_web();

    handle_button();
    detect_shaking();

    // delay(control_loop_delta_ms);
    prev_millis = ms;
}