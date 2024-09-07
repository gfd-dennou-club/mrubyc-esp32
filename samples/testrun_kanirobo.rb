# coding: utf-8
#概要 summary

lux36 = ADC.new(36)
lux34 = ADC.new(34)
lux35 = ADC.new(35)
lux2  = ADC.new(2)

servo27 = PWM.new(27, timer:2, channel:3, frequency:50)
servo14 = PWM.new(14, timer:2, channel:4, frequency:50)

motor1 = GPIO.new(25, GPIO::OUT)
motor1_pwm = PWM.new(26, timer:1, channel:1)
motor2 = GPIO.new(32, GPIO::OUT)
motor2_pwm = PWM.new(33, timer:1, channel:2)

loop do
  puts( "----------------------------------------------" )
  puts( lux36.read_raw )
  puts( lux34.read_raw )
  puts( lux35.read_raw )
  puts( lux2.read_raw )

  motor1_pwm.duty( 0 )
  motor1.write(1)
  motor2_pwm.duty( 100 )
  motor2.write(1)
  servo27.pulse_width_us( 1000 )
  servo14.pulse_width_us( 1000 )
  sleep 3

  motor1_pwm.duty( 100 )
  motor1.write(1)
  motor2_pwm.duty( 0 )
  motor2.write(1)
  servo27.pulse_width_us( 2000 )
  servo14.pulse_width_us( 2000 )
  sleep 3
end

