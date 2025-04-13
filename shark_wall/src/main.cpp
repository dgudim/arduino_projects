#include <Arduino.h>
#include <FastLED.h>

const int BrightnessLevel = A0;
const int BlueIn = A1;
const int GreenIn = A2;
const int RedIn = A3;
const int Button = 10;
const int Switch = 6;
const int Transistor = 7;
const int LED = 9;
// variable for storing the potentiometer value
int BrightnessLevelValue = 0;
int BlueInValue = 0;
int GreenInValue = 0;
int RedInValue = 0;
int ButtonValue = 0;
int SwitchValue = 0;

#define NUM_LEDS 1
#define LEDstrip 8

CRGB leds[NUM_LEDS];

void setup() {
    Serial.begin(9600);
    pinMode(BrightnessLevel, INPUT);
    pinMode(BlueIn, INPUT);
    pinMode(GreenIn, INPUT);
    pinMode(RedIn, INPUT);
    pinMode(Button, INPUT_PULLDOWN);
    pinMode(Switch, INPUT_PULLDOWN);
    pinMode(LED, OUTPUT);
    pinMode(Transistor, OUTPUT);
    delay(1000);
    FastLED.addLeds<WS2815, LEDstrip>(leds, NUM_LEDS);  // GRB ordering is assumed

}

void loop() {
    // Reading potentiometer value
    BrightnessLevelValue = analogRead(BrightnessLevel);
    BlueInValue = analogRead(BlueIn);
    GreenInValue = analogRead(GreenIn);
    RedInValue = analogRead(RedIn);
    ButtonValue = digitalRead(Button);
    SwitchValue = digitalRead(Switch);
    digitalWrite(LED, HIGH);
    digitalWrite(Transistor,HIGH);
    Serial.println(BrightnessLevelValue);
    Serial.println(BlueInValue);
    Serial.println(GreenInValue);
    Serial.println(RedInValue);
    Serial.println(ButtonValue);
    Serial.println(SwitchValue);
    Serial.println();
    delay(500);
    
      // Turn the LED on, then pause
  leds[0] = CRGB::Red;
  FastLED.show();
  delay(500);
  // Now turn the LED off, then pause
  leds[0] = CRGB::Black;
  FastLED.show();
  delay(500);
}