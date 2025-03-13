#include <Arduino.h>

#define FASTLED_INTERRUPT_RETRY_COUNT 1
#define FASTLED_ALLOW_INTERRUPTS 0

#include <FastLED.h>

// the setup function runs once when you press reset or power the board

#define NUM_LEDS 22

CRGB leds[NUM_LEDS];

float current_average_volume_30s = 0;
float current_average_volume_1s = 0;
float target_peak_volume = 900;

void setup() { // initialize digital pin 13 as an output.
    pinMode(A0, INPUT);
    pinMode(D3, OUTPUT);
    pinMode(D4, OUTPUT);
    analogWriteFreq(27000);

    FastLED.addLeds<WS2815, D3>(leds, NUM_LEDS);
}

// the loop function runs over and over again forever

void loop() {
    float raw_volume = analogRead(A0);
    for (int i = 0; i < 100; i++) {
        raw_volume = raw_volume * 0.5 + analogRead(A0) * 0.5;
        delay(0.1); // 10 Ms
    }

    current_average_volume_30s = current_average_volume_30s * 0.98 + raw_volume * 0.02;
    if (raw_volume > current_average_volume_30s + 20) {
        current_average_volume_30s = current_average_volume_30s * 0.7 + raw_volume * 0.3;
    }
    current_average_volume_1s = current_average_volume_1s * 0.8 + raw_volume * 0.2;

    for (int ib = 0; ib < NUM_LEDS; ib++) {
        leds[ib] = CHSV(current_average_volume_1s * 25, 255, 255);
    }
    analogWrite(D4, 0);
    FastLED.show();
    analogWrite(D4, max(255 - max(current_average_volume_30s - target_peak_volume, 0.0f), 35.0f));
    delay(20);
}