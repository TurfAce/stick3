# StickS3 → Insta360 X4 Air BLEリモコン

M5Stack **StickS3（K150 / ESP32-S3）**用のArduino / PlatformIOプロジェクトです。
「M5StickC S3」はStickS3を指すものとして作成しています。ESP32搭載のStickC / Plus / Plus2やCoreS3には、この設定で書き込まないでください。

**初版はユーザー環境でX4 Airへの接続・操作に成功しています。ちらつき対策・省電力版もESP32向けビルド済みですが、更新後の表示・操作・電池持ちは実機確認待ちです。**
公開されているX3向けBLE実装とX4向け実装例を参考に、GPSリモコンのGATTサービスとボタン通知を実装しています。
X4 Airは公式GPS Action Remoteに対応していますが、この自作実装との互換性は実機確認が必要です。

## 操作

| 操作 | 送信するコマンド |
|---|---|
| 正面Aボタンを押す | シャッター。動画モードでは録画開始／停止、写真モードでは撮影 |
| 側面Bボタンを短く押して離す | 撮影モードの切替 |
| Bボタンを1秒長押し | カメラ画面の切替。実際の動作はファームウェア依存 |

Aは長押ししても1回だけ送信します。Bの長押し後にはモード切替を送りません。
同時押し、起動・再接続時に押しっぱなしのボタンは、両方を離すまで無視します。
送信間隔は最低400msです。接続前・準備前の操作は破棄し、あとから再送しません。
起動時に自動で撮影する処理はありません。

電源ON／OFF・Bluetooth起動、GPS送信、プレビュー、画質変更は実装していません。
カメラの電源は手動で入れてください。

## 書き込み：PlatformIO

1. VS CodeにPlatformIO IDE拡張機能をインストールします。
2. このREADMEと `platformio.ini` がある **プロジェクトのルートフォルダ** を開きます。
3. StickS3をデータ通信対応のUSBケーブルで接続します。
4. PlatformIOの **Build** を実行します。書き込む前に側面のリセット／電源ボタンを約2秒長押しし、内部の緑LEDが点滅したら離して書き込みモードに入れます。その後 **Upload** を実行します。
5. **Serial Monitor** を開きます。速度は115200です。

PlatformIO Coreが利用できる場合は、同じフォルダで実行できます。

```sh
pio run -e sticks3
pio run -e sticks3 -t upload
pio device monitor -b 115200
```

初回はツールチェーンとライブラリのダウンロードにインターネット接続が必要です。
Espressif32 6.12.0（Arduino-ESP32 2.0.17）、M5Unified 0.2.29、M5GFX 0.2.36に固定しています。
BLEはArduino-ESP32同梱版を使用します。別の「ESP32 BLE Arduino」ライブラリを追加しないでください。

## 書き込み：Arduino IDE

