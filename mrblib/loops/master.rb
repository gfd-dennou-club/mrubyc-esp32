i2c = I2C.new(22, 21)
m5 = M5Stack.new(i2c)
x = 10
y = 20
c = 255
m5.set_pixel(x, y, c)
