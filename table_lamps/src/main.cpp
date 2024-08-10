#include "../.pio/libdeps/nanoatmega328/microLED/src/color_utility.h"
#include <IRremote.hpp>

#define IR_RECEIVE_PIN 2 // ИК пин
#define STRIP_PIN 5      // пин ленты
#define LAMP_PIN 8
#define NUMLEDS 50 // кол-во светодиодов

#define ANIMATION_SPEED_CONTROL_MULTIPLIER 5

#include <microLED.h>
microLED<NUMLEDS, STRIP_PIN, MLED_NO_CLOCK, LED_WS2812, ORDER_GRB, CLI_AVER> strip;
#include <FastLEDsupport.h> // нужна для шума

#include <EEPROM.h>

class dynGradient {

#define MAX_SIZE 10

  public:
    int actualSize = 0;

    mData colors[MAX_SIZE];

    dynGradient() {
    }

    mData get(int x, int amount) {
        int sectorSize = (amount + actualSize - 2) / (actualSize - 1); // (x+y-1)/y
        int sector = x / sectorSize;
        return getBlend(x - sector * sectorSize, sectorSize, colors[sector], colors[sector + 1]);
    }

    mData getNoise(int offsetX, int offsetY) {
        return get(inoise8(offsetX * 50, offsetY), 255);
    }

    mData getColor(int i) {
        if (i >= actualSize || i < 0) {
            return mBlack;
        }
        return colors[i];
    }

    void addColor(mData col) {
        actualSize++;
        colors[actualSize - 1] = col;
    }

    void addOrSetColor(mData col, int i) {
        if (i >= actualSize) {
            addColor(col);
        } else {
            colors[i] = col;
        }
    }

    void removeColor() {
        actualSize--;
    }

    void clear() {
        actualSize = 0;
    }

    void load_from_eeprom() {
        EEPROM.get(0, actualSize);
        EEPROM.get(sizeof(int), colors);
    }

    void save_to_eeprom() {
        EEPROM.put(0, actualSize);
        EEPROM.put(sizeof(int), colors);
    }
};

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

enum BaseColor {
    R,
    G,
    B
};

enum CurrentGrad {
    RED,
    GREEN,
    YELLOW,
    BLUE,
    CUSTOM
};

float fract(float x) { return x - int(x); }

float mix(float a, float b, float t) { return a + (b - a) * t; }

float step(float e, float x) { return x < e ? 0.0 : 1.0; }

void hsvOffset(mData &col, float h_offset, float s_offset, float v_offset) {

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
}

void fillWithColor(mData col) {
    for (int i = 0; i < NUMLEDS; i++) {
        strip.leds[i] = col;
    }
    strip.show();
}

bool repeat_flag() {
    return IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT;
}

void lamps_on() {
    digitalWrite(LAMP_PIN, HIGH);
}

void lamps_off() {
    digitalWrite(LAMP_PIN, LOW);
}

