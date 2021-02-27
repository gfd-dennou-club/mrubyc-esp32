
2020年度「田んぼ水入れモニタリングシステム」で作成したプログラム
LoRa (ES920) を用いたマルチホップ通信用のプログラム群

* 以下のプログラムは修正の上で iotex-esp32-mrubyc に登録済. 
  * main/mrbc_esp32_uart.c
  * main/mrbc_esp32_uart.c
  * mrbc/model/uart.rb
  * mrbc/model/master.rb (send)

* 未整理なプログラムを本ディレクトリに入れている.
  * master.rb.es920-send
    * データ送信用. 本家に取り込み済み. 
  * master.rb.es920-receive
    * 電波強度チェクのためのプログラム
  * master.rb.es920-relay, slave.rb.es920-relay
    * ESP920 の中継ノードのためのプログラム
  * gateway_rasberrypi.rb
    * ゲートウェイのプログラム. ラズパイなどの Linux OS 用. 