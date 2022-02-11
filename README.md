# atom-lite-dishwasher-detergent-sensor

Dishwasher Detergent Sensor (made with ATOM Lite and load cell sensor).

## Features / 機能

- Indication of whether detergent has been used / 洗剤を投入したかどうかの表示:

  - Even with liquid detergent, you can see at a glance whether or not you have used it. / 液体洗剤であっても、食洗機に洗剤を投入したかどうかがひと目でわかります。

- Integration with IFTTT / IFTTT 連携:

  - For example, you can make a reminder when the detergent is only few left. / 例えば、残量が少ないときにリマインダーを作成できます。

- Integration with Home Assistant / Home Assistant 連携:

  - You can see the remaining amount of detergent (weight of bottle) from your Home Assistant. / Home Assistant から洗剤の残量 (洗剤ボトルの重量) を確認できます。

## Equipment to be used / 使用機材

- ATOM Lite (or compatible board)

  - e.g. https://www.switch-science.com/catalog/6262/

- Load cell with HX711 A/D converter

  - e.g. https://www.amazon.co.jp/gp/product/B07JL7NP3F/

## About for Status LED / ステータス LED について

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

NOTE: After using the detergent, it will turn off after 1.5 hours. / 洗剤を使用したあと、1.5 時間経過すると、消灯に戻ります。

## Dependencies / 依存ライブラリ

- [M5Atom](https://github.com/m5stack/M5Atom)
- [ArduinoJSON](https://arduinojson.org/)
- [WiFiManager](https://github.com/tzapu/WiFiManager) 
- [HX711 Arduino Library](https://github.com/bogde/HX711)
