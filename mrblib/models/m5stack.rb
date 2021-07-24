class M5Stack

  ADDRESS = 0x41

  def initialize(i2c)
    @i2c = i2c
  end

  def set_pixel(x, y, c)
    @i2c.write(M5Stack::ADDRESS, x, y, c)
  end

end
