//ESP_ADC_NOW.ino


//  Thanks:
//    http://lowreal.net/2016/01/14/2
//    ESP8266 の低消費電力の限界をさぐる (ESP-NOWを使ってみる)
//

#define USE_US_TIMER 1

#include <Arduino.h>
#include <ESP8266WiFi.h>
extern "C" {
  #include <espnow.h>
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
#define WIFI_DEFAULT_CHANNEL 1

uint8_t mac[] = {0x1A,0xFE,0x34,0xEE,0x6D,0x11};

void printMacAddress(uint8_t* macaddr) {
  Serial.print("{");
  for (int i = 0; i < 6; i++) {
    Serial.print("0x");
    Serial.print(macaddr[i], HEX);
    if (i < 5) Serial.print(',');
  }
  Serial.println("}");
}

//
//  MCP3008
//
#include <SPI.h>

const int kMaxDacValue = 1023;



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

  
//  Serial.println("Initializing...");
  WiFi.mode(WIFI_STA);

  uint8_t macaddr[6];
  wifi_get_macaddr(STATION_IF, macaddr);
  Serial.print("mac address (STATION_IF): ");
  printMacAddress(macaddr);

  wifi_get_macaddr(SOFTAP_IF, macaddr);
  Serial.print("mac address (SOFTAP_IF): ");
  printMacAddress(macaddr);

  if (esp_now_init()==0) {
    Serial.println("direct link  init ok");
  } else {
    Serial.println("dl init failed");
    ESP.restart();
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_recv_cb([](uint8_t *macaddr, uint8_t *data, uint8_t len) {
    Serial.println("recv_cb");

    Serial.print("mac address: ");
    printMacAddress(macaddr);

    Serial.print("data: ");
    for (int i = 0; i < len; i++) {
      Serial.print(data[i], HEX);
    }
    Serial.println("");
  });
  
  esp_now_register_send_cb([](uint8_t* macaddr, uint8_t status) {
  });

  int res = esp_now_add_peer(mac, (uint8_t)ESP_NOW_ROLE_SLAVE,(uint8_t)WIFI_DEFAULT_CHANNEL, NULL, 0);


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
    esp_now_send(mac, (u8*)&stereo[ShouldSend], sizeof(Stereo));
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
  SPI.transfer(0b00011000 | ch); //  start=1, single=1, d2=0
  uint16_t result = word(0x3f & SPI.transfer(0x00), SPI.transfer(0x00)) >> 2;  
  digitalWrite(SS, HIGH);
  
  return result;
}

