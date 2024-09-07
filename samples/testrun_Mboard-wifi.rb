
#I2C 初期化
i2c = I2C.new()

# LCD 初期化
lcd = AQM0802A.new(i2c)

## RTC 初期化. 時刻設定
rtc = RX8035SA.new(i2c)

# WiFi 初期化
wlan = WLAN.new
puts "scan result: #{wlan.scan}"

wlan.connect("", "")

puts "ifconfig: #{wlan.ifconfig}"
puts "mac: #{wlan.mac}"
puts "ip: #{wlan.ip}"

# Web ページ表示 (HTTPS 対応)
HTTP.get("https://www.gfd-dennou.org/")

# SNTP
time = SNTP.new()

# 表示
puts "現在時刻:"
puts time.datetime
puts time.date
puts time.time

# RTC に初期値書き込み
#年(下2桁), 月, 日, 曜日, 時, 分, 秒
rtc.write([time.year, time.mon, time.mday, time.wday, time.hour, time.min, time.sec])

# LCD に "Hello World" 表示
lcd.cursor(0, 0)
lcd.write_string("Hello?!")
lcd.cursor(0, 1)
lcd.write_string("from ESP")
    
sleep(10)

var = ""
loop do
  tt = rtc.read
  t0 = sprintf("%02d-%02d-%02d", tt[0], tt[1], tt[2])
  t1 = sprintf("%02d:%02d:%02d", tt[4], tt[5], tt[6])
  
  lcd.cursor(0, 0)
  lcd.write_string( t0 )
  lcd.cursor(0, 1)
  lcd.write_string( t1 )

  # 無線 AP への接続の有無で動作を変える
  # ESP32 マイコンでは，無線 AP への再接続は自動的に行われる
  if tt[6] == 0
    # 毎分，時刻を保管
    var = var + "#{t0}T#{t1},"

    if wlan.connected?
      # 無線 LAN に接続している場合は表示
      puts var
      var = ""
    end
  end
  
  sleep 0.6
end


