//ESP_UDP_DAC.ino


//  Thanks:
//    http://www.pwv.co.jp/~take/TakeWiki/index.php?arduino%2FDACを試す
//    arduino/DACを試す
//


#define USE_US_TIMER 1

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SPI.h>

extern "C" {
#include <user_interface.h>
}

//
//  constants
//
const int kRightCh = 0;
const int kLeftCh  = 1;

const int kPinLatchDAC  = 16;
const int kPinLED   = 12;

const int kMaxDacValue = 4096;

//
//  Buffer for Sending
//
const int kMaxDacBuffer = 49;
typedef struct {
  uint8_t isRight;
  uint8_t length;
  uint16_t dacBuffer[kMaxDacBuffer];
} OneBuffer;


typedef struct {
  OneBuffer right;
  OneBuffer left;
} Stereo;


Stereo stereo[2];

volatile int  iDacBuffer;
volatile int sendingSet = 0;


//
//  WiFi Setting
//

#include <WiFiUDP.h>
static WiFiUDP wifiUdp; 
static const char *kReceiverIpadr = "192.168.0.51";
static const int kReceiverPort = 5431;


//
//  system
//

ETSTimer Timer;



//
//  init
//
void setup() {
  // put your setup code here, to run once:
  system_timer_reinit();  // to use os_timer_arm_us

  Serial.begin(115200);

  setupDac();
  setupWiFi();

  iDacBuffer = -1;

  //  int half  = kMaxDacValue / 2;
  //  int quarter = kMaxDacValue / 2;
  //  for (float i = 0; i < kMaxDacBuffer; i++) {
  //    dacBuffer[kRightCh][(int)i] = sin(i * 3.141592 * 2.000000 / (float)kMaxDacBuffer) * half    + half;
  //    dacBuffer[kLeftCh ][(int)i] = cos(i * 3.141592 * 2.000000 / (float)kMaxDacBuffer) * quarter + quarter;
  //  }

  //  iDacBuffer = 0;
  os_timer_setfn(&Timer, outputToDAC, NULL);
  os_timer_arm_us(&Timer, 100, true);
}



//
//  DAC init
//
void setupDac() {
  //  set pinMode
  pinMode(kPinLatchDAC, OUTPUT);
  pinMode(SS,           OUTPUT);

  //  init SPI for DAC
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV8);
  SPI.setDataMode(SPI_MODE0);
}

//
//  WiFi init
//
void setupWiFi() {
//  WiFi.mode(WIFI_AP);

  Serial.println("");

  WiFi.begin("ssid", "password");
  while( WiFi.status() != WL_CONNECTED) {
    delay(500);  
    Serial.print('.');
  }  
  Serial.println("connected");

//  WiFi.softAP("foobar", "12345678", 1, 0);
//  WiFi.config(IPAddress(192, 168, 0, 51), WiFi.gatewayIP(), WiFi.subnetMask());

  IPAddress myIpAddress = WiFi.localIP();
  Serial.print("my ip address=");
  Serial.println(myIpAddress);
  delay(1000);
  
  wifiUdp.begin(kReceiverPort);
}





//
//  Main Loop
//
void loop() {
  // put your main code here, to run repeatedly:
  int bytes = wifiUdp.parsePacket();
  if ( bytes ) {
    Serial.println(bytes);
    if (bytes > sizeof(Stereo)) {
      bytes = sizeof(Stereo);
    }
    int receivingSet = 1-sendingSet;
    wifiUdp.read((char *)&stereo[receivingSet], bytes);

    sendingSet = receivingSet;
    iDacBuffer = 0;
  }
}


int flip = LOW;

void outputToDAC(void *temp) {
  //  digitalWrite(kPinLatchDAC, flip);
  //  flip = !flip;

  if (iDacBuffer == -1)
    return;

  int r = stereo[sendingSet].right.dacBuffer[iDacBuffer];
  digitalWrite(kPinLatchDAC, HIGH) ;
  digitalWrite(SS, LOW) ;
  SPI.transfer((r >> 8) | 0x30) ; // Highバイト(0x30=OUTA/BUFなし/1x/シャットダウンなし)
  SPI.transfer(r & 0xff) ;        // Lowバイトの出力
  digitalWrite(SS, HIGH) ;
  digitalWrite(kPinLatchDAC, LOW) ;       // ラッチ信号を出す

  int l = stereo[sendingSet].left.dacBuffer[iDacBuffer];
  digitalWrite(kPinLatchDAC, HIGH) ;
  digitalWrite(SS, LOW) ;
  SPI.transfer((l >> 8) | 0xB0) ; // Highバイト(0xB0=OUTB/BUFなし/1x/シャットダウンなし)
  SPI.transfer(l & 0xff) ;        // Lowバイトの出力
  digitalWrite(SS, HIGH) ;
  digitalWrite(kPinLatchDAC, LOW) ;       // ラッチ信号を出す

  if (++iDacBuffer >= kMaxDacBuffer) {
    iDacBuffer = -1;
//    sendingSet = 1 - sendingSet;
  }
}


void printMacAddress(uint8_t* macaddr) {
  Serial.print("{");
  for (int i = 0; i < 6; i++) {
    Serial.print("0x");
    Serial.print(macaddr[i], HEX);
    if (i < 5) Serial.print(',');
  }
  Serial.println("}");
}