1. ボードマネージャで **esp32 by Espressif Systems 2.0.17** をインストールします。
   追加のボードマネージャURL：
   [公式パッケージインデックス](https://espressif.github.io/arduino-esp32/package_esp32_index.json)
2. ライブラリマネージャで **M5Unified 0.2.29** と **M5GFX 0.2.36** をインストールします。
   一覧にない場合は、末尾の公式GitHubリリースからZIPを取得して追加します。
3. 内側の `Insta360Remote/Insta360Remote.ino` を開きます。
   同じフォルダの `RemoteApp.cpp`、`RemoteLogic.h`、`PowerPolicy.h`、`Config.h` も保持してください。
   実装本体の `RemoteApp.cpp` はArduino IDEで自動的にコンパイルされます。
4. ボードとツール設定を以下に合わせます。

| 項目 | 設定 |
|---|---|
| Board | ESP32S3 Dev Module |
| USB CDC On Boot | Enabled |
| USB Mode | Hardware CDC and JTAG |
| Flash Size | 8MB (64Mb) |
| PSRAM | OPI PSRAM |
| Partition Scheme | 8M with spiffs（3MB程度のAPP領域がある設定） |
| Port | 接続したStickS3のポート |

5. 検証してから書き込みます。シリアルモニタは115200に設定します。

## X4 Airとの初回接続

1. カメラの初期アクティベーションを済ませ、電源を入れます。
2. StickS3を起動します。画面に `PAIR ON CAMERA` と表示されます。
3. **カメラ側**の設定／ショートカットから、Bluetoothのリモコン検索画面を開きます。
   メニュー名は言語・ファームウェアによって異なります。
4. `Insta360 GPS Remote` を選択し、カメラ側で接続を確定します。
5. StickS3に `READY` が表示されたら、カメラを動画モードにしてAを1回押します。
   **カメラ本体で録画開始を確認**し、もう一度Aを押して停止を確認します。
6. Bの短押しでモード切替も確認します。

スマートフォンのBluetooth設定からペアリングする方式ではありません。
StickS3がBLEサーバーになり、カメラが接続してきます。
切断すると約300ms後に広告を再開します。カメラが再接続しなければカメラ側から選び直してください。

## 画面のちらつき対策

初版では受信カウンタが増えるたびにLCD全体を黒く消して描き直していました。
対策版では、画面をメモリ上のキャンバスで完成させてからLCDへ転送します。
受信カウンタだけの画面更新は最大毎秒1回に制限し、ボタンと接続状態の表示は従来どおり最大約100ms間隔で更新します。
キャンバスは16bitカラーで約65KBを使い、PSRAMで確保できない場合は内部RAMを試します。
どちらも確保できない場合は直接描画へ戻し、受信だけを理由にした再描画を停止します。

同じフォルダをVS Codeで開いている場合は、そのまま再度Uploadしてください。
別フォルダにコピーしたプロジェクトでは、最新版ZIP内の内側の `Insta360Remote` フォルダのファイル一式を上書きしてからUploadします。省電力版は `RemoteApp.cpp` だけでなく、`Config.h` と新しい `PowerPolicy.h` も必要です。

## 省電力と使い勝手

- 無操作6秒で画面を減光し、10秒でバックライトを消します。
- ボタンを押すと通常の明るさに戻ります。**最初のAボタンから撮影コマンドを送信**します。
  Bの短押し・長押しも従来と同じ操作です。点灯のための追加の1回は不要です。
- BLE接続と5ms間隔のボタン確認を継続します。ディープスリープは使用しません。
- カメラからの定期受信では点灯タイマーを延長しません。接続・通知購読状態が変わった場合は点灯します。
- 消灯中は画面の再描画・LCDへの転送を停止し、復帰時に最新情報を表示します。
- CPUを起動時に240MHzから160MHzへ下げます。設定に失敗した場合は元のクロックのまま動作します。
- USBシリアルモニタが開かれていない間は、受信デバッグデータのコピーとキュー投入を省略します。
  RXカウンタ自体は継続します。

変更できる値は `Config.h` にまとめています。

| 設定 | 初期値 | 意味 |
|---|---|---|
| REMOTE_BRIGHTNESS | 100 | 操作中の明るさ（1〜255） |
| REMOTE_DIM_BRIGHTNESS | 15 | 減光時の明るさ（1〜通常時の明るさ） |
| REMOTE_SCREEN_DIM_AFTER_MS | 6000UL | 減光までの時間。0で減光を無効化 |
| REMOTE_SCREEN_OFF_AFTER_MS | 10000UL | 消灯までの時間。0で自動消灯を無効化 |
| REMOTE_CPU_MHZ | 160 | CPU周波数。240で従来の設定に戻せます |

バックライト消灯は本体の電源OFFではありません。バッテリー駆動のまま接続待ち・接続維持を続けます。
現在のArduino-ESP32同梱SDKではBluetoothの自動省電力と動的なCPU省電力が有効になっていないため、
BLEの接続間隔・送信出力・広告間隔は変更していません。今回の対策は画面とCPU、不要なデバッグ処理の削減です。

**何時間延びるかは未測定です。** 満充電からUSBを外し、10秒後に消灯すること、
Aの最初の1回で撮影できること、Bの操作と再接続が正常なことを確認してください。
電池持ちは同じ使用条件で旧版と比較すると確認できます。

## 表示とログ

| 表示 | 意味 |
|---|---|
| PAIR ON CAMERA | 接続待ち |
| CONNECTING | BLE接続済み。カメラからの通知購読待ち |
| READY | 接続済みでCE82通知の購読が有効 |
| SHUTTER / MODE / SCREEN sent | ローカルBLEスタックが通知を受け付けた |
| TX | ローカルBLEスタックが受け付けた送信の累計 |
| RX | カメラからCE81への書き込みの累計 |

**READYやsentは、撮影成功を確認した表示ではありません。** 録画状態の解析は未実装です。
カメラ本体やアプリからの操作でも状態が変わるため、録画中表示をボタン回数で推測しません。
開始／停止は同じトグルコマンドなので、自動再送もしません。TX/RXは起動後の累計です。

シリアルには `LINK UP SUBSCRIBE ON`、`TX status=... bytes=...`、
`RX ce81 len=...` などを出します。RXは先頭64バイトまで記録します。
大量受信時にはログだけを破棄し、`LOG dropped=...` で通知します。
受信データにはカメラの識別情報が含まれる場合があります。

## 接続できない場合

- 検索に出ない：カメラの「Bluetoothリモコン」の画面で再検索し、両方を再起動します。
- それでも出ない：`Config.h` の `REMOTE_ORBIT_ADVERTISING` を **0** に変えて再ビルド・再書き込みします。
  初期値1はX4向け公開実装のORBIT識別広告とCE80／名称のスキャン応答を使います。
  0ではCE80と補助サービスのUUIDを広告します。どちらも電源ON用ではありません。
- 他のリモコンに接続される：カメラに登録済みのリモコンを切断・解除してから接続します。
- CONNECTINGのまま：カメラ側の接続確定を確認します。通知購読が来なければ互換性の追加調査が必要です。
- READYだが操作できない：カメラのモード／メモリカード／エラー表示を確認します。
  X4 Airのファームウェア番号と接続開始からのシリアルログが追加調査に役立ちます。
- 画面やボタンが動かない：実機がStickS3 K150か、ライブラリとボード設定が上記どおりか確認します。

## 検証状況

- ホスト側C++テスト：**合格**。コマンドバイト列、広告長・ADフィールド、短押し／長押し、
  同時押し抑止、再接続時の押しっぱなし抑止、送信間隔、millisの桁あふれを確認。
- ESP32向けコンパイル：**ちらつき対策・省電力版で成功**。RAM静的使用48,472 bytes、Flash使用1,153,421 bytes。キャンバスの動的確保分はRAM静的使用量に含みません。
- 初版のStickS3への書き込み・X4 Air接続・操作：**ユーザーから成功報告あり**。
- 省電力ポリシーのC++テスト：**合格**。減光・消灯境界、最初のボタンでの撮影、Bの操作、接続変化時の復帰、タイマー無効化、時刻の桁あふれを確認。
- 対策版の再書き込み／ちらつき改善・反応・電池持ち：**実機確認待ち**。

ホストテストの再実行（C++コンパイラのあるPC）：

```sh
c++ -std=c++11 -Wall -Wextra -Werror tests/test_logic.cpp -o /tmp/insta360-remote-test
/tmp/insta360-remote-test
c++ -std=c++11 -Wall -Wextra -Werror tests/test_power.cpp -o /tmp/insta360-power-test
/tmp/insta360-power-test
```

これは実機向けビルドや無線での動作試験の代わりにはなりません。

## 参照

- [StickS3公式仕様・PlatformIO設定](https://docs.m5stack.com/en/core/StickS3)
- [X4 Air公式Bluetooth対応表](https://onlinemanual.insta360.com/x4air/en-us/specs/compatibility/bluetooth)
- [X3 / RS 1-inch向けBLE実装・Patrick Chwalek](https://github.com/pchwalek/insta360_ble_esp32)
- [X4 / ESP32-S3向けESPHome実装例・btittelbach](https://github.com/btittelbach/esphome_config_examples/blob/main/insta360_ble_remote_waveshare_touch169_esp32s3.yaml)
- [ESP32-S3省電力機能の構成条件](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32s3/api-reference/system/power_management.html)
- [M5Unified 0.2.29](https://github.com/m5stack/M5Unified/releases/tag/0.2.29)
- [M5GFX 0.2.36](https://github.com/m5stack/M5GFX/releases/tag/0.2.36)

非公式の自作リモコンです。Insta360公式の製品・SDKではありません。
