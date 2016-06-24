//  Both_STTS751 by koichi kurahashi 2016-06-24
//
//    Library for STTS751 temperature sensor
//      for Aruduino and ESP8226/ESP-WROOM-02
//

#include "STTS751.h"

#include <Wire.h>


STTS751 stts;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.begin();

  stts.setup();
}

void loop() {
  // put your main code here, to run repeatedly:
  float temperature = stts.readTemperature();

  Serial.print("Temperatue = ");
  Serial.println(temperature);

  delay(1000);
}
