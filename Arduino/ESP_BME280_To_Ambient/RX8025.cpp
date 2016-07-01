#include <Arduino.h>
#include <Time.h>
#include <Wire.h>


#include "RX8025.h"



//
//  constructor
//
RX8025::RX8025() {
  powerOn = false;
}

void RX8025::setup(int inCommand) {    // 0x20 = olarm off
  Serial.println("RTC constructor");

  delay(100);
  
  writeRegister(kRegisterCommand1, inCommand);  //  0x20 : alarm off
  writeRegister(kRegisterCommand2, 0x00);  //       0x00 : reset

  int r = readRegister(0x60);
  powerOn = (r & 0x10) == 0x10;
  Serial.print("Register Command2 = ");
  Serial.println(r, HEX);
}

bool RX8025::isPowerOn() {
  Serial.print("PO = ");
  Serial.println(powerOn);
  return powerOn;
}

void RX8025::writeRegister(int inAddr, int inValue) {
  Wire.beginTransmission(kRTC_Address);
  Wire.write(inAddr);   // レジスタ指定（下位4bitは0）
  Wire.write(inValue);  // 値セット
  Wire.endTransmission();
  delay(1);
}

int RX8025::readRegister(int inAddr) {
  Wire.beginTransmission(kRTC_Address);
  Wire.write(inAddr);
  Wire.endTransmission();

  Wire.requestFrom(kRTC_Address, 1);
  while (Wire.available() == 0) ;

  uint8_t r = Wire.read();

  return r;
}


void RX8025::writeRTC(time_t *inTime) {
  int yy, mm, dd, HH, MM, SS;

  setTime(*inTime);

  Serial.print("year = ");
  Serial.println(year());

  yy   = year() % 100;
  mm   = month();
  dd   = day();
  HH   = hour();
  MM   = minute();
  SS   = second();

  char buf[40];
  sprintf(buf, "write rtc = %02d-%02d-%02d %02d:%02d:%02d", yy, mm, dd, HH, MM, SS);
  Serial.println(buf);

  Wire.beginTransmission(kRTC_Address);
  Wire.write(0x00);
  Wire.write(toClockFormat(SS));
  Wire.write(toClockFormat(MM));
  Wire.write(toClockFormat(HH));
  Wire.write(0x00);
  Wire.write(toClockFormat(dd));
  Wire.write(toClockFormat(mm));
  Wire.write(toClockFormat(yy));
  Wire.endTransmission();
  delay(1);
  
  if (powerOn) {
    writeRegister(kRegisterCommand2, 0x00);  //  PO = offa
    powerOn = false;
  }
}

void RX8025::readRTC(time_t *outTime) {
  int yyyy, mm, dd, HH, MM, SS;

  Wire.beginTransmission(kRTC_Address);
  Wire.write(0x00);
  Wire.endTransmission();

  Wire.requestFrom(kRTC_Address, 8);

  Wire.read();
  SS   = fromClockFormat(Wire.read());
  MM   = fromClockFormat(Wire.read());
  HH   = fromClockFormat(Wire.read());
  Wire.read();  //  dummy read
  dd   = fromClockFormat(Wire.read());
  mm   = fromClockFormat(Wire.read());
  yyyy = 2000 + fromClockFormat(Wire.read());

  char buf[40];
  sprintf(buf, "write rtc = %02d-%02d-%02d %02d:%02d:%02d", yyyy, mm, dd, HH, MM, SS);
  Serial.println(buf);

  setTime(HH, MM, SS, dd, mm, yyyy);
  *outTime = now();
}


byte RX8025::fromClockFormat(int inClock) {
  return ((inClock & 0xf0) >> 4) * 10 + (inClock & 0x0f);
}

byte RX8025::toClockFormat(int inValue) {
  return abs(inValue / 10) * 16 + (inValue % 10);
}

