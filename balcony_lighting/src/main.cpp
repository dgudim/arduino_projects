#include <Arduino.h>
#include <IRremote.hpp>

#define IR_RECEIVE_PIN 7  // ИК пин
#define STRIP_PIN 9       // пин ленты
#define STRIP_POWER_PIN 5 // пин питания ленты
#define NUMLEDS 186       // кол-во светодиодов

#include <microLED.h>
microLED<NUMLEDS, STRIP_PIN, MLED_NO_CLOCK, LED_WS2815, ORDER_GRB, CLI_AVER> strip;
#include <FastLEDsupport.h> // нужна для шума

#define BTN_RED 73
#define BTN_GREEN 13
#define BTN_YELLOW 64
#define BTN_BLUE 74

void fillWithColor(mData col) {
    for (int i = 0; i < NUMLEDS; i++) {
        strip.leds[i] = col;
    }
    strip.show();
}

void setup() {
    Serial.begin(112500);
    pinMode(A0, INPUT);
    pinMode(STRIP_POWER_PIN, OUTPUT);
    pinMode(STRIP_PIN, OUTPUT);
    pinMode(IR_RECEIVE_PIN, OUTPUT);

    IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);

    strip.setBrightness(230);
    digitalWrite(STRIP_POWER_PIN, HIGH);
    fillWithColor(mRed);
    delay(3000);
    fillWithColor(mBlack);
}

void power_cycle() {
    pinMode(STRIP_PIN, INPUT);
    digitalWrite(STRIP_POWER_PIN, LOW);

    delay(30000);

    pinMode(STRIP_PIN, OUTPUT);
    digitalWrite(STRIP_POWER_PIN, HIGH);

    fillWithColor(mYellow);
}

void loop() {

    bool powered = false;

    mGradient<8> red_grad;
    red_grad.colors[0] = mRed;
    red_grad.colors[1] = mOrange;
    red_grad.colors[2] = mRed;
    red_grad.colors[3] = mOrange;
    red_grad.colors[4] = mRed;
    red_grad.colors[5] = mMagenta;
    red_grad.colors[6] = mBlue;
    red_grad.colors[7] = mOrange;

    int count = 0;

    while (true) {
        if (IrReceiver.decode()) {
            IrReceiver.resume();
            if (IrReceiver.decodedIRData.command == BTN_RED) {
                powered = false;
                fillWithColor(mBlack);
                strip.show();
            }
            if (IrReceiver.decodedIRData.command == BTN_GREEN) {
                powered = true;
            }
        }
        if (powered) {
            for (int i = 0; i < NUMLEDS; i++) {
                strip.leds[i] = red_grad.get(inoise8(i, count), 255);
            }
            strip.show();
            count++;
        }
        delay(20);
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