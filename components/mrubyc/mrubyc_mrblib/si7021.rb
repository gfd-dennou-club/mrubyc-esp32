class Si7021
    SI_7021_ADDRESS = 0x40
  
    def initialize(i2c)
      @i2c = i2c
    end
  
    def readTemp()
  
      @i2c.write(Si7021::SI_7021_ADDRESS, 0xE3)
      sleep(0.05)
      buf = @i2c.read_integer(Si7021::SI_7021_ADDRESS, 2)
  
      temp = (buf[0] << 8) | buf[1]
      temp *= 175.72
      temp /= 65536
      temp -= 46.85
  
      return temp
    end
  
    def readHumid()
      @i2c.write(Si7021::SI_7021_ADDRESS, 0xE5)
      sleep(0.05)
      buf = @i2c.read_integer(Si7021::SI_7021_ADDRESS, 2)
  
      humid = (buf[0] << 8) | buf[1]
      humid *= 125
      humid /= 65536
      humid -= 6
  
      return humid
    end
  end