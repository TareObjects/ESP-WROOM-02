//  STTS751 Library for STTS751 temperature sensor
//      for Aruduino and ESP8226/ESP-WROOM-02
//
//      by koichi kurahashi 2016-06-24
//         http://jiwashin.blogspot.jp/ 

#include "STTS751.h"

#include <Wire.h>


STTS751::STTS751() {
  
}


void STTS751::setup() {
  delay(100);

  //  mask alert, use 12bit mode, stop
  writeCommandToRegisterAddress(kConfigMaskAlertBit | kConfig_12_bits | kConfigStopBit, kRegisterConfiguration);

  //  conversion twice per second
  writeCommandToRegisterAddress(kConversion2hz, kRegisterConversionRate);
}


float STTS751::readTemperature() {
  uint8_t high = 0;
  uint8_t low  = 0;

  //  start one shot measurement
  writeCommandToRegisterAddress(0x00, kRegisterOneShot);

  waitForReady();
  delay(10);
  high = readRegister(kRegisterTemperatureHigh);
  low  = readRegister(kRegisterTemperatureLow) & 0xf0;

  float temperature = high + (low / 256.0);

  writeCommandToRegisterAddress(0x00, kRegisterOneShot);

//  Serial.print("temperature = ");
//  Serial.println(temperature);

  return temperature;
}

int STTS751::readRegister(int inRegister) {
  setRegisterAddress(inRegister);

  Wire.requestFrom(kI2CAddress, 1);
  return Wire.read();
}

int STTS751::writeCommandToRegisterAddress(int inCommand, int inRegister) {
  Wire.beginTransmission(kI2CAddress);
  Wire.write(inRegister);
  Wire.write(inCommand);
  Wire.endTransmission();
}

void STTS751::waitForReady() {
  //  wait for data ready
  int status;
  while (true) {
    status = readRegister(kRegisterStatus);

    if (!(status & kStatusBusyBit)) {
      break;
    }

    delay(15);
  }

}

int STTS751::setRegisterAddress(int inRegister) {
  Wire.beginTransmission(kI2CAddress);
  Wire.write(inRegister);
  Wire.endTransmission();
}


