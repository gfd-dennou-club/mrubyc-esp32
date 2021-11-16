i2c = I2C.new(22, 21)
sht3x = SHT3X.new(i2c)
bmp280 = BMP280.new(i2c)
while 1 do
  p "sht3x: #{sht3x.readTemperature} ℃, #{sht3x.readHumidity} %"
  p "#{bmp280.readTemperature} ℃, #{bmp280.readPressure} Pa"
  sleep 1
end
