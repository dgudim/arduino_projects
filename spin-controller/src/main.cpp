/*
 * Microstepping demo
 *
 * This requires that microstep control pins be connected in addition to STEP,DIR
 *
 * Copyright (C)2015 Laurentiu Badea
 *
 * This file may be redistributed under the terms of the MIT license.
 * A copy of this license has been included with this distribution in the file LICENSE.
 */
#include <Arduino.h>
#include <FastAnalogRead.h>

// Motor steps per revolution. Most steppers are 200 steps or 1.8 degrees/step
#define MOTOR_STEPS 200

#define DIR 4
#define STEP 7

#define pot1_pin A4
#define pot2_pin A5

#define ADC_SMOOTHING 0.2

FastAnalogRead pot1;
FastAnalogRead pot2;

int coun;
bool dir = 0;
int del;
int c = 1912;
int cf = 1805;
int d = 1703;
int df = 1607;
int e = 1517;
int f = 1431;
int ff = 1351;
int g = 1275;
int gf = 1203;
int a = 1136;
int af = 1072;
int b = 1012;
int c1 = floor(c / 2);
int cf1 = floor(cf / 2);
int d1 = floor(d / 2);
int df1 = floor(df / 2);
int e1 = floor(e / 2);
int f1 = floor(1431 / 2);
int ff1 = floor(1351 / 2);
int g1 = floor(1275 / 2);
int gf1 = floor(1203 / 2);
int a1 = floor(1136 / 2);
int af1 = floor(1072 / 2);
int b1 = floor(1012 / 2);
int e0 = e * 2;
int g0 = g * 2;
int b0 = b * 2;
int af0 = af * 2;
int a0 = a * 2;
int f0 = f * 2;
int use = 60;
int tempo = 120;
int oct = 5;

void note(int num, long dur) {
    del = (num * oct) / 400;
    dir = !dir;
    digitalWrite(DIR, dir);
    coun = floor((dur * 5 * tempo) / del);
    for (int x = 0; x < coun; x++) {
        digitalWrite(STEP, HIGH);
        delayMicroseconds(del);
        digitalWrite(STEP, LOW);
        delayMicroseconds(del);
    }
}

void pa(int durp) {
    int ker = floor(durp / 100) * tempo;
    delay(ker);
}

void music() {
    note(e1, 250);
    note(e1, 500);
    note(e1, 250);
    pa(250);
    note(c1, 250);
    note(e1, 500);
    note(g1, 1000);
    note(g, 1000);
    note(c1, 500);
    pa(250);
    note(g, 250);
    pa(500);
    note(e, 500);
    pa(250);
    note(a, 250);
    pa(250);
    note(b, 250);
    pa(250);
    note(af, 250);
    note(a, 500);
    note(g, 330);
    note(e1, 330);
    note(g1, 330);
    note(a1, 500);
    note(f1, 250);
    note(g1, 250);
    pa(250);
    note(e1, 250);
    pa(250);
    note(c1, 250);
    note(d1, 250);
    note(b, 250);
    pa(1000);

    for (int i = 0; i < 3; i++) {
        note(d, 100);
        pa(use);
        note(f, 100);
        pa(use);
        note(c1, 100);
        pa(use);
        note(f, 100);
        pa(use);
    }
    note(c1, 100);
    pa(use);
    note(c1, 100);
    pa(use);
    note(af, 100);
    pa(use);
    note(a, 100);
    pa(use);
    for (int i = 0; i < 3; i++) {
        note(c, 100);
        pa(use);
        note(e, 100);
        pa(use);
        note(af, 100);
        pa(use);
        note(e, 100);
        pa(use);
    }
    note(af, 100);
    pa(use);
    note(af, 100);
    pa(use);
    note(a, 100);
    pa(use);
    note(f, 100);
    pa(use);
    for (int i = 0; i < 3; i++) {
        note(d, 100);
        pa(use);
        note(f, 100);
        pa(use);
        note(af, 100);
        pa(use);
        note(f, 100);
        pa(use);
    }
    for (int i = 0; i < 3; i++) {
        note(af0, 100);
        pa(use);
        note(d, 100);
        pa(use);
        note(f, 100);
        pa(use);
        note(a, 100);
        pa(use);
    }
}

void setup() {

    pinMode(pot1_pin, INPUT);
    pinMode(pot2_pin, INPUT);

    pinMode(STEP, OUTPUT);
    pinMode(DIR, OUTPUT);

    digitalWrite(DIR, HIGH);

    pot1.begin(pot1_pin, true);
    pot2.begin(pot2_pin, true);

    // enable faster ADC mode globally
    FastAnalogRead::enableFastADC();

    // music();

    note(af0, 100);
    pa(use);
    note(d, 100);
    pa(use);
    note(f, 100);
    pa(use);
    note(a, 100);
    pa(use);

    // Serial.begin(9600);
}

void loop() {
    pot1.update();
    //    pot2.update();

    long delay = pot1.getValue() * 2 + 26;

    // Serial.println(delay);

    digitalWrite(STEP, HIGH);
    delayMicroseconds(delay);
    digitalWrite(STEP, LOW);
    delayMicroseconds(delay);
}