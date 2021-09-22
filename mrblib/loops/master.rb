

i2c = I2C.new(22, 21)

si7021 = Si7021.new(i2c)

puts si7021.readHumid()
puts si7021.readTemp()