# coding: utf-8

###
### Hello World
###
=begin
while true
  puts "hello world from ESP32"
  sleep 1
end
=end

###
### GPIO PIN or SWITCH
###

=begin
led1 = GPIO.new( 13, GPIO::OUT )
sw1  = GPIO.new( 34, GPIO::IN, GPIO::PULL_UP )

while true
 if (sw1.read == 1)
    led1.write(1)
  else
    led1.write(0)
  end
  sleep 1
end
=end

=begin
led1 = GPIO.new( 12, GPIO::OUT )
while true
  led1.on
  sleep 1

  led1.off
  sleep 1
end
=end

=begin
led1 = GPIO.new( 13, GPIO::OUT )
while true
  led1.write(1)
  sleep 1

  led1.write(0)
  sleep 1
end
=end

###
### PWM
###

=begin
pwm = PWM.new(13)
pwm.frequency( 1000 )
num = 10

while true
  for i in 0..num do
    duty = ( 1023 / num ) * i
    pwm.duty( duty )
    sleep 10.0 / num
  end
  # 少し休む
  sleep 0.2
end
=end

=begin
pwm0 = PWM.new( 15 )

pwm0.freq(5000)
pwm0.duty(512)
sleep 10
pwm0.deinit
=end

###
### ADC
###

=begin
# A/D 変換 初期化
adc = ADC.new( 39, ADC::ATTEN_11DB, ADC::WIDTH_12BIT )

#温度計測用変数初期化
B = 3435.0
To = 25.0
V = 3300.0
Rref = 10.0

while true
  voltage = adc.read()
  temp = 1.0 / ( 1.0 / B * Math.log( (V - voltage) / (voltage/ Rref) / Rref) + 1.0 / (To + 273.0)) - 273.0
  puts "#{voltage} mV, #{temp} K"
  sleep(10)
end
=end


###
### UART
### 

=begin
# GPS初期化 txPin = 17, rxPin = 16 のため uart_num = 2 とする
gps = UART.new(2, 9600)

# GPSの電源を設定
gps_pw = GPIO.new(5, GPIO::OUT)
gps_pw.write(0)

# データの到着まで少し待つ
sleep 2

# 4096バイトもデータがないため、nilが表示される
puts "> gps.read(4096)"
p gps.read(4096)

# 4096バイトのデータはないが、
# nonblockのため到着している分のデータが表示される
puts "> gps.read_nonblock(4096)"
puts gps.read_nonblock(4096)

# データの到着まで少し待つ
sleep 2

# 入力データをclear_tx_bufferで消去する
puts "> gps.clear_tx_buffer"
gps.clear_tx_buffer
# こちらは消去されているため何も表示されない
puts "> gps.read_nonblock(4096)"
puts gps.read_nonblock(4096)

puts "*---------------------------*"
# データの到着まで少し待つ
sleep 2

while true
    # 以下、1行ずつ読み込んで表示
    puts gps.gets()
    sleep 1
end
=end


###
### I2C
###

=begin
@lcd_address = 0x3e
@rtc_address = 0x32

def lcd_cmd(i2c, cmd)
  i2c.writeto(@lcd_address, [0x00, cmd])
end

def lcd_data(i2c, data)
  i2c.writeto(@lcd_address, [0x40, data])
end

def lcd_clear(i2c)
  lcd_cmd(i2c, 0x01)
  sleep 0.1
end

def lcd_home0(i2c)
  lcd_cmd(i2c, 0x02)
  sleep 0.1
end

def lcd_home1(i2c)
  lcd_cmd(i2c, 0x40|0x80)
  sleep 0.1
end

def lcd_cursor(i2c, x, y)
  pos = (x + y * 0x40) | 0x80
  lcd_cmd(i2c, pos)
  sleep 0.1
end

def lcd_init(i2c)
  sleep 0.2
  [0x38, 0x39, 0x14, 0x70, 0x56, 0x6c].each do |cmd|
    lcd_cmd(i2c, cmd)
  end
  sleep(0.3)
  [0x38, 0x0c, 0x01].each do |cmd|
    lcd_cmd(i2c, cmd)
  end
  sleep(0.1)
