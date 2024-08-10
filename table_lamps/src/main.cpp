#include <IRremote.hpp>

#define IR_RECEIVE_PIN 2 // ИК пин
#define STRIP_PIN 5      // пин ленты
#define LAMP_PIN 8
#define NUMLEDS 50 // кол-во светодиодов

#include <microLED.h>
microLED<NUMLEDS, STRIP_PIN, MLED_NO_CLOCK, LED_WS2812, ORDER_GRB, CLI_AVER> strip;
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

enum CurrentGrad {
    RED,
    GREEN,
    YELLOW,
    BLUE
};

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

    // Speed of the animation
    int count_increment = 5;

    // For power on/off animation (return to previous brightness on power on)
    int last_saved_target_brightness = 220;

    int current_brightness = 220;
    int target_brightness = 220;

    // State of the lamps
    bool lamps_active = false;

    

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

            default:
                break;
            }

            count += count_increment;
            strip.show();
        }
        delay(40);
        if (IrReceiver.decode()) {
            IrReceiver.resume();
            switch (IrReceiver.decodedIRData.command) {

            case BTN_OK:
                if (repeat_flag()) {
                    // ignore if repeat flag is set
                    break;
                }
                if(lamps_active) {
                    lamps_off();
                    lamps_active = false;
                } else {
                    lamps_on();
                    lamps_active = true;
                }
                break;

            case BTN_RED:
                current_gradient = CurrentGrad::RED;
                count_increment = 30;
                break;

            case BTN_GREEN:
                current_gradient = CurrentGrad::GREEN;
                count_increment = 5;
                break;

            case BTN_BLUE:
                current_gradient = CurrentGrad::BLUE;
                count_increment = 5;
                break;

            case BTN_YELLOW:
                current_gradient = CurrentGrad::YELLOW;
                count_increment = 5;
                break;

            case BTN_CH_PLUS:
                count_increment = constrain(count_increment + 1, 0, 230);
                delay(500);
                break;
            case BTN_CH_MINUS:
                count_increment = constrain(count_increment - 1, 0, 230);
                delay(500);
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
                    if(lamps_active) {
                        lamps_on();
                    }
                }
                // Debounce
                delay(500);
                break;

            default:
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