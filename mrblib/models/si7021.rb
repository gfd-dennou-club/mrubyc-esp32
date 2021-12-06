class Si7021
  ADDRESS = 0x40
  MEASURE_HUMIDITY = 0xE5
  MEASURE_TEMPERATURE = 0xE3
  READRHT_REG_CMD = 0xE7
  REG_HTRE_BIT = 0x02
  WRITERHT_REG_CMD = 0xE6
  WRITEHEATER_REG_CMD = 0x51
  HEAT_LEVEL = {"LOWEST" => 0x00,
               "LOW" => 0x01,
               "MEDIUM" => 0x02,
               "HIGH" => 0x04,
               "HIGHER" => 0x08,
               "HIGHEST" => 0x0F}

  def initialize(i2c)
    @i2c = i2c
  end

  def read_temp
    @i2c.write(Si7021::ADDRESS, Si7021::MEASURE_TEMPERATURE)
    sleep(0.05)
    buf = @i2c.read_integer(Si7021::ADDRESS, 2)
    ((buf[0] << 8) | buf[1]) * 175.72 / 65_536 - 46.85
  end

  def read_humid
    @i2c.write(Si7021::ADDRESS, Si7021::MEASURE_HUMIDITY)
    sleep(0.05)
    buf = @i2c.read_integer(Si7021::ADDRESS, 2)
    humid = ((buf[0] << 8) | buf[1]) * 125 / 65_536 - 6
    humid > 100.0 ? 100.0 : humid
  end

  def heater(sw)
    @i2c.write(Si7021::ADDRESS, Si7021::READRHT_REG_CMD)
    reg_value = @i2c.read_integer(Si7021::ADDRESS, 1)
    if sw
      reg_value |= (1 << (Si7021::REG_HTRE_BIT))
    else
      reg_value &= ~(1 << (Si7021::REG_HTRE_BIT))
    end
    @i2c.write(Si7021::ADDRESS, Si7021::WRITERHT_REG_CMD)
    @i2c.write(Si7021::ADDRESS, reg_value)
  end

  def heater_status
    @i2c.write(Si7021::ADDRESS, Si7021::READRHT_REG_CMD)
    reg_value = @i2c.read_integer(Si7021::ADDRESS, 1)
    (reg_value << Si7021::REG_HTRE_BIT) & 1 == 1 ? True : False
  end

  def set_heater_level(level)
    @i2c.write(Si7021::ADDRESS, Si7021::WRITEHEATER_REG_CMD)
    @i2c.write(Si7021::ADDRESS, level)
  end
end
