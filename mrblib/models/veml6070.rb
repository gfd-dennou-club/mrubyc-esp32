# coding: utf-8

class VEML6070
  ADDRESS_L = 0x38
  ADDRESS_H = 0x39
  ARA       = 0x0c
  COMMAND   = 0x02

  def initialize(i2c)
    @i2c = i2c
    @uv = 0
  end

  def init
    @i2c.write(VEML6070::ARA,[0x01])
    @i2c.write(VEML6070::ADDRESS_L,[0x06])
  end

  def get_uv
    data_h = @i2c.read_integer(VEML6070::ADDRESS_H,1)
    data_l = @i2c.read_integer(VEML6070::ADDRESS_L,1)
    @uv = (data_h[0] << 8) | data_l[0] 
  end

  def get_uv_index(uv = @uv)
    if uv <= 560
      "Low"
    elsif uv >= 561 && uv <= 1120
      "Moderate"
    elsif uv >= 1121 && uv <= 1494
      "High"
    elsif uv >= 1495 && uv <= 2054
      "Very High"
    elsif uv >= 2055
      "Extreme"
    end
  end
end


