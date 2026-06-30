# coding: utf-8
#概要 summary


###
### GPIO チェック
### 
=begin
led = [
  GPIO.new( 13, GPIO::OUT ), 
  GPIO.new( 12, GPIO::OUT ),
  GPIO.new( 14, GPIO::OUT ),
  GPIO.new( 27, GPIO::OUT ),
  GPIO.new( 26, GPIO::OUT ),
  GPIO.new( 25, GPIO::OUT ),
  GPIO.new( 33, GPIO::OUT ),
  GPIO.new( 32, GPIO::OUT )
]
sw = [
  GPIO.new( 34, GPIO::IN ),
  GPIO.new( 35, GPIO::IN ),
  GPIO.new( 18, Pin::IN|GPIO::PULL_UP ),
  GPIO.new( 19, Pin::IN|GPIO::PULL_UP ),
]

puts "**************************"
puts "  GPIO Check (10 times) "
puts "**************************"

10.times do |i|
  if sw[0].read == 1 or sw[1].read == 1 or sw[2].read == 1 or sw[3].read == 1
    (0..7).each do |k|
      led[k].write( i % 2 )
    end
  else
    (0..7).each do |k|
      led[k].write( 0 )
    end
  end

  sleep 1
end

(0..7).each do |k|
  led[k].write( 0 )
end  
=end

###
### PWM 
### 
=begin
led = [
  GPIO.new( 13, GPIO::OUT ), 
  GPIO.new( 12, GPIO::OUT ),
  GPIO.new( 14, GPIO::OUT ),
  GPIO.new( 27, GPIO::OUT ),
  GPIO.new( 26, GPIO::OUT ),
  GPIO.new( 25, GPIO::OUT ),
  GPIO.new( 33, GPIO::OUT ),
  GPIO.new( 32, GPIO::OUT )
]
sw = [
  GPIO.new( 34, GPIO::IN ),
  GPIO.new( 35, GPIO::IN ),
  GPIO.new( 18, Pin::IN|GPIO::PULL_UP ),
  GPIO.new( 19, Pin::IN|GPIO::PULL_UP ),
]

puts "*********************"
puts " PWM check (LED)     "
puts "*********************"

# 初期化. timer (< 4) と channel は指定しないと 0 になる．
# デフォルトの周波数は 440
led2 = [
#  PWM.new( 13, timer:0, frequency:440, duty:0 ), 
#  PWM.new( 12, timer:0, frequency:440, duty:0 ), 
#  PWM.new( 14, timer:0, frequency:440, duty:0 ), 
#  PWM.new( 27, timer:0, frequency:440, duty:0 ), 
#  PWM.new( 26, timer:0, frequency:440, duty:0 ), 
#  PWM.new( 25, timer:0, frequency:440, duty:0 ), 
#  PWM.new( 33, timer:0, frequency:440, duty:0 ), 
#  PWM.new( 32, timer:0, frequency:440, duty:0 )
  PWM.new( 13 ),
  PWM.new( 12 ),
  PWM.new( 14 ),
  PWM.new( 27 ),
  PWM.new( 26 ),
  PWM.new( 25 ),
  PWM.new( 33 ),
  PWM.new( 32 )
]

# 同じ timer, channel のピンは必ず duty が同じになる．
# 同じ timer でも channel が違えば異なる duty を設定可能
num = 20
3.times do |i|
  for i in 0..num do
    duty_percent = ( ( 1023.0 / num ) * i / 1024.0 * 100 ).to_i # 0 < duty_percent < 100 
    
    (0..7).each do |k|     
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
3.times do |i|
  for i in 0..num do
    duty = ( 1023.0 / num ) * i / 1024.0 # 0 < duty < 1 
    us = (( 1.0 / 440.0 ) * duty * 1000000).to_i   # マイクロ秒
    #p "us = #{us} us (#{1.0 / 440.0 * 1000000} us), duty = #{duty}"
    
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


