# coding: utf-8

# I2C初期化
i2c = I2C.new(22, 21)

# pwm初期化
pwm0 = PWM.new( 15 )

# MCP9808初期化
mcp = MCP9808.new(i2c)

# LCD初期化
lcd = LCD.new(i2c)
lcd.init

if !mcp.init_begin() then
  print "miss begin"
else

  while true
    mcp.shutdown_wake(false)
    temp = mcp.readTempC()
    temp2 = sprintf(" %.1f*C", temp)
    print "Temp:#{temp}\n"
    lcd.home0()
    lcd.print("  TEMP")
    lcd.home1()
    lcd.print(temp2)

    if temp >= 28 then
      pwm0.freq(440)
      pwm0.duty(128)
      sleep 1

      pwm0.duty(0)
      sleep 1

      pwm0.freq(440)
      pwm0.duty(128)
      sleep 1

      pwm0.duty(0)
      sleep 1
    end

    sleep 1
    lcd.clear()

    mcp.shutdown_wake(true)
    sleep 1
  end
end
