# coding: utf-8
#概要 summary

led = [
  GPIO.new( 13, GPIO::OUT ), 
  GPIO.new( 12, GPIO::OUT ),
  GPIO.new( 14, GPIO::OUT ),
  GPIO.new( 27, GPIO::OUT ),
  Pin.new( 26, Pin::OUT ),
  Pin.new( 25, Pin::OUT ),
  Pin.new( 33, Pin::OUT ),
  Pin.new( 32, Pin::OUT )
]
sw = [
  GPIO.new( 34, GPIO::IN, GPIO::PULL_UP ),
  GPIO.new( 35, GPIO::IN, GPIO::PULL_UP ),
  GPIO.new(  18, Pin::IN,  Pin::PULL_UP ),
  Pin.new(  19, Pin::IN,  Pin::PULL_UP )
]

puts "**************************"
puts "  GPIO Check (10 times) "
puts "**************************"
puts "LEDs are ON if sw[0].read == 1 or sw[1].high? or sw[2].low? or sw[3].value == 1"


5.times do |i|
  if sw[0].read == 1 or sw[1].high? or sw[2].low? or sw[3].value == 1
    (0..3).each do |k|
      led[k].write( i % 2)
    end
    (4..7).each do |k|
      led[k].on
    end
  else
    (4..7).each do |k|
      led[k].off
    end
  end

  sleep 1
end

(0..3).each do |k|
  led[k].write( 0 )
end  
(4..7).each do |k|
  led[k].off
end

puts "*********************"
puts " PWM check (LED)     "
puts "*********************"

# 初期化. timer (< 4) と channel は指定しないと 0 になる．
# デフォルトの周波数は 440
led2 = [
  PWM.new( 13 ), 
  PWM.new( 12 ), 
  PWM.new( 14 ), 
  PWM.new( 27 ), 
  PWM.new( 26, channel:1, timer:1 ), 
  PWM.new( 25, channel:2, timer:1 ), 
  PWM.new( 33, channel:3, timer:1 ), 
  PWM.new( 32, channel:4, timer:1 )
]

# 同じ timer, channel のピンは必ず duty が同じになる．
# 同じ timer でも channel が違えば異なる duty を設定可能
num = 20
2.times do |i|
  for i in 0..num do
    duty_percent = ( ( 1023.0 / num ) * i / 1024.0 * 100 ).to_i # 0 < duty_percent < 100 
    
    (3..6).each do |k|     
      led2[k].duty( duty_percent )
    end
    sleep 0.1
  end

  sleep 1
end

led2.each do |pwm|
  pwm.duty( 0 )
end  


puts "*************************************"
puts " PWM check (with pluse_width_us)     "
puts "*************************************"

num = 20
2.times do |i|
  for i in 0..num do
    duty = ( 1023.0 / num ) * i / 1024.0 # 0 < duty < 1 
    us = (( 1.0 / 440.0 ) * duty * 1000000).to_i   # マイクロ秒
#    p "us = #{us} us (#{1.0 / 440.0 * 1000000} us), duty = #{duty}"
    
    (3..7).each do |k|     
      led2[k].pulse_width_us( us )
    end
    sleep 0.1
  end

  sleep 1
end

led2.each do |pwm|
  pwm.duty( 0 )
end  


puts "*************************************"
puts " PWM check (buzzer)                  "
puts "*************************************"

pwm0 = PWM.new(15)
pwm0.duty( 50 )    # duty 比 50%
C = 261
D = 293
E = 329
G = 391
mer = [E, G, G, E, D, C, D, E, G, E, D]
len = [0.6,0.2,0.8, 0.6,0.2,0.8, 0.6,0.2,0.6,0.2,1.2]

for i in 0..10 do
  pwm0.frequency(mer[i])  #周波数 (Hz) を指定
  sleep len[i]
end


for i in 0..10 do
  pwm0.period_us( (1.0 / mer[i] * 1000000).to_i )  #周期 (us) を指定
  sleep len[i]
end

pwm0.duty( 0 )

#  mrbc_define_method(0, pwm, "period_us",      mrbc_esp32_ledc_period_us);
#  mrbc_define_method(0, pwm, "pulse_width_us", mrbc_esp32_ledc_pulse_width_us);


puts "*********************"
puts " ADC check (Temp)    "
puts "*********************"

# A/D 変換 初期化
adc = ADC.new( 39 )

#温度計測用変数初期化
B = 3435.0
To = 25.0
V = 3300.0
Rref = 10.0

5.times do |i|
  rawdata = adc.read_raw
  p rawdata

  voltage = adc.read()
  temp = 1.0 / ( 1.0 / B * Math.log( (V - voltage) / (voltage/ Rref) / Rref) + 1.0 / (To + 273.0) ) - 273.0
  puts "#{voltage} mV, #{temp} K"
  sleep(2)
end

puts "*********************"
puts " I2C check (LCD, RTC)    "
puts "*********************"


#I2C 初期化
i2c = I2C.new()

# LCD 初期化
lcd = AQM0802A.new(i2c)

# RTC 初期化. 時刻設定
rtc = RX8035SA.new(i2c)

