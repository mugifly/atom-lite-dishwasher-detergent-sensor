# atom-lite-dishwasher-detergent-sensor

Dishwasher Detergent Sensor (made with ATOM Lite and load cell sensor).

https://youtu.be/9q-ngajI_L4

## Features / 機能

- Indication of whether detergent has been used / 洗剤を投入したかどうかの表示:

  - Even with transparent liquid detergent, you can see at a glance whether or not you have used it. / 透明の液体洗剤であっても、食洗機に洗剤を投入したかどうかがひと目でわかります。

- Integration with IFTTT / IFTTT 連携:

  - For example, you can make a reminder when the detergent is only few left. / 例えば、残量が少ないときにリマインダーを作成できます。

- Integration with Home Assistant / Home Assistant 連携:

  - You can see the remaining amount of detergent (weight of bottle) from your Home Assistant. / Home Assistant から洗剤の残量 (洗剤ボトルの重量) を確認できます。

## Equipment to be used / 使用機材

- ATOM Lite (or compatible board)

  - e.g. https://www.switch-science.com/catalog/6262/

- Load cell with HX711 A/D converter

  - e.g. https://www.amazon.co.jp/gp/product/B07JL7NP3F/

- USB-C to A cable

- USB-AC adapter (with USB-A port)

## Usage / 使用方法

### Setup / セットアップ

1. Connect ATOM Lite and Loadcell with HX711 A/D converter with a cable.<br> / ATOM Lite と Loadcell with HX711 A/D converter をケーブルで接続します。

   - 5V
   - GND
   - DOUT (to G25) / DOUT (G25 ピンへ)
   - SCK (to G21) / SCK (G21 ピンへ)

