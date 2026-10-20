本 mruby/c ファームウェア・クラスライブラリ for ESP32 は，ESP-IDF 6.0 で書かれており，mruby/c 4.0 をサポートしています．

クラスライブラリは，[mruby mruby/c 共通 I/O API ガイドライン](https://github.com/mruby/microcontroller-peripheral-interface-guide) に準拠しています．

## 利用方法

詳細は [wiki](https://github.com/gfd-dennou-club/mrubyc-esp32/wiki) をご覧ください．

### ESP32 S3 (例えば，ATOM S3R) を使う場合

現時点では menuconfig で設定を入れる必要があります．

```
idf.py set-target esp32s3

idf.py menuconfig

   (Top) > Serial flasher config
      Flash size (4 MB)

   (Top) > Partition Table
      Partition Table (Custom partition table CSV)
      (partitions.csv) Custom partition table CSV file

   (Top) > mrubyc-esp32 Configuration
      BOARD SELECT (Atom S3 series)

   (Top) > Compiler options
      [*] Disable new warnings introduced in GCC 15

   (Top) > Component config > ESP-STDIO
      Channel for console output (USB Serial/JTAG Controller)

idf.py build

idf.py flash spiffs monitor


## 周辺機器

周辺機器用のクラスライブラリは，ESP32 以外のマイコンからも利用することを想定し，別リポジトリに分けています．
それらのリポジトリを参照してください．なお，これらのクラスライブラリは Ruby のみで書かれており，ESP-IDF (ESP32 マイコンの IDE) には依存しません．

+ GROVE sensors : https://github.com/gfd-dennou-club/mrubyc-gem-grove/wiki

## 利用シーン

+ 松江高専の関係
  + [情報工学科のマイコン/IoT演習](https://www.gfd-dennou.org/arch/iotex/)
  + 小中学生向け出張講座
  + 社会人向け DX 講座 
+ 地域コミュニティにて
  + [Matz葉がにロボコン](https://www.shimane-oss.org/kani-robo/)
  + [ドジョウすくいロボコン](https://www.shimane-oss.org/dojyosukui-robo/)
  + Ruby 合宿 2025年春