# RTC に初期値書き込み
rtc.write([20, 3, 31, 1, 23, 59, 50]) #年(下2桁), 月, 日, 曜日, 時, 分, 秒

# LCD に "Hello World" 表示
lcd.cursor(0, 0)
lcd.write_string("Hello!")
lcd.cursor(0, 1)
lcd.write_string("from ESP")

sleep(10)

#a = i2c.readfrom(0x32, 8)   # 8 バイト分読み込み


# 適当な時間を表示
while true
  tt = rtc.read
  t0 = sprintf("%02d-%02d-%02d", tt[0], tt[1], tt[2])
  t1 = sprintf("%02d:%02d:%02d", tt[4], tt[5], tt[6])

  lcd.clear
  lcd.cursor(0, 0)
  lcd.write_string( t1 )
  lcd.cursor(0, 1)
  lcd.write_string( t0 )

  puts sprintf("#{t0} #{t1}")
  sleep 1.0
end


=begin
# GPSの電源を入れる (高専ボードの場合に必要)
gps_pw = GPIO.new(5, GPIO::OUT)
gps_pw.write(0)

#p "UART.new"
# GPS初期化 txPin = 17, rxPin = 16 のため uart_num = 2 とする
gps = UART.new(2, baudrate:9600)

#p "UART.write"
# 出力を RMS のみに
sleep 1
gps.write("$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n")

## 出力をデフォルトに戻すときは以下をコメントアウトすること．
## sleep 1
##gps.write("$PMTK314,-1*04\r\n")

# データの到着まで少し待つ
sleep 2

# read メソッドで 100 バイト分のデータを受け取る
# 指定したバイト数分のデータを受け取れない場合は nill が返る
puts "> gps.read"
p gps.read(100)

# 入力データをclear_tx_bufferで消去する
puts "> gps.clear_tx_buffer"
gps.clear_tx_buffer

# データの到着まで少し待つ
sleep 2

# 4096バイトのデータはないが、
# nonblockのため到着している分のデータが表示される
puts "> gps.read_nonblock(4096)"
puts gps.read(4096, nonblock:1)

# 入力データをclear_tx_bufferで消去する
puts "> gps.clear_tx_buffer"
gps.clear_tx_buffer

# こちらは消去されているため何も表示されない
puts "> gps.read(4096, nonblock:1)"
puts gps.read(4096, nonblock:1)

# 以下、到着したデータを 1 行ずつ読み込んで表示
puts "> gps.gets"
while true
  puts  gps.gets()
  sleep 1
end
=end

#spi = SPI.new(miso:23, mosi:19, clk:18, cs:4)

#spi = SPI.new( miso:23, mosi:23, clk:18, cs:14 )
#display = ILI934X.new( spi, dc:27, rst:33, bl:32 )
#display.line(20, 50, 240, 230, [0, 0, 255])

#display.draw_fillrectangle(20, 20,  300, 170, [0x69, 0xba, 0xf5])
#display.draw_fillrectangle(20, 170, 300, 180, [0x28, 0xad, 0x35])
#display.draw_fillrectangle(20, 180, 300, 220, [0x55, 0x42, 0x3d])
#display.drawString(52, 52, "mruby/cの\n  世界へ\n      ようこそ！", 30, [0, 0, 0])
#display.drawString(50, 50, "mruby/cの\n  世界へ\n      ようこそ！", 30, [255, 255, 255])
=begin
sleep 10
i = 0
while true do
  display.fill([i, i, i])
  5.times do |x|
    5.times do |y|
      display.draw_pixel(10 + x, 10 + y, [255, 0, 0])
    end
  end
  display.draw_line(20, 50, 240, 230, [0, 0, 255])
  display.draw_rectangle(30, 20, 310, 200, [0, 255, 0])
  display.draw_circle(160, 120, 20, [100, 50, 220])
  display.draw_fillcircle(160, 120, 10, [100, 50, 220])
  i += 1
  p "light: #{i}" if i % 10 == 0
  i = 0 if i == 256
end
=end
=begin
# coding: utf-8
wlan = WLAN.new()
puts "Wlan.Active"

# connection
while true
  # ネットワークパラメタを定期的に表示
  #  puts wlan.ifconfig
  #puts wlan.ip
  puts wlan.mac

  # WiFiの接続が切れたときに自動的に際接続する
  if !wlan.connected?
    puts 'start reconnect....'
    wlan.connect("SugiyamaLab", "matsue-ct.ac.jp")
    while !wlan.connected?
      sleep 1
    end
  end
  sleep 1
end
=end
=begin
wlan = WLAN.new
puts "scan result: #{wlan.scan}"

wlan.connect("SugiyamaLab", "matsue-ct.ac.jp")
puts "connected: #{wlan.connected?}"
puts "ifconfig: #{wlan.ifconfig}"
puts "mac: #{wlan.mac}"
puts "ip: #{wlan.ip}"

# Web ページ表示
#HTTP.get("https://www.gfd-dennou.org/")

# SNTP
time = SNTP.new()

# 表示
puts "現在時刻:"
puts sprintf("%04d-%02d-%02d %02d:%02d:%02d", time.year, time.mon, time.mday, time.hour, time.min, time.sec)
=end