2. Install [esptool](https://github.com/espressif/esptool/releases) on your PC.<br> / PC に [esptool](https://github.com/espressif/esptool/releases) をインストールします。

3. Download zip file from [Releases](https://github.com/mugifly/atom-lite-dishwasher-detergent-sensor/releases), then unarchive it.<br> / [Releases](https://github.com/mugifly/atom-lite-dishwasher-detergent-sensor/releases) から ZIP ファイルをダウンロードし、バイナリファイルを展開します。

4. Connect ATOM Lite and PC with a USB-C to A cable. Then, Upload the binary files to ATOM Lite, with using Arduino IDE.<br> / ATOM Lite と PC を USB-C to A ケーブルで接続したのち、以下のコマンドを使って、ATOM Lite へバイナリファイルを書き込みます。<br><pre>esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 1500000 --before default_reset \\
   --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect \\
   0xe000 boot_app0.bin \\
   0x1000 bootloader_dio_80m.bin \\
   0x10000 atom_lite_dishwasher_detergent_sensor.ino.bin \\
   0x8000 atom_lite_dishwasher_detergent_sensor.ino.partitions.bin </pre>

5. Unplug the USB-C to A cable from PC.<br> / PC から USB-C to A ケーブルを取り外します。

6. Make sure with nothing on the loadcell (scale), then connect the USB-C to A cable to the USB-AC adapter and wait at least 10 seconds. / ロードセル (はかり) に何も載っていないことを確認してから、USB-C to A ケーブルを USB-AC アダプタへ接続し、10 秒以上待ちます。

7. Follow the steps in "[About for Calibration](#about-calibration)" section to calibrate.<br> / "[較正について](#about-calibration)" の手順に沿って、較正を行います。

8. Place the detergent on the load cell and wait at least 10 seconds. Now you are ready to use it.<br> / ロードセル (はかり) に洗剤のボトルを載せ、10 秒以上待ちます。これで準備は完了です。

9. [ADDITIONAL STEP] If you want to use the integration (IFTTT, Home Assistant), connect to the Wi-Fi access point named `dishwasher-detergent-sensor` on your smartphone and make the initial settings in your browser.<br> / さらに、もし、IFTTT や Home Assistant との連携を行いたい場合は、スマートフォンで `dishwasher-detergent-sensor` という名前の Wi-Fi アクセスポイントへ接続し、ブラウザで初期設定を行ってください。

### Daily Usage / 日常的な使用方法

1. Lift the detergent bottle and pour the detergent into the dishwasher as you normally would.<br> / ロードセル (はかり) に置かれた洗剤のボトルを持ち上げて、通常どおり、食洗機に洗剤を投入します。

2. Again, place the detergent bottle in the load cell (scale).<br> / 再び、洗剤のボトルをロードセル (はかり) に置きます。

3. The Status LED blinks green or yellow. This will tell you at a glance that you have used the detergent. For the meaning of the status LED, please see "[About for Status LED](#about-status-led)" section.<br> / ステータス LED が緑色または黄色に点滅します。これにより、洗剤を使ったことがひと目でわかります。尚、ステータス LED の意味については、"[ステータス LED について](#about-status-led)" をご覧ください。

### About for Calibration / 較正について <a name="about-calibration"></a>

1. Please prepare the following item for calibration.<br> / 較正用に以下のアイテムを用意します。<br>NOTE: If you want to use different weight item, edit the `LOADCELL_CALIBRATION_KNOWN_WEIGHT_GRAM` constant value in the sketch.

   - 100 Yen (JPY) (4.8gram) / 100 円硬貨 (4.8g)

2. With nothing on the loadcell (scale), please press and hold the button of device for about 2 seconds, then release it.<br> / ロードセル (はかり) に何も載せない状態で、デバイスのボタンを約 2 秒間押し続けてから、離します。

3. Make sure the device status LED blinks purple 5 times.<br> / ステータス LED が紫色で 5 回点滅したことを確認します。

4. After a few seconds, when the status LED blinks purple 5 times again, immediately place the item prepared in step 1 on the loadcell (scale).<br> / 数秒後にもう一度、ステータス LED が紫色で 5 回点滅したら、直ちに、手順 1 で用意したアイテムを ロードセル (はかり) に載せてください。

5. Wait until the status LED to flash white. After that, please unload the item placed in the load cell (scale).<br> / ステータス LED が白色で点滅するまで待ちます。その後、ロードセル (はかり) からアイテムを降ろしてください。

### About for Status LED / ステータス LED について <a name="about-status-led"></a>

- Off / 消灯:

  - When detergent is not used. / 洗剤が未使用なとき

- Blinking red / 赤色で点滅:

  - High speed / 高速: &nbsp;&nbsp;&nbsp; When detergent bottle is lifted. / 洗剤ボトルが持ち上げられているとき

- Blinking green / 緑色で点滅:

  - Once / 1 回: &nbsp;&nbsp;&nbsp; When detergent is used. / 洗剤を使用したとき
  - Twice / 2 回: &nbsp;&nbsp;&nbsp; When a lot of detergent is used. / 洗剤を多めに使用したとき

- Blinking yellow / 黄色で点滅:

  - Once / 1 回: &nbsp;&nbsp;&nbsp; When detergent is used, and only few left. / 洗剤を使用したとき (残量少)
  - Twice / 2 回: &nbsp;&nbsp;&nbsp; When a lot of detergent is used, and only few left. / 洗剤を多めに使用したとき (残量少)

NOTE: The amount of detergent used was just a reference. The accuracy depends on your load cell. / 洗剤の使用量はあくまで参考値です。精度はロードセル (はかり)に依存します。

NOTE: After using the detergent, Status LED will turn off after 1.5 hours. / 洗剤を使用したあと、1.5 時間経過すると、ステータス LED は消灯に戻ります。

## Dependencies / 依存ライブラリ

- [M5Atom](https://github.com/m5stack/M5Atom)
- [ArduinoJSON](https://arduinojson.org/)
- [WiFiManager](https://github.com/tzapu/WiFiManager)
- [HX711 Arduino Library](https://github.com/bogde/HX711)
