class Si7021
  ADDRESS = 0x40
  MEASRH_HOLD_CMD = 0xE5
  MEASRH_NOHOLD_CMD = 0xF5
  MEASTEMP_HOLD_CMD = 0xE3
  MEASTEMP_NOHOLD_CMD = 0xF3
  READPREVTEMP_CMD = 0xE0
  RESET_CMD = 0xFE
  WRITERHT_REG_CMD = 0xE6
  READRHT_REG_CMD = 0xE7
  WRITEHEATER_REG_CMD = 0x51
  READHEATER_REG_CMD = 0x11
  REG_HTRE_BIT = 0x02
  ID1_CMD = 0xFA0F
  ID2_CMD = 0xFCC9
  FIRMVERS_CMD = 0x84B8
  
  REV_1 = 0xff
  REV_2 = 0x20

  HEAT_LEVEL = {"LOWEST" => 0x00,
               "LOW" => 0x01,
               "MEDIUM" => 0x02,
               "HIGH" => 0x04,
               "HIGHER" => 0x08,
               "HIGHEST" => 0x0F
              }

  def initialize(i2c)
    @i2c = i2c
  end

  def begin
    reset()
    @i2c.write(Si7021::ADDRESS, Si7021::READRHT_REG_CMD)
    if @i2c.read_integer(Si7021::ADDRESS, 1)[0] != 0x3A
      return false
    end
    true
  end

  def reset
    @i2c.write(Si7021::ADDRESS, Si7021::RESET_CMD)
    sleep(0.05)
  end

  def read_temp
    @i2c.write(Si7021::ADDRESS, Si7021::MEASTEMP_NOHOLD_CMD)
    sleep(0.02)
    buf = @i2c.read_integer(Si7021::ADDRESS, 2)
    ((buf[0] << 8) | buf[1]) * 175.72 / 65_536 - 46.85
  end

  def read_humid
    @i2c.write(Si7021::ADDRESS, Si7021::MEASRH_NOHOLD_CMD)
    sleep(0.02)
    buf = @i2c.read_integer(Si7021::ADDRESS, 2)
    humid = ((buf[0] << 8) | buf[1]) * 125 / 65_536 - 6
    humid > 100.0 ? 100.0 : humid
  end

  def heater(sw)
    @i2c.write(Si7021::ADDRESS, Si7021::READRHT_REG_CMD)
    reg_value = @i2c.read_integer(Si7021::ADDRESS, 1)[0]
    if sw
      reg_value |= (1 << (Si7021::REG_HTRE_BIT))
    else
      reg_value &= ~(1 << (Si7021::REG_HTRE_BIT))
    end
    # 書き込めてなさそう
    @i2c.write(Si7021::ADDRESS, [Si7021::WRITERHT_REG_CMD, reg_value])
  end

  def heater_status
    @i2c.write(Si7021::ADDRESS, Si7021::READRHT_REG_CMD)
    reg_value = @i2c.read_integer(Si7021::ADDRESS, 1)[0]
    ((reg_value >> (Si7021::REG_HTRE_BIT)) & 1) == 1 ? true : false
  end

  def set_heater_level(level)
    @i2c.write(Si7021::ADDRESS, [Si7021::WRITEHEATER_REG_CMD, level])
  end
end
