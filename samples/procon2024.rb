puts "*********************"
puts " UART check (RS485)  "
puts "*********************"
#
# センサーの説明
# https://wiki.dfrobot.com/RS485_Soil_Sensor_Temperature_Humidity_EC_PH_SKU_SEN0604
#
# 注意：
# センサーの電源を入れて 1 分くらいおかないと値がまともにならない．
# 毎正時に測定・データ送信し，測定しない時間帯はディープスリープ & センサーの電源を切る
# 
def crc16_2(buf)
  crc = 0xFFFF
  11.times do |i|
    byte = buf[i]
    crc ^= byte
    8.times do
      if (crc & 0x0001) != 0
        crc >>= 1
        crc ^= 0xA001
      else
        crc >>= 1
      end
    end
  end
  crc = ((crc & 0x00FF) << 8) | ((crc & 0xFF00) >> 8)
  return crc
end

#電源入れる
pin_5vEN = 12
pin_5vEN = GPIO.new( 12, GPIO::OUT )
pin_5vEN.write( 1 )

#I2C 初期化
i2c = I2C.new()

## RTC 初期化. 時刻設定
rtc = RX8035SA.new(i2c)

# WiFi
wlan = WLAN.new('STA')

# connect
wlan.connect("", "")  #SSID, パスフレーズ (適切に書き換える必要あり)

# SNTP
time = SNTP.new()

# RTC に初期値書き込み
rtc.write([time.year, time.mon, time.mday, time.wday, time.hour, time.min, time.sec]) #年(下2桁), 月, 日, 曜日, 時, 分, 秒

# URLの作成
server = "http://procon2024.epi.it.matsue-ct.ac.jp"   #ダミー (適切に書き換える必要あり)

# UART 初期化 txPin = 17, rxPin = 16 のため uart_num = 2 とする
rs485 = UART.new(2, baudrate:9600, rts_pin:18)

sleep 60 

loop do
    rtc.read  #時刻読み込み
    puts "現在時刻: #{rtc.str_datetime}"

    if( rtc.min == 0 && rtc.sec == 0)  #毎正時に実行
#    if( rtc.min % 10 == 0 && rtc.sec == 0)      

        puts "データ計測・送信 開始"

        hum = 0.0
        tem = 0.0
        ec  = 0.0
        ph  = 0.0
        num = 0.0

        5.times do |j|
          
          # コマンド送信
          rs485.write( [0x01, 0x03, 0x00, 0x00, 0x00, 0x04, 0x44, 0x09] )
        
          # データ取得．13 バイト分．
          data = rs485.read( 13 ).bytes
        
          # 数値に変換 (含む，エラーチェック)
          if (data[0] == 1 && data[1] == 3 && data[2] == 8)
            if ( crc16_2( data ) == ( data[11] * 256 + data[12] ) )
              hum += (data[3] * 256 + data[4]) / 10.00
              tem += (data[5] * 256 + data[6]) / 10.00
              ec  += (data[7] * 256 + data[8])
              ph  += (data[9] * 256 + data[10]) /10.00
              num += 1.0
            end
          end
          
          #clear buffer
          rs485.clear_tx_buffer
          rs485.clear_rx_buffer
        end
        
        # 送信 (URL はダミー)
        url = sprintf("/input.php?titen=ysg1&jikan=%s&temp=%.1f&electric=%.1f&water=%.1f&ph=%.1f", rtc.str_datetime, tem / num, ec / num, hum / num, ph / num)
        puts url
        HTTP.get("#{server}#{url}")

        # 終了処理
        puts "sensor shut down"
        sleep 1
        pin_5vEN.write( 0 )
        puts "start deepSleep"
        sleep 1
        SLEEP.deep( 3420 )     #57min
#        SLEEP.deep( 420 )     #7min
    end
    
    #wait
    sleep 0.5
end
