#pragma once


class RX8025 {
  private:
    const int     kRTC_Address = 0x32;
    bool          powerOn      = false;

    
  public:
    const int kRegisterSeconds      = 0x00;
    const int kRegisterMinutes      = 0x01;
    const int kRegisterHours        = 0x02;
    const int kRegisterWeekDays     = 0x03;
    const int kRegisterDays         = 0x04;
    const int kRegisterMoths        = 0x05;
    const int kRegisterYears        = 0x06;
    const int kRegisterDitalOffset  = 0x07;
    const int kRegisterAlermWMinute = 0x08;
    const int kRegisterAlermWHour   = 0x09;
    const int kRegisterAlermWeekday = 0x0a;
    const int kRegisterAlermDMinute = 0x0b;
    const int kRegisterAlermDHour   = 0x0c;
    const int kRegisterReserved     = 0x0d;
    const int kRegisterCommand1     = 0xe0;
    const int kRegisterCommand2     = 0xf0;

    //  
    //  constructor
    //
    RX8025();

    // setup
    void setup(int inCommand = 0x20);
    
    //  PO flag
    bool isPowerOn();

    //  write to RX8025's register. inAddr should be use kRegisterxx.
    void writeRegister(int inAddr, int inValue);
    //  read from RX8025's register. inAddr should be use kRegisterxx.
    int readRegister(int inAddr);

    //  read / write in unix time
    void writeRTC(time_t *inTime);
    void readRTC(time_t *outTime);


  private:
    byte fromClockFormat(int inClock);
    byte toClockFormat(int inValue);
};

