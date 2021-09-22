# coding: utf-8 
#I2C 初期化
i2c = I2C.new(22, 21)
# LCD 初期化
lcd = LCD.new(i2c)
lcd.init

# LCD に "Hello World" 表示
# 自分で書くこと
lcd.cursor(1,0)
lcd.print('Hello!')
lcd.home1()
lcd.print('from ESP')

sleep 3
lcd.clear

# VEML6070の初期化
veml = VEML.new(i2c)
veml.init

while true
  date1 = veml.get_1
  date2 = veml.get_2(date1)
  level = veml.level(date2)
  lcd.home0()
  lcd.print("UV date")
  lcd.home1()
  lcd.print(date2.to_s)
  sleep 1
  lcd.clear()
  lcd.home0()
  lcd.print("UV level")
  lcd.home1()
  lcd.print(level.to_s)
  sleep 1
  lcd.clear()
end
