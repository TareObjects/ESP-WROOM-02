//  ESP_BME280_To_Ambient by k.kurahashi 2016-06-17
//
//  thanks:
//    BME280 Library
//    https://learn.sparkfun.com/tutorials/sparkfun-bme280-breakout-hookup-guide

//    OLED Library
//    https://github.com/squix78/esp8266-oled-ssd1306
//
//    Ambient data visualize service from Japan
//    https://Ambient.io
//
//    RTC memory storage
//    https://lowreal.net/2016/01/10/1
//
//    Arduino CRC-32
//    http://excamera.com/sphinx/article-crc.html
//
//    JSON
//    https://github.com/bblanchon/ArduinoJson
//
//    秋月のリアルタイムクロック（ＲＴＣ）モジュール
//    http://s2jp.com/2015/03/rtc-module/
//

#define kInterval 60


extern "C" {
#include "user_interface.h"
#include "sntp.h"
#include "mem.h"
}

#include <SPI.h>
#include <Wire.h>
#include <Time.h>
#include <ArduinoJson.h>
#include <Ambient.h>


#include "SSD1306.h"
#include "SSD1306Ui.h"

#include <ESP8266WiFi.h>
#include <WiFiClient.h>


//  OLED / SSD1306
//
SSD1306   display(0x3c, 4, 5);


//
// Wifi
//
#include "../../private_ssid.h"
//const char *ssid     = "*************";
//const char *password = "*************";
//const char *ambient_write_key_ESP_BME280_To_Ambient = "****************";

WiFiClient client;

bool startWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin ( ssid, password );
    Serial.println("Started");
  } else {
    Serial.println("wifi already connected");
    return true;
  }

  // Wait for connection
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }
  Serial.println("Wifi Connected");

  return true;
}

//
//  Ambient
//
Ambient ambient;

void setupAmbient() {
  ambient.begin(143, ambient_write_key_ESP_BME280_To_Ambient, &client);
}


//
//  storage handling
//

#define kOffset_CRC         65
#define kMaxLengthCreated   24

#pragma pack(push)
#pragma pack(4)
struct OneData_t {
  float temperature;
  float humidity;
  float pressure;
  uint16_t airQuality;
  time_t created;
};

#define kMaxDataArea  (512 - 4 - sizeof(int) - sizeof(time_t))
#define kMaxBlocks    ((kMaxDataArea - sizeof(int)) / sizeof(OneData_t))
#define kMaxSend      24


typedef struct {
  int    counter;
  time_t nextAdjust;
  struct OneData_t blocks[kMaxBlocks];
} Datas;


typedef struct {
  int crc;
  union Uni {
    Datas datas;
    char buffer[kMaxDataArea];
  } uni;
} Storage;
#pragma pack(pop)


Storage storage;

//
//  Storageを読み込む。もしCRCが不一致なら初期化
//
bool readStorageAndInitIfNeeded() {
  Serial.print("storage size = ");
  Serial.println(sizeof(storage));
  bool ok = system_rtc_mem_read(kOffset_CRC, &storage, sizeof(storage));
  if (!ok) {
    Serial.println("readStorageAndInitIfNeeded : mem_read fail");
    return ok;
  }

  int crc = calcCRC();
  Serial.print("crc = ");
  Serial.print(crc, HEX);
  Serial.print(", stored crc = ");
  Serial.println(storage.crc, HEX);
  if (crc != storage.crc) {
    Serial.println("crc mismatch");
    memset(storage.uni.buffer, 0, sizeof(storage.uni.buffer));
    storage.uni.datas.counter    = 0;
    storage.uni.datas.nextAdjust = 0;
    crc = calcCRC();
    Serial.print("new crc = ");
    Serial.println(crc, HEX);
    storage.crc = crc;
    ok = system_rtc_mem_write(kOffset_CRC, &storage, sizeof(storage));
    if (!ok) {
      Serial.println("readStorageAndInitIfNeeded : write fail");
    }
  }

  Serial.print("read counter = ");
  Serial.println(storage.uni.datas.counter);

  return ok;
}


bool writeStorage() {
  storage.crc = calcCRC();
  bool ok = system_rtc_mem_write(kOffset_CRC, &storage, sizeof(storage));
  if (!ok) {
    Serial.println("Error : writeStorage : system_rtc_mem_write fail");
  }

  return ok;
}


