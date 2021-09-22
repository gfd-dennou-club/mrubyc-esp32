#config: utf-8

class TMG
 ADDR=0x39
 ADDRESS=0x3e
 
 def initialize(i2c)
  @i2c=i2c
 end

 def set()
  @i2c.write(TMG::ADDR,[0x80,0x0F])
  @i2c.write(TMG::ADDR,[0x81,0x00])
  @i2c.write(TMG::ADDR,[0x83,0xFF])
  @i2c.write(TMG::ADDR,[0x8F,0x00])
  sleep(3)
 end

 def init()
  @i2c.write(TMG::ADDR,[0x94])
  @data = @i2c.read_integer(TMG::ADDR,9)
 end
 
 def calculation()
  cData = @data[1] * 256.0 + @data[0]
  red = @data[3] * 256.0 + @data[2]
  green = @data[5] * 256.0 + @data[4]
  blue = @data[7] * 256.0 + @data[6]
  {"red"=>red,"green"=>green,"blue"=>blue,"cData"=>cData,"proximity"=>@data[8]}
 end
end
