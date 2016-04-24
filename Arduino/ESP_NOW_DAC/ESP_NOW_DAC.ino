//ESP_DAC_NOW.ino


//	Thanks:
//		http://www.pwv.co.jp/~take/TakeWiki/index.php?arduino%2FDACを試す
//		arduino/DACを試す
//
//  	http://lowreal.net/2016/01/14/2
//  	ESP8266 の低消費電力の限界をさぐる (ESP-NOWを使ってみる)


#define USE_US_TIMER 1

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SPI.h>

extern "C" {
#include <espnow.h>
#include <user_interface.h>
}

//
//	constants
//
const int kRightCh = 0;
const int kLeftCh  = 1;

const int kPinLatchDAC	= 16;
const int kPinLED		= 12;

const int kMaxDacValue = 4096;

//
//	Buffer for Sending
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

volatile int 	iDacBuffer;
volatile int sendingSet = 0;


//
//	WiFi Setting
//

const int kWiFiDefaultChannel = 1;
uint8_t mac[][6] = {
  {0x18, 0xFE, 0x34, 0xEF, 0x5C, 0x76},
  {0x18, 0xFE, 0x34, 0xEE, 0x6D, 0xFC},
  {0x5C, 0xCF, 0x7F, 0x08, 0x36, 0x9A},
  {0x5C, 0xCF, 0x7F, 0x08, 0x36, 0xA1}
};

//
//	system
//

ETSTimer Timer;



//
//	init
//
void setup() {
  // put your setup code here, to run once:
  system_timer_reinit();  // to use os_timer_arm_us

  Serial.begin(115200);

  setupDac();
  setupWiFi();

  iDacBuffer = -1;

  //  int half 	= kMaxDacValue / 2;
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
//	DAC init
//
void setupDac() {
  //	set pinMode
  pinMode(kPinLatchDAC, OUTPUT);
  pinMode(SS,           OUTPUT);

  //	init SPI for DAC
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV8);
  SPI.setDataMode(SPI_MODE0);
}

//
//	WiFi init
//
void setupWiFi() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("foobar", "12345678", 1, 0);

  uint8_t macaddr[6];
  wifi_get_macaddr(STATION_IF, macaddr);
  Serial.print("mac address (STATION_IF): ");
  printMacAddress(macaddr);

  wifi_get_macaddr(SOFTAP_IF, macaddr);
  Serial.print("mac address (SOFTAP_IF): ");
  printMacAddress(macaddr);

  if (esp_now_init() == 0) {
    Serial.println("init");
  } else {
    Serial.println("init failed");
    ESP.restart();
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);

  esp_now_register_recv_cb([](uint8_t *macaddr, uint8_t *data, uint8_t len) {
    int receivingSet = 1 - sendingSet;

    memcpy((void *)&stereo[receivingSet], data, len);

    Serial.print(stereo[receivingSet].right.dacBuffer[0]);
    Serial.print(" ");
    
    iDacBuffer = 0;
    sendingSet = receivingSet;
  });

  esp_now_register_send_cb([](uint8_t* macaddr, uint8_t status) {
    Serial.println("send_cb");

    Serial.print("mac address: ");
    printMacAddress(macaddr);

    Serial.print("status = "); Serial.println(status);
  });


}


//
//	Main Loop
//
void loop() {
  // put your main code here, to run repeatedly:
  delay(1000);
}


int flip = LOW;

void outputToDAC(void *temp) {
  //  digitalWrite(kPinLatchDAC, flip);
  //  flip = !flip;

  if (iDacBuffer == -1)
    return;

  int r = stereo[sendingSet].right.dacBuffer[iDacBuffer];
  digitalWrite(kPinLatchDAC, HIGH);
  digitalWrite(SS, LOW);
  SPI.transfer((r >> 8) | 0x30);
  SPI.transfer(r & 0xff);
  digitalWrite(SS, HIGH);
  digitalWrite(kPinLatchDAC, LOW) ;
  
  int l = stereo[sendingSet].left.dacBuffer[iDacBuffer];
  digitalWrite(kPinLatchDAC, HIGH) ;
  digitalWrite(SS, LOW);
  SPI.transfer((l >> 8) | 0xB0);
  SPI.transfer(l & 0xff);
  digitalWrite(SS, HIGH);
  digitalWrite(kPinLatchDAC, LOW);
  
  if (++iDacBuffer >= kMaxDacBuffer) {
    iDacBuffer = -1;
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
