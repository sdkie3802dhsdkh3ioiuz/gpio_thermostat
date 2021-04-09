#include "DHT.h"
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
// we represent a temperature using 7 pins
// to represent the binary form
// pin 12 represents the 2^6 place,
// pin 11 the 2^5 place, and so on.
int pins[] = {12, 11, 10, 9, 8, 7, 6};
int places[] = {64, 32, 16, 8, 4, 2, 1};
// we use 7 pins in order to be able to
// represent temperatures from 0 to 127
// Connect the 7 pins on the Arduino to 7 pins on the Talker's (one of
// the Raspberry Pi's) GPIO pins. On the Arduino I'm using pins 12, 11,
// 10, 9, 8, 7, 6, to connect to the Pi's GPIO pins 23, 24, 25, 8, 7, 12,
// 16, respectively
#define NUM_PINS 7 
void setup() {
  for (int i = 0; i < NUM_PINS; i++) {
    pinMode(pins[i], OUTPUT);
  }
  dht.begin();
}
void loop() {
  int i;
  // Wait between measurements.
  delay(2000);
  float f = dht.readTemperature(true);
  int temperature = int(f + .5);
  // bitwise AND the temperature with each of
  // 64, 32, 16, 8, 4, 1. If the result is 1,
  // turn the corresponding pin on.
  for (i = 0; i < NUM_PINS; i++) {
    if (temperature & places[i]) {
      digitalWrite(pins[i], HIGH);
    } else {
      digitalWrite(pins[i], LOW);
    }
  }
  if (isnan(f)) {
    return;
  }
  // give connected devices time to read from the
  // pins that we output to
  delay(8000); 
}