//
//  MBE280
//
#include "SparkFunBME280.h"

BME280 bme;


void setup_BME280() {
  uint8_t chipID;

  bme.settings.commInterface = I2C_MODE;
  bme.settings.I2CAddress = 0x76;

  bme.settings.runMode = 3; //Forced mode
  bme.settings.tStandby = 0;  //  0.5mS
  bme.settings.filter = 0;
  bme.settings.tempOverSample  = 2;
  bme.settings.pressOverSample = 2;
  bme.settings.humidOverSample = 2;

  delay(10);
}


//
//  RX8025
#include "RX8025.h"

RX8025 rtc;


//
//  setup
//
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.print("Max Blocks = ");
  Serial.println(kMaxBlocks);

  // Wireは複数デバイス共通なので最初に
  Wire.begin();

  //  初めての実行ならバッファを初期化する
  //
  readStorageAndInitIfNeeded();

  //  RTCが初期化されていないならWifiに接続してsntpで時刻を取得
  rtc.setup(0x20);
  if (rtc.isPowerOn() || storage.uni.datas.nextAdjust == 0) {
    Serial.println("RTC is not initilized");
    startWifi();
    adjustRTCwithSNTP();
  } else {
    Serial.println("RTC is already initalized");
  }


  //  BMEとOLEDを初期化
  //
  setup_BME280();

  display.init();
  display.flipScreenVertically();
  display.displayOn();
  display.clear();

  time_t timestamp;
  rtc.readRTC(&timestamp);

  Serial.print("timestamp = ");
  Serial.println(timestamp);

  //  BME読み込み
  //
  float temperature, humidity, pressure;
  char buffer[80];

  bme.begin();
  delay(100);
  temperature = bme.readTempC();
  humidity    = bme.readFloatHumidity();
  pressure    = bme.readFloatPressure() / 100;

  //  Air Quality
  int airQuality = system_adc_read();

  Serial.print("temperature = ");
  Serial.print(temperature);
  Serial.print(", humidity = ");
  Serial.print(humidity);
  Serial.print(", pressure = ");
  Serial.print(pressure);
  Serial.print(", air = ");
  Serial.println(airQuality);

  int n = storage.uni.datas.counter;

  if (n >= kMaxBlocks) {
    Serial.println("Buffer overflow : dispose last data");
    storage.uni.datas.counter = n = n - 1;
  }

  Serial.print("counter = ");
  Serial.println(n);

  OneData_t *pData = &storage.uni.datas.blocks[n];
  pData->temperature = temperature;
  pData->humidity    = humidity;
  pData->pressure    = pressure;
  pData->airQuality  = airQuality;
  pData->created     = timestamp;

  storage.uni.datas.counter = ++n;

  int nMaxSend = kMaxBlocks > kMaxSend ? kMaxSend : kMaxBlocks;
  
  if (n >= nMaxSend) {
    startWifi();

    setupAmbient();
    if (sendStorageToAmbient()) {
      Serial.println("sendStorageToAmbient : OK");
      storage.uni.datas.counter = n = 0;
    } else  {
      Serial.println("sendStorageToAmbient : NG");
    }


    time_t next = storage.uni.datas.nextAdjust;
    time_t rtcTime;
    rtc.readRTC(&rtcTime);
    Serial.print("next, rtcTime = ");
    Serial.print(next);
    Serial.print(",");
    Serial.println(rtcTime);

    if (next == 0 || next < rtcTime) {
      adjustRTCwithSNTP();
    }
  }

  writeStorage();

  ESP.deepSleep(kInterval * 1000 * 1000, WAKE_RF_DEFAULT);
  delay(1000);
}


void adjustRTCwithSNTP() {
  Serial.println("getting sntp");
  
  time_t sntpTime = getTimestamp(); //sntp

  time_t rtcTime;
  rtc.readRTC(&rtcTime);
  
  Serial.print("rtc - sntp = ");
  Serial.println(rtcTime - sntpTime);
  
  rtc.writeRTC(&sntpTime);

  Serial.println("new adjust timing has been set");
  storage.uni.datas.nextAdjust = rtcTime + 24 * 3600 * 1000;  // 1day later
}


//
//  loop
//
void loop() {
  delay(1000);
}


