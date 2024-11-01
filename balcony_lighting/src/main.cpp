#include <Arduino.h>
#include <IRremote.hpp>

#define IR_RECEIVE_PIN 7  // ИК пин
#define STRIP_PIN 9       // пин ленты
#define STRIP_POWER_PIN 5 // пин питания ленты
#define MIC_PIN A0
#define NUMLEDS 186 // кол-во светодиодов

#include <microLED.h>
microLED<NUMLEDS, STRIP_PIN, MLED_NO_CLOCK, LED_WS2815, ORDER_GRB, CLI_AVER> strip;
#include <FastLEDsupport.h> // нужна для шума

#define BTN_POWER 23
#define BTN_MUTE 14
#define BTN_FAV 14

#define BTN_PMODE 31
#define BTN_SMODE 69
#define BTN_SLEEP 70
#define BTN_SCALER 22

#define BTN_0 0
#define BTN_1 1
#define BTN_2 2
#define BTN_3 3
#define BTN_4 4
#define BTN_5 5
#define BTN_6 6
#define BTN_7 7
#define BTN_8 8
#define BTN_9 9

#define BTN_M_S_MM 15
#define BTN_RECALL 11
#define BTN_SETUP 21
#define BTN_EXIT 16
#define BTN_CALL 10
#define BTN_SOURCE 17

#define BTN_LEFT 77
#define BTN_RIGHT 78
#define BTN_OK 79
#define BTN_UP 75
#define BTN_DOWN 76

#define BTN_RED 73
#define BTN_GREEN 13
#define BTN_YELLOW 64
#define BTN_BLUE 74

#define BTN_CH_PLUS 24
#define BTN_CH_MINUS 25
#define BTN_VOL_PLUS 27
#define BTN_VOL_MINUS 26

#define BTN_TEXT 28
#define BTN_MIX 71
#define BTN_INDEX 66
#define BTN_III 20
#define BTN_CANCEL 72
#define BTN_REVEAL 65
#define BTN_HOLD 30
#define BTN_SIZE 12

#define BTN_SPAGE 29
#define BTN_PAGE_PLUS 67
#define BTN_PAGE_MINUS 68
#define BTN_BLANK 80
#define BTN_INFO 81
#define BTN_RADIO 82
#define BTN_EPG 83
#define BTN_STITLE 84

enum Effect {
    FIRST = -1,
    STATIC = 0,
    FIRE = 1,
    GRADIENT = 2,
    LAST = 3,
};

mData static_color;

int fire_cooling = 55;
int fire_sparking = 120;

mData grad_color1 = mRed;
mData grad_color2 = mOrange;

int brightness = 210;
int speed_delay = 15;
bool powered = false;
Effect mode = Effect::STATIC;

mGradient<2> red_grad;
long loop_count = 0;

#pragma region EFFECTS ----------------------------------------------------------------

void setPixelHeatColor(int pixel, byte temperature) {
    // Scale 'heat' down from 0-255 to 0-191
    byte t192 = round((temperature / 255.0) * 191);

    // calculate ramp up from
    byte heatramp = t192 & 0x3F; // 0..63
    heatramp <<= 2;              // scale up to 0..252

    // figure out which third of the spectrum we're in:
    if (t192 > 0x80) { // hottest
        strip.leds[pixel] = mRGB(255, 255, heatramp);
    } else if (t192 > 0x40) { // middle
        strip.leds[pixel] = mRGB(255, heatramp, 0);
    } else { // coolest
        strip.leds[pixel] = mRGB(heatramp, 0, 0);
    }
}

