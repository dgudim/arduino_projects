#include <Arduino.h>

#define GH_INCLUDE_PORTAL
#define GH_NO_MQTT
#include <GyverHub.h>

#define DEV_NAME "Horny-v3"
#define DEV_NAME_NET "Horny-network"

GyverHub hub(DEV_NAME_NET, DEV_NAME, "f6ad"); // имя сети, имя устройства, иконка // f0eb

#define HORN_PIN_LEFT D6
#define HORN_PIN_RIGHT D7

#define MAX_BRIGHTNESS_SCALE 100.0F
#define DAC_RESOLUTION 10
const float DAC_STEPS = pow(2, DAC_RESOLUTION) - 1.0;

static float time_passed = 0;

static byte mode = 0;
static byte prev_mode = 0;

static float max_brightness = 0;

static float current_brightness_left = 0;
static float current_brightness_right = 0;

static float target_brightness_left = 0;
static float target_brightness_right = 0;

static float breathing_speed_mult = 1;
static float disco_speed = 1;

void build(gh::Builder &b) {

    b.Slider(&max_brightness).range(0, MAX_BRIGHTNESS_SCALE, 1).label("Brightness");
    {
        gh::Row r(b);
        b.Gauge(&current_brightness_left).range(0, MAX_BRIGHTNESS_SCALE, 1).unit("%").label("Current brightness left");
        b.Gauge(&current_brightness_right).range(0, MAX_BRIGHTNESS_SCALE, 1).unit("%").label("Current brightness right");
    }

    if (b.Select(&mode).text("Static;Breathing;Breathing round;Disco;Mixed;Flash").label("Modes").click()) {
        b.refresh();
    }

    if (mode != prev_mode) {
        // Reset brightness of mode change
        current_brightness_left = 0;
        current_brightness_right = 0;
        prev_mode = mode;
    }

    switch (mode) {
    case 1:
    case 2:
        b.Slider(&breathing_speed_mult).unit("X").range(0.1, 10, 0.1).label("Speed");
        break;
    case 3:
        b.Slider().unit("X").range(0.1, 10, 0.1).label("Rave speed");
        break;
    case 4:
        b.Slider().unit("Sec").range(5, 120, 5).label("Switch time");
        break;
    case 5:
        b.Slider().unit("Sec").range(1, 10, 1).label("Flash duration");
        b.Slider().unit("Sec").range(1, 20, 1).label("Flash spacing");
        b.Switch().label("Randomize");
        break;
    default:
        break;
    }
}

void setup() {
    pinMode(HORN_PIN_LEFT, OUTPUT);
    pinMode(HORN_PIN_RIGHT, OUTPUT);

    analogWriteResolution(DAC_RESOLUTION);
    analogWriteFreq(200);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(DEV_NAME);

    hub.setPIN(1133);
    hub.setVersion("0.1.0-nice");
    hub.onBuild(build);
    hub.begin();

    delay(3000);
}

// A - current value
// B - target value
// dt - delta time in seconds
// h - half-life (time until half-way, in seconds)
// https://mastodon.social/@acegikmo/111931613710775864
// https://www.youtube.com/watch?v=LSNQuFEDOyQ
float lerp_smooth(float a, float b, float dt, float h) {
    return b + (a - b) * exp2f(-dt / h);
}

void tick_effects(float dt) {

    if (mode == 0) {
        current_brightness_left = MAX_BRIGHTNESS_SCALE; // Scaled by max brightness
        current_brightness_right = MAX_BRIGHTNESS_SCALE;
    }

    if (mode == 1 || mode == 2) {
        current_brightness_left = lerp_smooth(current_brightness_left, target_brightness_left, dt, 1.5 / breathing_speed_mult);
        if (current_brightness_left >= MAX_BRIGHTNESS_SCALE - 10) {
            target_brightness_left = 0;
        }
        if (current_brightness_left <= 10) {
            target_brightness_left = MAX_BRIGHTNESS_SCALE;
        }
    }

    switch (mode) {
        // Breathing
    case 1:
        current_brightness_right = current_brightness_left;
        break;
    case 2:
        current_brightness_right = MAX_BRIGHTNESS_SCALE - current_brightness_left;
        break;

    default:
        break;
    }

    analogWrite(HORN_PIN_LEFT, current_brightness_left * (max_brightness / MAX_BRIGHTNESS_SCALE) / MAX_BRIGHTNESS_SCALE * DAC_STEPS);
    analogWrite(HORN_PIN_RIGHT, current_brightness_right * (max_brightness / MAX_BRIGHTNESS_SCALE) / MAX_BRIGHTNESS_SCALE * DAC_STEPS);
}

int prev_millis = millis();

void loop() {
    int ms = millis();
    float delta = (ms - prev_millis) / 1000.0;
    hub.tick();
    tick_effects(delta);
    prev_millis = ms;
    time_passed += delta;
    // if ((int)time_passed % 3 == 0) {
    //     hub.update("b_left");
    //     hub.update("b_right");
    //     time_passed += 1; // Arbitrarily leap forward to avoid a lot of updates in that second
    // }
}