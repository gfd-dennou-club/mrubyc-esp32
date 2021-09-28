i2c = I2C.new(22, 21)
sht3x = SHT3X.new(i2c)
bmp280 = BMP280.new(i2c)
while 1 do
  p "#{sht3x.readTemperature} C"
  p "#{bmp280.readTemperature} C, #{bmp280.readPressure} Pa" 
  sleep 1
end
