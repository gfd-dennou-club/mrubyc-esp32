#cording:UTF-8

class TMP007
  VOBJ = 0x00
  TDIE = 0x01
  CONFIG = 0x02
  TOBJ = 0x03
  STATUS = 0x04
  STATMASK = 0x05

  CFG_RESET = 0x8000
  CFG_MODEON = 0x1000
  CFG_1SAMPLE = 0x0000
  CFG_2SAMPLE = 0x0200
  CFG_4SAMPLE = 0x0400
  CFG_8SAMPLE = 0x0600
  CFG_16SAMPLE = 0x0800
  CFG_ALERTEN = 0x0100
  CFG_ALERTF = 0x0080
  CFG_TRANSC = 0x0040

  STAT_ALERTEN = 0x8000
  STAT_CRTEN = 0x4000

  I2CADDR = 0x40
  DEVID = 0x1F

  def initialize(i2c)
    @i2c = i2c
  end

  def init(samplerate) 
    @i2c.write(TMP007::I2CADDR, [TMP007::CONFIG, (TMP007::CFG_RESET >> 8) & 0xFF, TMP007::CFG_RESET & 0xFF])
    sleep(0.01)

    @i2c.write(TMP007::I2CADDR, [TMP007::CONFIG, ((TMP007::CFG_MODEON | TMP007::CFG_ALERTEN | TMP007::CFG_TRANSC | samplerate) >> 8) & 0xFF, (TMP007::CFG_MODEON | TMP007::CFG_ALERTEN | TMP007::CFG_TRANSC | samplerate) & 0xFF])
    sleep(0.01)

    @i2c.write(TMP007::I2CADDR, [TMP007::STATMASK, ((TMP007::STAT_ALERTEN | TMP007::STAT_CRTEN) >> 8) & 0xFF, (TMP007::STAT_ALERTEN | TMP007::STAT_CRTEN) & 0xFF])
    sleep(0.01)

    @i2c.write(TMP007::I2CADDR, TMP007::DEVID)

    data = @i2c.read_integer(TMP007::I2CADDR, 2)

    value = ((data[0]) << 8) | data[1]
    did = value

    if (did != 0x78)
      return 0
    end

    sleep(4)

    return 1
  end

  def read_die_temp_c()
    tdie = read_raw_die_temperature()
    tdie *= 0.03125
    return tdie
  end

  def read_obj_temp_c()
    @i2c.write(TMP007::I2CADDR, TMP007::TOBJ)
    data = @i2c.read_integer(TMP007::I2CADDR, 2)
    value = ((data[0]) << 8) | data[1]
    raw = value
    if (raw & 0x1 != 0)
      return nil
    end
    raw >>= 2
    tobj = raw
    tobj *= 0.03125
    return tobj
  end

  def read_raw_die_temperature()
    @i2c.write(TMP007::I2CADDR, TMP007::TDIE)
    data = @i2c.read_integer(TMP007::I2CADDR, 2)
    value = ((data[0]) << 8) | data[1]
    raw = value
    raw >>= 2
    return raw
  end

  def read_raw_voltage()
    @i2c.write(TMP007::I2CADDR, TMP007::VOBJ)
    data = @i2c.read_integer(TMP007::I2CADDR, 2)
    value = ((data[0]) << 8) | data[1]
    raw = value
    return raw
  end

end

