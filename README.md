# ESP-WROOM-02
ESP-WROOM-02関連の基板設計とソフトウェアをまとめておきます。

サポートページはこちら：http://jiwashin.blogspot.com/p/esp-wroom-02-board1.html

# Eagle下のプロダクツ

## Board1
ESP-WROOM-02と安定化電源を搭載した標準基板です。「さくっと使えるESP-WROOM-02基板」がコンセプトです。

## Board1 DustCounter
Board1をベースにして、ダストカウンターPPD42とOLED（I2c, 0.96インチのあれ）を簡単に接続して使えるようにしました。
https://gist.github.com/TareObjects/b11eb0981f3f318fd35e

# Arduino下のプロジェクト

## ESP_NOW_ADC / ESP_NOW_DAC / ESP_UDP_ADC / ESP_UDP_DAC
WiFiワイアレスオーディオをESPで廉価に実現しようとしたもののADC/DACとのやりとりがNOW/UDPの処理に邪魔されてしまい信号波形が崩れてしまい、挫折。ADCはMCP3204、DACはMCP4922を使っていますので、そっち方面の参考にしていただければ幸いです。
