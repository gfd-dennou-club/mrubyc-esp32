# AQM0802A-RN-GBW
#
# I2C address : 0x3e

class AQM0802A
  
  def initialize(i2c)
    @i2c = i2c
  end

  def lcd_write(opcode, data)
    n = 0
    while n < data.length
      @i2c.write(0x3e, [opcode, data[n]])
      n += 1
    end
  end

  def setup
    lcd_write(0x00, [0x38, 0x39, 0x14, 0x70, 0x56, 0x6c])
    sleep(1)
    lcd_write(0x00, [0x38, 0x0c, 0x01])
  end

  def clear
    lcd_write(0x00, [0x01])
  end

  def cursor(x, y)
    lcd_write(0x00, [0x80 + (0x40 * y + x)])
  end

  def write_string(s)
    a = Array.new
    s.length.times do |n|
      a.push(s[n].ord)
    end
    lcd_write(0x40, a)
  end

end