end

def lcd_print(i2c, data)
  data.length.times do |n|
    lcd_data(i2c, data[n].ord)
  end
end

def rtc2_init(i2c)
  i2c.writeto(0x32, [0xE0, 0x00, 0x00])
  sleep 0.1
end


def rtc2_set(i2c)
  i2c.writeto(@rtc_address, [0x00, 0x50, 0x59, 0x23|0x80, 0x01, 0x31, 0x10, 0x20])
  sleep 0.1
  i2c.writeto(@rtc_address, [0xF0, 0x00])
  sleep 0.1
end

def rtc2_get(i2c, tt)
  buf = i2c.read(@rtc_address, 8)   # 8 バイト分読み込み --> strings
  p buf
  buf = i2c.readfrom(@rtc_address, 8)   # 8 バイト分読み込み
  p buf

  tt['year'] = buf[7]
  tt['mon']  = buf[6]
  tt['mday'] = buf[5]
  tt['hour'] = buf[3] & 0x3f
  tt['min']  = buf[2]
  tt['sec']  = buf[1]
end

#I2C 初期化
i2c = I2C.new(22, 21)

# LCD 初期化
lcd_init(i2c)

# LCD に "Hello World" 表示
lcd_cursor(i2c, 1, 0)
lcd_print(i2c, "Hello!")

lcd_home1(i2c)
lcd_print(i2c, "from ESP")

# i2c で直接送信．以下のように文字列を送っても同様の操作ができる
# opcode = 0x40.chr
# i2c.write(0x3e, opcode + "from ESP")

sleep(5)

# RTC 初期化. 時刻設定
rtc2_init(i2c)
rtc2_set(i2c)

while true
  # 時刻表示
  tt = {}  #ハッシュ
  rtc2_get(i2c, tt)

  time0 = sprintf("%02x-%02x-%02x", tt['year'], tt['mon'], tt['mday'])
  time1 = sprintf("%02x:%02x:%02x", tt['hour'], tt['min'], tt['sec'])

  lcd_home0(i2c)
  lcd_print(i2c, time0)
  puts time0

  lcd_home1(i2c)
  lcd_print(i2c, time1)
  puts time1

  sleep(1)
end
=end


###
### WLAN
###

=begin
sleep 0.1
wlan = WLAN.new('STA')
wlan.active(true)

# scan
wlan.scan

# connection
while true
  # ネットワークパラメタを定期的に表示
  puts wlan.ifconfig
  puts wlan.config('ip')
  puts wlan.config('mac')
  wlan.invoke( "http://www.gfd-dennou.org/" )
  
  # WiFiの接続が切れたときに自動的に際接続する
  if( ! wlan.is_connected? )
    puts 'start reconnect....'
    wlan.connect("essid", "pass")
    while true
      if( wlan.is_connected? )
        break
      end
      sleep 1
    end
  end
  sleep 1
end
=end


###
### WLAN
###

=begin
sleep 0.1
wlan = WLAN.new('STA')
wlan.active(true)

# scan
wlan.scan

# connection
while true
  # ネットワークパラメタを定期的に表示
  puts wlan.ifconfig
  puts wlan.config('ip')
  puts wlan.config('mac')
  wlan.invoke( "http://www.gfd-dennou.org/" )
  
  # WiFiの接続が切れたときに自動的に際接続する
  if( ! wlan.is_connected? )
    puts 'start reconnect....'
    wlan.connect("essid", "pass")
    while true
      if( wlan.is_connected? )
        break
      end
      sleep 1
    end
  end
  sleep 1
end
=end


###
### WLAN + SNTP
###
=begin
sleep 0.1
wlan = WLAN.new('STA')
wlan.active(true)

# connect
wlan.connect("essid", "pass")

# SNTP
time = SNTP.new()

# 表示
puts "現在時刻:"
puts sprintf("%04d-%02d-%02d %02d:%02d:%02d", time.year, time.mon, time.mday, time.hour, time.min, time.sec)
=end
