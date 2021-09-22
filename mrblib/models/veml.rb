# coding: utf-8

class VEML
  ADDRESS_L = 0x38
  ADDRESS_H = 0x39
  ARA       = 0x0c
  COMMAND   = 0x02

  def initialize(i2c)
    @i2c = i2c
  end

  def get_1()
    data = @i2c.read_integer(VEML::ADDRESS_H,1)
    return data[0]
  end

  def get_2(data1)
    data2 = @i2c.read_integer(VEML::ADDRESS_L,1)
    return (data1 << 8) | data2[0] 
  end

  def level (data)
    if data <= 560
      return "Low"
    elsif data >= 561 && data <= 1120
      return "Moderate"
    elsif data >= 1121 && data <= 1494
      return "High"
    elsif data >= 1495 && data <= 2054
      return "Very High"
    elsif data >= 2055
      return "Extreme"
    end
  end

  def init()
    sleep 0.2
    @i2c.write(VEML::ARA,[0x01])
    @i2c.write(VEML::ADDRESS_L,[0x06])
  end
end


