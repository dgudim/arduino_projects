#include <Arduino.h>

#include <FastLED.h>

#define NUM_LEDS 22
CRGB leds[NUM_LEDS];

#define BAT_SENSE_PIN A0
#define CHARGE_ENABLE_PIN 4
#define SHAKER_PIN 10
#define PWM_PIN A1
#define WS2815_PIN 2
#define TWELVE_VOLT_OUT_ENABLED_PIN 3
#define CHARGE_SENSE_PIN 7
#define BUZZER_PIN 5
#define BUTTON_PIN 6
#define BUTTON2_PIN 21

void setup() {
    pinMode(BAT_SENSE_PIN, INPUT);

    pinMode(CHARGE_ENABLE_PIN, OUTPUT);
    pinMode(PWM_PIN, OUTPUT);
    pinMode(WS2815_PIN, OUTPUT);
    pinMode(TWELVE_VOLT_OUT_ENABLED_PIN, OUTPUT);
    pinMode(CHARGE_SENSE_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    pinMode(SHAKER_PIN, INPUT_PULLUP);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(BUTTON2_PIN, INPUT_PULLUP);

    delay(5000);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(40);
    digitalWrite(BUZZER_PIN, LOW);
    delay(500);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);

    FastLED.addLeds<WS2815, WS2815_PIN, BRG>(leds, NUM_LEDS);
}

int shaker_prev = 0;

void loop() {
    // for (int i = 0; i < 1024; i++) {
    //     analogWrite(PWM_PIN, i);
    //     delay(5);
    // }
    // for (int i = 1023; i > 0; i--) {
    //     analogWrite(PWM_PIN, i);
    //     delay(5);
    // }

    digitalWrite(CHARGE_ENABLE_PIN, HIGH);
    digitalWrite(TWELVE_VOLT_OUT_ENABLED_PIN, HIGH);

    int filled = analogRead(BAT_SENSE_PIN) / 190;
    CRGB col = CRGB::Cyan;

    if (digitalRead(BUTTON_PIN) == 1) {
        col = CRGB::White;
    }

    if (digitalRead(BUTTON2_PIN) == 1) {
        col = CRGB::Orange;
    }

    for (int i = 0; i < NUM_LEDS; i++) {

        leds[i] = i < filled ? col : CRGB::Black;
    }
    FastLED.show();

    uint16_t shaker_enabled = digitalRead(SHAKER_PIN);
    if (shaker_enabled != shaker_prev) {
        shaker_prev = shaker_enabled;
        for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = CRGB::Red;
        }
        FastLED.show();
    }
}