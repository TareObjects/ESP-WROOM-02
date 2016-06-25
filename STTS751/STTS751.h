//  STTS751 Library for STTS751 temperature sensor
//      for Aruduino and ESP8226/ESP-WROOM-02
//
//      by koichi kurahashi 2016-06-24
//         http://jiwashin.blogspot.jp/ 
//
//
// STTS751 Connection
//
//  MCU - STTS751
//  3v3 - 3 Vcc
//  GND - 2 Gnd
//  SDA - 6 SDA
//  SCL - 4 SCL
//

#pragma once

#include <Arduino.h>



class STTS751 {
  public:
    const int kI2CAddress =  0x39;


    const int kRegisterTemperatureHigh  = 0x00;
    const int kRegisterStatus           = 0x01;
    const int kRegisterTemperatureLow   = 0x02;
    const int kRegisterConfiguration    = 0x03;
    const int kRegisterConversionRate   = 0x04;

    const int kRegisterOneShot          = 0x0f;

    const int kConfigMaskAlertBit = 0x80;
    const int kConfigStopBit      = 0x40;

    const int kStatusBusyBit      = 0x80;

    const int kConfig_10_bits     = 0b00 << 2;
    const int kConfig_11_bits     = 0b01 << 2;
    const int kConfig_12_bits     = 0b11 << 2;
    const int kConfig_09_bits     = 0b10 << 2;

    const int kConversion1_16hz = 0x00;
    const int kConversion1_8hz  = 0x01;
    const int kConversion1_4hz  = 0x02;
    const int kConversion1_2hz  = 0x03;
    const int kConversion1hz    = 0x04;
    const int kConversion2hz    = 0x05;
    const int kConversion4hz    = 0x06;
    const int kConversion8hz    = 0x07;
    const int kConversion16hz   = 0x08;
    const int kConversion32hz   = 0x09;

    STTS751();

    void setup();

    float readTemperature();
    int   readRegister(int inRegister);
    int   writeCommandToRegisterAddress(int inCommand, int inRegister);

  private:
    void  waitForReady();
    int   setRegisterAddress(int inRegister);
};
