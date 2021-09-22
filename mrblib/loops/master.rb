#config: utf-8

i2c=I2C.new(22,21)

sensor = TMG.new(i2c)
lcd = LCD.new(i2c) 

sensor.set()
lcd.init()

led1 = GPIO.new(13, GPIO::OUT)
led2 = GPIO.new(12, GPIO::OUT)
led3 = GPIO.new(14, GPIO::OUT)
led4 = GPIO.new(27, GPIO::OUT)
led5 = GPIO.new(26, GPIO::OUT)
led6 = GPIO.new(25, GPIO::OUT)
led7 = GPIO.new(33, GPIO::OUT)
led8 = GPIO.new(32, GPIO::OUT)

sw1 = GPIO.new(34, GPIO::IN,GPIO::PULL_UP)
sw2 = GPIO.new(35, GPIO::IN,GPIO::PULL_UP)
sw3 = GPIO.new(18, GPIO::IN,GPIO::PULL_UP)
sw4 = GPIO.new(19, GPIO::IN,GPIO::PULL_UP)

while true
 sensor.init()

 ans = sensor.calculation()

 print "Green Color Luminance : "
 puts ans["green"]
 print "Red Color Luminance : "
 puts ans["red"]  
 print "Blue Color Luminance : "
 puts ans["blue"]
 print"InfraRed Luminance : "
 puts ans["cData"] 
 print "Proximity of the device : "
 puts ans["proximity"]
 sleep(1)

 if (sw1.read == 1)
   time0 = sprintf("red")
   time1 = sprintf("%d", ans["red"])

 elsif (sw2.read == 1)
   time0 = sprintf("green")
   time1 = sprintf("%d", ans["green"])

 elsif (sw3.read == 1)
   time0 = sprintf("blue")
   time1 = sprintf("%d", ans["blue"])
 
 elsif (sw4.read == 1)
   time0 = sprintf("cData")
   time1 = sprintf("%d", ans["cData"])

 else
   time0 = sprintf("proxi")
   time1 = sprintf("%d", ans["proximity"])
 end

 if (ans["proximity"] >= 100)
   led1.write(1)
   led2.write(1)
   led3.write(1)
   led4.write(1)
   led5.write(1)
   led6.write(1)
   led7.write(1)
   led8.write(1)
 else
   led1.write(0)
   led2.write(0)
   led3.write(0)
   led4.write(0)
   led5.write(0)
   led6.write(0)
   led7.write(0)
   led8.write(0)
 end

 lcd.clear()

  # time0, time1 を LCD に表示する部分は自分で書くこと
  lcd.home0()
  lcd.print(time0)
  lcd.home1()
  lcd.print(time1)
 # sleep(1)
end
