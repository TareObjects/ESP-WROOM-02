//  ESP_STTS751_Bathtab
//    by koichi kurahashi 2016-06-13
//
//    Thanks:
//      電子工作部
//      https://plus.google.com/u/0/communities/114133885055784182372
//


extern "C" {
#include "user_interface.h"
}


#include <Time.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#include <Wire.h>


//
// Wifi
//
#include "../../private_ssid.h"
//const char *ssid     = "*************";
//const char *password = "*************";
WiFiClient client;


//
// STTS751 I2C address is 0x39(57)
//
//
//  MCU - STTS751
//  3v3 - 3 Vcc
//  GND - 2 Gnd
//  SDA - 6 SDA
//  SCL - 4 SCL
//
#include "STTS751.h"

STTS751 stts751;


//
//  Setup
//

int waitSec = 5 * 60;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  //  GND for special board
  pinMode(12, OUTPUT);
  digitalWrite(12, LOW);

  stts751.setup();

  //
  //  Wifi
  //
  WiFi.begin ( ssid, password );
  Serial.println("Started");

  // read temperature
  float temperature = stts751.readTemperature();

  // Wait for connection
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }
  Serial.println("Wifi Connected");

  //  send to thingspeak.com
  sendTemperature(temperature);

  //  into deep sleep
  waitSec = temperature > 30.0 ? 300 : 600;
  ESP.deepSleep((waitSec - second()) * 1000 * 1000, WAKE_RF_DEFAULT);
  delay(1000);
}


void loop() {
//  Serial.println(stts751.readTemperature());
  delay(1000);
}



//
//  send to thing speak
//
void sendTemperature(float inTemperature) {
  char fBuf[10];

  ftoa(inTemperature, fBuf);

  String postStr = "&field1=" + String(fBuf);
  send(postStr);

  Serial.println(postStr);
}

void send(String inPostStr) {
  String apiKey = "<your api key here";
  Serial.print("Connecting...");
  if (client.connect("184.106.153.149", 80)) {  //  api.thingspeak.com
    Serial.print("Connected....");
    String postStr = apiKey + inPostStr + "\r\n\r\n";
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
    Serial.println("posted.");
  }
  client.stop();
}


void ftoa(float f, char *buf) {
  float a = int(f);
  float b = int((f - a) * 100.0000);

  sprintf(buf, "%d.%d", (int)a, (int)b);
}




