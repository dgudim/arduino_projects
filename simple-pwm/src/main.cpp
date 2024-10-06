#include <Arduino.h>

void setup() {
  pinMode(A7, INPUT);
  pinMode(5, INPUT);
  pinMode(3, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  const float total_delay = 500; // 2Khz 


  while(true) {
      int raw_value = analogRead(A7);
      float duty = raw_value / 1024.0;
      float on_time = total_delay * duty;
      digitalWrite(3, HIGH);
      delayMicroseconds(on_time);
      digitalWrite(3, LOW);
      delayMicroseconds(total_delay - on_time);
  }
}