###
### PWM buzzer
###

puts "*************************************"
puts " PWM check (buzzer)                  "
puts "*************************************"

pwm0 = PWM.new(15, timer:1, frequency:440, duty:50 ) # duty 比 50%
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
=end

=begin
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
  temp = 1.0 / ( 1.0 / B * Math.log( (V - voltage*1000.0) / (voltage*1000.0/ Rref) / Rref) + 1.0 / (To + 273.0) ) - 273.0
  puts "#{voltage} V, #{temp} K"
  sleep(2)
end
=end

###
### 時刻設定
###
=begin
Time.mktime(2026,2,10,8,30,0)  #2026年2月10日 8:30:00 にセット

loop do
  puts "時刻更新"
  time = Time.now
  puts time.datetime
  p time.year, time.mon, time.mday, time.wday, time.hour, time.min, time.sec, time.msec
  sleep 10
end
=end


###
### I2C
###
=begin
class AQM0802A 
  def initialize(i2c)
    @i2c = i2c
    sleep(0.1)
    lcd_write(0x00, [0x38, 0x39, 0x14, 0x70, 0x56, 0x6c])
    sleep(1)
    lcd_write(0x00, [0x38, 0x0c, 0x01])
  end
  def lcd_write(opcode, data)
    n = 0
    while n < data.length
      @i2c.write(0x3e, [opcode, data[n]])
      n += 1
    end
  end
  def clear
    lcd_write(0x00, [0x01])
  end
  def cursor(line: 1)
    lcd_write(0x00, [0x80 + (0x40 * (line - 1))])
  end
  def print(s)
    a = Array.new
    str = s.to_s
    str.length.times do |n|
      a.push(str[n].ord)
    end
    lcd_write(0x40, a)
  end
end

#I2C 初期化
i2c = I2C.new()

# LCD 初期化
lcd = AQM0802A.new(i2c)

# LCD に "Hello World" 表示
lcd.clear          #初期化
var = 2222
str = "ESP"        #変数に値を代入
lcd.cursor(line: 1)   
lcd.print(var)
lcd.cursor(line: 2)
lcd.print("from #{str}") #変数の埋め込み

=end



###
### SPI SD
###
=begin
spi = SPI.new(miso_pin:19, mosi_pin:23, clk_pin:18)
sdspi = SDSPI.new(spi, cs_pin:2, mount_point:'/sdcard')

puts "-----ファイル出力-----"
f1 = File.open("/sdcard/herohero.txt", "w") #新規作成
f1.puts("Hello mruby/c! \n")
f1.close
f2 = File.open("/sdcard/herohero.txt", "a") #追記
f2.puts("Hi, mruby/c!\n")
f2.close
f3 = File.open("/sdcard/herohero.txt", "a") #追記
f3.puts("Kon-nichiwa, mruby/c!")
f3.close
puts ""

puts "-----ファイル内の全データを一度に取り出す(read)-----"
f4 = File.open("/sdcard/herohero.txt", "r")
puts f4.read
f4.close
puts ""

puts "----- 1 行ずつファイル内のデータを取り出す(gets)-----"
f5 = File.open("/sdcard/herohero.txt", "r")
while (text = f5.gets)
  puts text
end
f5.close
puts ""

puts "------ディレクトリ内の表示-------"
puts Dir.childrenFiles("/sdcard")
puts ""
puts Dir.childrenDirs("/sdcard")
puts ""
puts Dir.children("/sdcard")
puts ""

puts "------ファイル名の変更と確認-------"
# rename bar.txt to piyo.txt
File.rename("/sdcard/herohero.txt", "/sdcard/hogehero.txt")
puts Dir.childrenFiles("/sdcard")
puts ""

puts "------ファイルの削除と確認-------"
File.delete("/sdcard/hogehero.txt")
puts Dir.childrenFiles("/sdcard")
puts ""

# Clear up.
sdspi.umount()
=end