void setup() {
    pinMode(LAMP_PIN, OUTPUT);
    lamps_off();

    Serial.begin(9600);

    // Speed of the animation
    int count_increment = 5 * ANIMATION_SPEED_CONTROL_MULTIPLIER;

    // For power on/off animation (return to previous brightness on power on)
    int last_saved_target_brightness = 220;

    int current_brightness = 220;
    int target_brightness = 220;

    // State of the lamps
    bool lamps_active = false;

    bool custom_grad_editor_active = false;
    bool custom_grad_color_select_active = false;
    int custom_grad_currently_selected_color_slot = 0;
    mData custom_grad_currently_selected_color = mBlack;
    int custom_grad_currently_selected_color_fraction = BaseColor::R;
    int custom_grad_animation_index = 0;

    dynGradient custom_grad;
    custom_grad.load_from_eeprom();

    CurrentGrad current_gradient = CurrentGrad::BLUE;

    IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);

    mGradient<8> blue_grad;
    blue_grad.colors[0] = mRed;
    blue_grad.colors[1] = mPurple;
    blue_grad.colors[2] = mBlue;
    blue_grad.colors[3] = mBlue;
    blue_grad.colors[4] = mMagenta;
    blue_grad.colors[5] = mRed;
    blue_grad.colors[6] = mYellow;
    blue_grad.colors[7] = mOrange;

    mGradient<6> yellow_grad;
    yellow_grad.colors[0] = mYellow;
    yellow_grad.colors[1] = 0xd98e43;
    yellow_grad.colors[2] = 0xffa200;
    yellow_grad.colors[3] = 0xff8000;
    yellow_grad.colors[4] = 0xc49206;
    yellow_grad.colors[5] = 0xe3a64b;

    mGradient<8> green_grad;
    green_grad.colors[0] = mRed;
    green_grad.colors[1] = 0xffa200;
    green_grad.colors[2] = mLime;
    green_grad.colors[3] = 0x996100;
    green_grad.colors[4] = 0x258a00;
    green_grad.colors[5] = mYellow;
    green_grad.colors[6] = 0xff8000;
    green_grad.colors[7] = mOrange;

    mGradient<8> red_grad;
    red_grad.colors[0] = mRed;
    red_grad.colors[1] = mOrange;
    red_grad.colors[2] = mRed;
    red_grad.colors[3] = mYellow;
    red_grad.colors[4] = mRed;
    red_grad.colors[5] = mYellow;
    red_grad.colors[6] = mMaroon;
    red_grad.colors[7] = mOrange;

    int count = 0;
    for (;;) {

        if (custom_grad_editor_active) {

            if (custom_grad_color_select_active) {
                if (IrReceiver.decode()) {
                    IrReceiver.resume();
                    switch (IrReceiver.decodedIRData.command) {
                    case BTN_0:
                        custom_grad_currently_selected_color = mRed;
                        break;
                    case BTN_1:
                        custom_grad_currently_selected_color = mGreen;
                        break;
                    case BTN_2:
                        custom_grad_currently_selected_color = mLime;
                        break;
                    case BTN_3:
                        custom_grad_currently_selected_color = mPurple;
                        break;
                    case BTN_4:
                        custom_grad_currently_selected_color = mMagenta;
                        break;
                    case BTN_5:
                        custom_grad_currently_selected_color = mOrange;
                        break;
                    case BTN_6:
                        custom_grad_currently_selected_color = mTeal;
                        break;
                    case BTN_7:
                        custom_grad_currently_selected_color = mBlue;
                        break;
                    case BTN_8:
                        custom_grad_currently_selected_color = mNavy;
                        break;
                    case BTN_9:
                        custom_grad_currently_selected_color = mAqua;
                        break;


                    case BTN_UP:
                        switch (custom_grad_currently_selected_color_fraction) {
                        case BaseColor::R:
                            custom_grad_currently_selected_color.r = constrain(custom_grad_currently_selected_color.r + 5, 0, 255);
                            break;
                        case BaseColor::G:
                            custom_grad_currently_selected_color.g = constrain(custom_grad_currently_selected_color.g + 5, 0, 255);
                            break;
                        case BaseColor::B:
                            custom_grad_currently_selected_color.b = constrain(custom_grad_currently_selected_color.b + 5, 0, 255);
                            break;
                        }
                        break;
                    case BTN_DOWN:
                        switch (custom_grad_currently_selected_color_fraction) {
                        case BaseColor::R:
                            custom_grad_currently_selected_color.r = constrain(custom_grad_currently_selected_color.r - 5, 0, 255);
                            break;
                        case BaseColor::G:
                            custom_grad_currently_selected_color.g = constrain(custom_grad_currently_selected_color.g - 5, 0, 255);
                            break;
                        case BaseColor::B:
                            custom_grad_currently_selected_color.b = constrain(custom_grad_currently_selected_color.b - 5, 0, 255);
                            break;
                        }
                        break;


                    case BTN_RIGHT:
                        hsvOffset(custom_grad_currently_selected_color, 0.007, 0, 0);
                        break;
                    case BTN_LEFT:
                        hsvOffset(custom_grad_currently_selected_color, -0.007, 0, 0);
                        break;

                    case BTN_CH_PLUS:
                        hsvOffset(custom_grad_currently_selected_color, 0, 0.01, 0);
                        break;
                    case BTN_CH_MINUS:
                        hsvOffset(custom_grad_currently_selected_color, 0, -0.01, 0);
                        break;

                    case BTN_VOL_PLUS:
                        hsvOffset(custom_grad_currently_selected_color, 0, 0, 0.011);
                        break;
                    case BTN_VOL_MINUS:
                        hsvOffset(custom_grad_currently_selected_color, 0, 0, -0.011);
                        break;


                    case BTN_RED:
                        custom_grad_currently_selected_color_fraction = BaseColor::R;
                        break;
                    case BTN_GREEN:
                        custom_grad_currently_selected_color_fraction = BaseColor::G;
                        break;
                    case BTN_BLUE:
                        custom_grad_currently_selected_color_fraction = BaseColor::B;
                        break;


                    case BTN_SOURCE:
                        custom_grad.addOrSetColor(custom_grad_currently_selected_color, custom_grad_currently_selected_color_slot);
                        custom_grad_color_select_active = false;
                        continue;
                    case BTN_CANCEL:
                        custom_grad_color_select_active = false;
                        continue;
                    }
                }

                fillWithColor(custom_grad_currently_selected_color);

                // Skip main editor loop
                continue;
            }

            if (IrReceiver.decode()) {
                IrReceiver.resume();
                switch (IrReceiver.decodedIRData.command) {
                case BTN_0:
                case BTN_1:
                case BTN_2:
                case BTN_3:
                case BTN_4:
                case BTN_5:
                case BTN_6:
                case BTN_7:
                case BTN_8:
                case BTN_9:
                    custom_grad_currently_selected_color_slot = IrReceiver.decodedIRData.command;
                    break;
                case BTN_HOLD:
                    custom_grad_editor_active = false;
                    custom_grad.save_to_eeprom();
                    continue;

                case BTN_EXIT:
                    custom_grad_editor_active = false;
                    custom_grad.load_from_eeprom();
                    continue;
                case BTN_SETUP:
                    custom_grad_currently_selected_color = custom_grad.getColor(custom_grad_currently_selected_color_slot);
                    custom_grad_color_select_active = true;
                    continue;
                case BTN_M_S_MM:
                    custom_grad.clear();
                    break;
                }
            }

            if (custom_grad.actualSize > 2) {
                for (int i = 0; i < 35; i++) {
                    strip.leds[i] = custom_grad.getNoise(i, count);
                }
                count += count_increment / ANIMATION_SPEED_CONTROL_MULTIPLIER;
            } else {
                for (int i = 0; i < 35; i++) {
                    strip.leds[i] = i < custom_grad_animation_index ? mRed : mBlack;
                }
                custom_grad_animation_index = (custom_grad_animation_index + 1) % 35;
            }

            mData col = custom_grad.getColor(custom_grad_currently_selected_color_slot);
            for (int i = 35; i < NUMLEDS; i++) {
                strip.leds[i] = col;
            }

            strip.show();
            delay(40);

            // Skip the main animation loop
            continue;
        }

        if (current_brightness > 0) {
            strip.setBrightness(current_brightness);
            switch (current_gradient) {
            case CurrentGrad::BLUE:
                for (int i = 0; i < NUMLEDS; i++) {
                    strip.leds[i] = blue_grad.get(inoise8(i * 50, count), 255);
                }
                break;
            case CurrentGrad::RED:
                for (int i = 0; i < NUMLEDS; i++) {
                    strip.leds[i] = red_grad.get(inoise8(i * 50, count), 255);
                }
                break;
            case CurrentGrad::GREEN:
                for (int i = 0; i < NUMLEDS; i++) {
                    strip.leds[i] = green_grad.get(inoise8(i * 50, count), 255);
                }
                break;
            case CurrentGrad::YELLOW:
                for (int i = 0; i < NUMLEDS; i++) {
                    strip.leds[i] = yellow_grad.get(inoise8(i * 50, count), 255);
                }
                break;
            case CurrentGrad::CUSTOM:
                if (custom_grad.actualSize > 2) {
                    for (int i = 0; i < NUMLEDS; i++) {
                        strip.leds[i] = custom_grad.get(inoise8(i * 50, count), 255);
                    }
                } else {
                    for (int i = 0; i < NUMLEDS; i++) {
                        strip.leds[i] = mMaroon;
                    }
                }
                break;
            }

            count += count_increment / ANIMATION_SPEED_CONTROL_MULTIPLIER;
            strip.show();
        }
        delay(40);
        if (IrReceiver.decode()) {
            IrReceiver.resume();
            switch (IrReceiver.decodedIRData.command) {

            case BTN_INFO:
                custom_grad_editor_active = true;
                break;

            case BTN_OK:
                if (repeat_flag()) {
                    // ignore if repeat flag is set
                    break;
                }
                if (lamps_active) {
                    lamps_off();
                    lamps_active = false;
                } else {
                    lamps_on();
                    lamps_active = true;
                }
                break;

            case BTN_RED:
                current_gradient = CurrentGrad::RED;
                count_increment = 30 * ANIMATION_SPEED_CONTROL_MULTIPLIER;
                break;

            case BTN_GREEN:
                current_gradient = CurrentGrad::GREEN;
                count_increment = 5 * ANIMATION_SPEED_CONTROL_MULTIPLIER;
                break;

            case BTN_BLUE:
                current_gradient = CurrentGrad::BLUE;
                count_increment = 5 * ANIMATION_SPEED_CONTROL_MULTIPLIER;
                break;

            case BTN_YELLOW:
                current_gradient = CurrentGrad::YELLOW;
                count_increment = 5 * ANIMATION_SPEED_CONTROL_MULTIPLIER;
                break;

            case BTN_BLANK:
                current_gradient = CurrentGrad::CUSTOM;
                count_increment = 5 * ANIMATION_SPEED_CONTROL_MULTIPLIER;
                break;

            case BTN_CH_PLUS:
                count_increment = constrain(count_increment + 1, 0, 50 * ANIMATION_SPEED_CONTROL_MULTIPLIER);
                break;
            case BTN_CH_MINUS:
                count_increment = constrain(count_increment - 1, 0, 50 * ANIMATION_SPEED_CONTROL_MULTIPLIER);
                break;

            case BTN_VOL_PLUS:
                target_brightness = constrain(target_brightness + 5, 0, 230);
                break;
            case BTN_VOL_MINUS:
                target_brightness = constrain(target_brightness - 5, 0, 230);
                break;

            case BTN_POWER:
                if (repeat_flag()) {
                    // ignore if repeat flag is set
                    break;
                }
                if (target_brightness > 0) {
                    // we are powered up, need to shut down
                    last_saved_target_brightness = target_brightness;
                    target_brightness = 0;
                    lamps_off();
                } else {
                    // we are shut down, need to power up
                    target_brightness = last_saved_target_brightness;
                    if (lamps_active) {
                        lamps_on();
                    }
                }
                // Debounce
                delay(500);
                break;

            default:
                Serial.println(IrReceiver.decodedIRData.command);
                break;
            }
        }
        if (target_brightness != current_brightness) {
            current_brightness = (int)(current_brightness * 0.9 + target_brightness * 0.1);
        }
    }
}

void loop() {
}