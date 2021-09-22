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
    if data <= 400
      return "Low"
    elsif data >= 401 && data <= 800
      return "Moderate"
    elsif data >= 801 && data <= 1200
      return "High"
    elsif data >= 1201 && data <= 1600
      return "Very High"
    elsif data >= 1601
      return "Extreme"
    end
  end

  def init()
    sleep 0.2
    @i2c.write(VEML::ARA,[0x01])
    @i2c.write(VEML::ADDRESS_L,[0x06])
  end
end


