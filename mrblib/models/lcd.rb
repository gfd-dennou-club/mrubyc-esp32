# coding: utf-8
class LCD
  ADDRESS = 0x3e
 
  def initialize(i2c)
    @i2c = i2c
  end
  def cmd(cmd)
    @i2c.write(LCD::ADDRESS, [0x00, cmd])
  end  
  def data(data)
    @i2c.write(LCD::ADDRESS, [0x40, data])
  end
  def clear()
    cmd( 0x01)
    sleep 0.1
  end
  def home0()
    cmd( 0x02)
    sleep 0.1
  end
  def home1()
    cmd( 0x40|0x80)
    sleep 0.1
  end
 
  def cursor(x, y)
    pos = (x + y * 0x40) | 0x80 
    cmd( pos)
    sleep 0.1
  end
  def init()
    sleep 0.2
    [0x38, 0x39, 0x14, 0x70, 0x56, 0x6c].each do |cmd|
      cmd( cmd)
    end
    sleep(0.3)
    [0x38, 0x0c, 0x01].each do |cmd|
      cmd( cmd)
    end
    sleep(0.1)
  end
  def print(data)
      data.length.times do |n|
      data( data[n].ord)
    end
  end
end
