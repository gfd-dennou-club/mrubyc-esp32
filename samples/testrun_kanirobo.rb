###
### kanirobo for ESP32 (片道ライントレース)
###

pwm27 = PWM.new(27, timer:1, frequency:50, duty:0)
pwm14 = PWM.new(14, timer:1, frequency:50, duty:0)

adc36 = ADC.new(36)
adc34 = ADC.new(34)
adc35 = ADC.new(35)
adc2  = ADC.new(2)

gpio13 = GPIO.new(13, GPIO::OUT)
gpio12 = GPIO.new(12, GPIO::OUT)
gpio25 = GPIO.new(25, GPIO::OUT)
gpio32 = GPIO.new(32, GPIO::OUT)
pwm26  = PWM.new(26, timer:0, frequency:1000, duty:0)
pwm33  = PWM.new(33, timer:0, frequency:1000, duty:0)


MIGI = 700
HIDARI = 250
pwm27.pulse_width_us( 1000 )
until adc36.read_raw > MIGI && adc34.read_raw > HIDARI
  puts("---------------")
  puts(adc36.read_raw)
  puts(adc34.read_raw)
  if adc36.read_raw > MIGI
    gpio13.write(0)
    gpio25.write(1)
    pwm26.duty( 100 )
  else
    gpio13.write(1)
    gpio25.write(1)
    pwm26.duty( 0 )
  end
  if adc34.read_raw > HIDARI
    gpio12.write(0)
    gpio32.write(1)
    pwm33.duty( 100 )
  else
    gpio12.write(1)
    gpio32.write(1)
    pwm33.duty( 0 )
  end
  sleep(0.1)
end
pwm27.pulse_width_us( 2000 )
sleep(1)
pwm27.pulse_width_us( 1000 )