//
//  OLEDへの描画と送信
//
void drawAndSend() {
  float temp, humidity,  pressure, pressureMoreAccurate;
  double tempMostAccurate, humidityMostAccurate, pressureMostAccurate;
  char buffer[80];

  //  BME280各データ取り出し
  bme.begin();
  delay(10);
  temp      = bme.readTempC();
  humidity  = bme.readFloatHumidity();
  pressure  = bme.readFloatPressure() / 100.0;
  
  //  OLEDへ表示
  display.clear();

  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString( 0, 16, "Temperature");
  display.drawString( 0, 32, "Humidity");
  display.drawString( 0, 48, "Pressure");

  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_RIGHT);

  dtostrf(tempMostAccurate, 7, 2, buffer);
  display.drawString(127, 12, buffer);

  dtostrf(humidityMostAccurate, 7, 2, buffer);
  display.drawString(127, 28, buffer);

  dtostrf(pressureMostAccurate, 7, 2, buffer);
  display.drawString(127, 44, buffer);

  display.display();
}


//
//  send to Ambient
//

const int kMaxJsonBuffer = kMaxBlocks * 100;

StaticJsonBuffer<kMaxJsonBuffer*2> jsonBuffer;
char jsonPrintBuffer[kMaxJsonBuffer];

bool sendStorageToAmbient() {
  int n = storage.uni.datas.counter;

  char *bufCreated[kMaxBlocks];
  for (int i = 0; i < n; i++) {
    bufCreated[i] = (char *)os_malloc(kMaxLengthCreated);
    if (bufCreated[i] == NULL) {
      Serial.println("alloc fail!");
    }
  }

  JsonObject& root = jsonBuffer.createObject();
  root["writeKey"] = ambient_write_key_ESP_BME280_To_Ambient;    // ambientのapi write key

  JsonArray& dataArray = root.createNestedArray("data");
  for (int i = 0; i < n; i++) {
    OneData_t *pSrc = &storage.uni.datas.blocks[i];
    JsonObject& one = jsonBuffer.createObject();

    time_t timestamp = pSrc->created;
    sprintf(bufCreated[i], "%4d-%02d-%02d %02d:%02d:%02d.000", year(timestamp), month(timestamp), day(timestamp), hour(timestamp), minute(timestamp), second(timestamp));
    Serial.println(bufCreated[i]);

    one["created"] = bufCreated[i];
    one["d1"] = pSrc->temperature;
    one["d2"] = pSrc->humidity;
    one["d3"] = pSrc->pressure / 10.0;
    one["d4"] = pSrc->airQuality;
    dataArray.add(one);
  }

  root["data"] = dataArray;

  Serial.print("text size = ");
  Serial.println(root.measureLength());

  root.printTo(jsonPrintBuffer, sizeof(jsonPrintBuffer));

  Serial.println(jsonPrintBuffer);

  int sent = ambient.bulk_send(jsonPrintBuffer);
  Serial.print("Ambient result = ");
  Serial.println(sent);

  for (int i = 0; i < n; i++) {
    os_free(bufCreated[i]);
  }

  return true;
}



//
//  sntp
//
//
uint32_t getTimestamp() {
  configTime(9 * 3600, 0, "ntp.nict.jp", NULL, NULL);

  uint32_t result = 0;
  int cnt = 0;
  while (result == 0) {
    result = sntp_get_current_timestamp();
    delay(100);
    if (++cnt > 10) {
      break;
    }
  }

  return result;
}




//  crc
//  thanks : https://github.com/vinmenn/Crc16/blob/master/examples/Crc16_Example/Crc16_Example.ino
//  below comment is from original source
//Check routine taken from
//http://web.mit.edu/6.115/www/miscfiles/amulet/amulet-help/xmodem.htm
int calcCRC()
{
  char *ptr = storage.uni.buffer;
  int   count = sizeof(storage.uni.buffer);
  Serial.print("calc crc counter = ");
  Serial.println(count);

  int  crc;
  char i;
  crc = 0;
  while (--count >= 0)
  {
    crc = crc ^ (int) * ptr++ << 8;
    i = 8;
    do
    {
      if (crc & 0x8000)
        crc = crc << 1 ^ 0x1021;
      else
        crc = crc << 1;
    } while (--i);
  }
  return (crc);
}