void fire(int cooling, int sparking) {
    static byte heat[NUMLEDS];
    int cooldown;

    // Step 1.  Cool down every cell a little
    for (int i = 0; i < NUMLEDS; i++) {
        cooldown = random(0, ((cooling * 10) / NUMLEDS) + 2);

        if (cooldown > heat[i]) {
            heat[i] = 0;
        } else {
            heat[i] = heat[i] - cooldown;
        }
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (int k = NUMLEDS - 1; k >= 2; k--) {
        heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks' near the bottom
    if (random(255) < sparking) {
        int y = random(70);
        heat[y] = heat[y] + random(160, 255);
        // heat[y] = random(160,255);
    }

    // Step 4.  Convert heat to LED colors
    for (int j = 0; j < NUMLEDS; j++) {
        setPixelHeatColor(j, heat[j]);
    }
}

#pragma endregion EFFECTS-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

void fillWithColor(mData col) {
    for (int i = 0; i < NUMLEDS; i++) {
        strip.leds[i] = col;
    }
    strip.show();
}

float fract(float x) { return x - int(x); }

float mix(float a, float b, float t) { return a + (b - a) * t; }

float step(float e, float x) { return x < e ? 0.0 : 1.0; }

mData hsvOffset(mData col, float h_offset, float s_offset, float v_offset) {

    float r = col.r / 255.0;
    float g = col.g / 255.0;
    float b = col.b / 255.0;

    float s = step(b, g);
    float px = mix(b, g, s);
    float py = mix(g, b, s);
    float pz = mix(-1.0, 0.0, s);
    float pw = mix(0.6666666, -0.3333333, s);
    s = step(px, r);
    float qx = mix(px, r, s);
    float qz = mix(pw, pz, s);
    float qw = mix(r, px, s);
    float d = qx - min(qw, py);

    float h_ = abs(qz + (qw - py) / (6.0 * d + 1e-10)) + h_offset;
    float s_ = d / (qx + 1e-10) + s_offset;
    float v_ = qx + v_offset;

    col.r = constrain(v_ * mix(1.0, constrain(abs(fract(h_ + 1.0) * 6.0 - 3.0) - 1.0, 0.0, 1.0), s_) * 255, 0, 255);
    col.g = constrain(v_ * mix(1.0, constrain(abs(fract(h_ + 0.6666666) * 6.0 - 3.0) - 1.0, 0.0, 1.0), s_) * 255, 0, 255);
    col.b = constrain(v_ * mix(1.0, constrain(abs(fract(h_ + 0.3333333) * 6.0 - 3.0) - 1.0, 0.0, 1.0), s_) * 255, 0, 255);

    return col;
}

void play_on_anim() {
    mData color = mRed;

    fillWithColor(mBlack);
    strip.setBrightness(0);

    int half_point = NUMLEDS / 2;
    for (int i = 0; i < half_point; i++) {
        int right_target_start = half_point + i;
        int left_target_start = half_point - i - 1;

        int left_target_end = constrain(left_target_start - 15, 0, NUMLEDS - 1);
        int right_target_end = constrain(right_target_start + 15, 0, NUMLEDS - 1);

        for (int fill = left_target_start + 1; i < right_target_start; i++) {
            strip.leds[fill] = color;
        }

        for (int grad = left_target_end; grad <= left_target_start; grad++) {
            float value = (grad - left_target_end) / 15.0f;
            strip.leds[grad] = getFade(color, value) hsvOffset(color, 0, value / 2.0f, -value);
        }

        for (int grad = right_target_start; grad <= right_target_end; grad++) {
            float value = (grad - right_target_start) / 15.0f;
            strip.leds[grad] = hsvOffset(color, 0, value / 2.0f, value - 1);
        }

        // strip.setBrightness(((i + 1) / (float)half_point) * brightness);

        delay(100);
        strip.show();
        fillWithColor(mPurple);
        delay(500);
        fillWithColor(mBlue);
    }

    strip.setBrightness(brightness);

    for (int i = 0; i <= 500; i++) {
        float inverted = 500.0f - i;
        fillWithColor(getFade(color, i / 500.0f * 255));
        delay(inverted * inverted / 10000.0f + 10);
    }

    fillWithColor(mBlack);
}

void play_off_anim() {
    for (int i = 0; i <= brightness; i++) {
        strip.setBrightness(brightness - i);
        delay(15);
        strip.show();
    }
    fillWithColor(mBlack);
    strip.setBrightness(0);
}

void turn_off() {
    powered = false;
    play_off_anim();
    delay(1000);
    // Disable signal pin for extra protection against leakage current
    pinMode(STRIP_PIN, INPUT);
    // Cut power to led strip completely
    digitalWrite(STRIP_POWER_PIN, LOW);
}

void turn_on() {
    // Give power to LED strip
    digitalWrite(STRIP_POWER_PIN, HIGH);
    // Enable command pin
    pinMode(STRIP_PIN, OUTPUT);
    delay(1000);
    play_on_anim();
    powered = true;
}

void setup() {
    // Serial.begin(112500);

    red_grad.colors[0] = grad_color1;
    red_grad.colors[1] = grad_color2;

    static_color = mBlue;

    pinMode(MIC_PIN, INPUT);
    pinMode(STRIP_POWER_PIN, OUTPUT);
    pinMode(STRIP_PIN, INPUT); // Off at start
    pinMode(IR_RECEIVE_PIN, OUTPUT);

    IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);

    delay(5000);
    turn_on();
}

void display_effect() {
    switch (mode) {
    case Effect::STATIC:
        fillWithColor(static_color);
        break;
    case Effect::FIRE:
        fire(fire_cooling, fire_sparking);
        break;
    case Effect::GRADIENT:
        for (int i = 0; i < NUMLEDS; i++) {
            strip.leds[i] = red_grad.get(inoise8(i, loop_count), 255);
        }
        break;
    default:
        break;
    }
    strip.show();
    delay(speed_delay);
    loop_count++;
}

bool is_repeat_flag() {
    return IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT;
}

void handle_control_events() {
    if (IrReceiver.decode()) {
        IrReceiver.resume();
        switch (IrReceiver.decodedIRData.command) {
        case BTN_MUTE:
            if (is_repeat_flag()) {
                // ignore if repeat flag is set
                break;
            }
            fillWithColor(mPurple);
            if (powered) {
                turn_off();
            } else {
                turn_on();
            }
            // Debounce
            delay(500);
            break;

        case BTN_UP:
            brightness = constrain(brightness + 5, 0, 230);
            strip.setBrightness(brightness);
            break;

        case BTN_DOWN:
            brightness = constrain(brightness - 5, 0, 230);
            strip.setBrightness(brightness);
            break;

        case BTN_RIGHT:
            if (is_repeat_flag()) {
                // ignore if repeat flag is set
                break;
            }
            mode = static_cast<Effect>(constrain(static_cast<int>(mode) + 1, Effect::FIRST, Effect::LAST));
            if (mode == Effect::LAST) {
                mode = static_cast<Effect>(static_cast<int>(Effect::FIRST) + 1);
            }
            // Debounce
            delay(500);
            break;

        case BTN_LEFT:
            if (is_repeat_flag()) {
                // ignore if repeat flag is set
                break;
            }
            mode = static_cast<Effect>(constrain(static_cast<int>(mode) - 1, Effect::FIRST, Effect::LAST));
            if (mode == Effect::FIRST) {
                mode = static_cast<Effect>(static_cast<int>(Effect::LAST) - 1);
            }
            // Debounce
            delay(500);
            break;

        case BTN_PMODE:
            switch (mode) {

            case Effect::STATIC:
                hsvOffset(static_color, -0.007, 0, 0);
                break;

            case Effect::GRADIENT:
                hsvOffset(grad_color1, -0.007, 0, 0);
                break;

            case Effect::FIRE:
                fire_sparking = constrain(fire_sparking - 1, 0, 255);
                // Debounce
                delay(150);
                break;

            default:
                break;
            }

        case BTN_SMODE:
            switch (mode) {

            case Effect::STATIC:
                static_color = hsvOffset(static_color, 0.007, 0, 0);
                break;

            case Effect::GRADIENT:
                grad_color1 = hsvOffset(grad_color1, 0.007, 0, 0);
                break;

            case Effect::FIRE:
                fire_sparking = constrain(fire_sparking + 1, 0, 255);
                // Debounce
                delay(150);
                break;

            default:
                break;
            }

        case BTN_SLEEP:
            switch (mode) {

            case Effect::STATIC:
                static_color = hsvOffset(static_color, 0, -0.01, 0);
                break;

            case Effect::GRADIENT:
                grad_color2 = hsvOffset(grad_color2, -0.007, 0, 0);
                break;

            case Effect::FIRE:
                fire_cooling = constrain(fire_cooling - 1, 0, 255);
                // Debounce
                delay(150);
                break;

            default:
                break;
            }

        case BTN_SCALER:
            switch (mode) {

            case Effect::STATIC:
                static_color = hsvOffset(static_color, 0, 0.01, 0);
                break;

            case Effect::GRADIENT:
                grad_color2 = hsvOffset(grad_color2, 0.007, 0, 0);
                break;

            case Effect::FIRE:
                fire_cooling = constrain(fire_cooling + 1, 0, 255);
                // Debounce
                delay(150);
                break;

            default:
                break;
            }

        default:
            break;
        }
    }
}

void loop() {

    while (true) {
        handle_control_events();
        if (powered) {
            display_effect();
        }
    }

    // float average = 0;
    // float noise_floor = 0;

    // for (int i = 0; i < 10000; i++) {
    //     noise_floor = noise_floor * 0.7 + analogRead(A0) * 0.3;
    // }

    // Serial.print("noise_floor is");
    // Serial.println(noise_floor);

    // // while (true) {
    // //     digitalWrite(5, HIGH);
    // //     delay(3000);
    // //     digitalWrite(5, LOW);
    // //     delay(3000);
    // // }

    // while (true) {
    //     float min_ = 1000000;
    //     float max_ = -1000000;
    //     for (int i = 0; i < 100; i++) {
    //         int value = analogRead(A0);
    //         min_ = min(min_, value - noise_floor);
    //         max_ = max(max_, value - noise_floor);
    //     }
    //     average = average * 0.5 + (max_ - min_) * 0.5;
    //     if(average > 65) {
    //       fillWithColor(mWhite);
    //     } else {
    //       fillWithColor(mOrange);
    //     }
    // }
}