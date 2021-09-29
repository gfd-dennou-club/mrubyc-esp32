#cording: UTF-8
class LCD
  ADDRESS = 0x3e
  
  def initialize(i2c)
    @i2c = i2c
  end
 
  def cmd(_cmd)
    @i2c.write(LCD::ADDRESS, [0x00, _cmd])
  end
  
  def data(_data)
    @i2c.write(LCD::ADDRESS, [0x40, _data])
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
    [0x38, 0x39, 0x14, 0x70, 0x56, 0x6c].each do |_cmd|
      cmd( _cmd)
    end
    sleep(0.3)
    [0x38, 0x0c, 0x01].each do |_cmd|
      cmd(_cmd)
    end
    sleep(0.1)
  end
   
  def print(_data)
    _data.length.times do |n|
      data(_data[n].ord)
    end
  end
 
end


i2c = I2C.new(22, 21)
tmp007 = TMP007.new(i2c)
sleep 4
lcd = LCD.new(i2c)

lcd.init()

puts "Adafruit TMP007 example"
if (!tmp007.init(TMP007::CFG_16SAMPLE)) 
  puts "No sensor found"
else
  while(1)
    objt = tmp007.readObjTempC()
    puts "Obj Temperature: #{objt} *C"
    diet = tmp007.readDieTempC()
    puts "Die Temperature: #{diet} *C" 

    str1 = sprintf("obj:%.1f",objt)
    str2 = sprintf("die:%.1f",diet)

    lcd.home0()
    lcd.print(str2)
    lcd.cursor(0,1)
    lcd.print(str1)
    sleep(1)
  end
end
