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
  GPIO.new( 18, Pin::IN,  Pin::PULL_UP ),
  Pin.new(  19, Pin::IN,  Pin::PULL_UP )
]

puts "**************************"
puts "  GPIO Check (10 times) "
puts "**************************"
puts "LEDs are ON if sw[0].read == 1 or sw[1].high? or sw[2].low? or sw[3].value == 1"
puts "GO TO DeepSleep if All Button HIGH"
puts "NEXT if All Button LOW"

1000.times do |i|
  if sw[0].read == 1 and sw[1].high? and sw[2].high? and sw[3].value == 1
    puts "deep sleep 10 sec (reboot)"
    us = 10 * 1000000
    SLEEP.deep_us( us )

  elsif sw[0].read == 0 and sw[1].low? and sw[2].low? and sw[3].value == 0
    break
    
  elsif sw[0].read == 1 or sw[1].high? or sw[2].low? or sw[3].value == 1
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

  if i == 3
    puts "light sleep 10 sec"
    us = 10 * 1000000
    sleep 0.2
    SLEEP.light_us( us )
  end
  
  sleep 1
end

# 消灯
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
  PWM.new( 32, channel:3, timer:3 )
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

# 440 Hz に変更
led2.each do |pwm|
  pwm.period_us( (1.0 / 440.0 * 1000000).to_i )
end

num = 20
2.times do |i|
  for i in 0..num do
    duty = ( 1023.0 / num ) * i / 1024.0           # 0 < duty < 1 
    us = (( 1.0 / 440.0 ) * duty * 1000000).to_i   # マイクロ秒
    p "us = #{us} us (#{1.0 / 440.0 * 1000000} us), duty = #{duty}"
    
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

## RTC 初期化. 時刻設定
rtc = RX8035SA.new(i2c)

# RTC に初期値書き込み
rtc.write([20, 3, 31, 1, 23, 59, 30]) #年(下2桁), 月, 日, 曜日, 時, 分, 秒

# LCD に "Hello World" 表示
lcd.cursor(0, 0)
lcd.write_string("Hello?!")
lcd.cursor(0, 1)
lcd.write_string("from ESP")

sleep(10)

# 適当な時間を表示
while true
  tt = rtc.read
  t0 = sprintf("%02d-%02d-%02d", tt[0], tt[1], tt[2])
  t1 = sprintf("%02d:%02d:%02d", tt[4], tt[5], tt[6])

  lcd.cursor(0, 0)
  lcd.write_string( t0 )
  lcd.cursor(0, 1)
  lcd.write_string( t1 )

#  puts sprintf("#{t0} #{t1}")
  sleep 0.2
end

