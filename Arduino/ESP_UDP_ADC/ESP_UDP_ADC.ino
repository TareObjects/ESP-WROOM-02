//ESP_UDP_ADC.ino


#define USE_US_TIMER 1

#include <Arduino.h>
#include <ESP8266WiFi.h>
extern "C" {
  #include <user_interface.h>
}


//  
//  constants
//

const int kRightCh = 0;
const int kLeftCh  = 1;

//  SINカーブで試すならコメントアウト
//#define USE_ADC 1

//
//  Buffer for Send/Receive
//
const int kMaxBuffer =  49;

typedef struct {
  uint8_t isRight;
  uint8_t length;
  uint16_t buffer[kMaxBuffer];
} OneBuffer;


typedef struct {
  OneBuffer right;
  OneBuffer left;
} Stereo;


Stereo stereo[2];

volatile int iBuffer;
volatile int fillingSet = 0;

const int SendChoiceHold = -1;

volatile int ShouldSend = SendChoiceHold;



//
//  WiFi
//
#include <WiFiUDP.h>
static WiFiUDP wifiUdp; 
static const char *kReceiverIpadr = "192.168.0.73";
static const int kReceiverPort = 5431;

//
//  ADC
//
#include <SPI.h>

const int kMaxDacValue = 4095;



//
//  system
//

ETSTimer Timer;




//
//  ADC init
//
void setupDac() {
  //  set pinMode
//  pinMode(kPinLatchDAC, OUTPUT);
  pinMode(SS, OUTPUT);

  //  init SPI for DAC
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV8);
  SPI.setDataMode(SPI_MODE0);
}



void setup() {
  system_timer_reinit();  // to use os_timer_arm_us
  Serial.begin(115200);

  // ADC
  setupDac();

  
 static const int kLocalPort = 7000;

  WiFi.begin("ssid", "password");
  while( WiFi.status() != WL_CONNECTED) {
    delay(500);  
    Serial.print('.');
  }  
  Serial.println("connected");

  Serial.print("my ip address is ");
  Serial.println(WiFi.localIP());
   
  wifiUdp.begin(kLocalPort);

  
#ifndef USE_ADC
  int half   = kMaxDacValue / 2;
  int quarter = kMaxDacValue / 2;
  for (int set = 0; set < 2; set++) {
    stereo[set].right.isRight = 1;
    stereo[set].left.isRight  = 0;
    stereo[set].right.length = kMaxBuffer;
    stereo[set].left.length  = kMaxBuffer;
    
    for (float i = 0; i < kMaxBuffer; i++) {
      int value = sin(i * 3.141592 * 2.000000 / (float)kMaxBuffer) * half    + half;
      int ii = (int)i;
      stereo[set].right.buffer[ii] = value;
      stereo[set].left.buffer[ii]  = value;
    }
  }
#endif
  
  ShouldSend = SendChoiceHold;
  
  os_timer_setfn(&Timer, outputToADC, NULL);
  os_timer_arm_us(&Timer,100, true);
}

void loop() {
  if (ShouldSend != SendChoiceHold) {
//    esp_now_send(mac, (u8*)&stereo[ShouldSend], sizeof(Stereo));
    
    wifiUdp.beginPacket(kReceiverIpadr, kReceiverPort);
    char *conv = (char *)&stereo[ShouldSend];
    wifiUdp.write(conv, sizeof(Stereo));
    wifiUdp.endPacket();
        
    Serial.print(ShouldSend);    
    ShouldSend = -1;
  }
}


int flip = LOW;

void outputToADC(void *temp) {
//  int a = readADC(kRightCh);
//  char buf[32], buf2[32];
//  itoa(a, buf, 2);
//  sprintf(buf2, "%16s %5d %5d", buf, a, readADC(kLeftCh));
//  Serial.println(buf2);

#ifdef USE_ADC
  stereo[fillingSet].right.buffer[iBuffer] = readADC(kRightCh);
  stereo[fillingSet].left .buffer[iBuffer] = readADC(kLeftCh);
#endif

  if (++iBuffer >= kMaxBuffer) {
    ShouldSend = fillingSet;

    iBuffer = 0;
    fillingSet = 1 - fillingSet;
  }
}

int readADC(int ch) {
  digitalWrite(SS, LOW);
  SPI.transfer(0b01100000 | ch << 2); //  start=1, single=1, d2=0
  uint16_t result = word(SPI.transfer(0x00), SPI.transfer(0x00)) >> 4;  
  digitalWrite(SS, HIGH);


  return result;
}

