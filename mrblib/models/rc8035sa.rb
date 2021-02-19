# coding: utf-8
# RTC: EPSON RC-8035SA
#
# I2C address : 0x32

class RC8035SA
  def initialize(i2c)
    @i2c = i2c
    @i2c.write(0x32, [0xE0, 0x00, 0x00])
    sleep 0.1
  end

  def read
    a = @i2c.read_integer(0x32, 8)   # 8 バイト分読み込み
    ctrl2 = a.shift
    return [a[6], a[5], a[4], a[3], 0x3f & a[2], a[1], a[0], ctrl2]
  end

  def write(date)
    @i2c.write(0x32, [0x00, date[6], date[5], 0x80 | date[4], date[3], date[2], date[1], date[0]])
    sleep 0.1
    @i2c.write(0x32, [0xF0, 0x00])
    sleep 0.1
  end
end
