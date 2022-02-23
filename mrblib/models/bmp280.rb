class BMP280
  def initialize(i2c)
    @t_fine = 0
    @address = 0x76
    @i2c = i2c
    chipid = 0x58
    ret = @i2c.read_integer(@address, 1, 0xd0)
    if(chipid != ret[0])
      p "BMP280 initialization failed."
      return
    end
    readCoefficients()
    setSampling()
    sleep 0.1
  end

  def toInt16(n)
    (n & ~(1 << 15)) - (n & (1 << 15))
  end

  def read16(data = nil)
    ret = @i2c.read_integer(@address, 2, data)
    ret[0] << 8 | ret[1]
  end

  def read24(data = nil)
    ret = @i2c.read_integer(@address, 3, data)
    ret[0] << 16 | ret[1] << 8 | ret[2]
  end

  def read16_le(data = nil)
    r = read16(data)
    toInt16(((r >> 8) | (r << 8)) & 0xffff)
  end

  def read16s_le(data = nil)
    r = read16(data)
    ((r >> 8) | (r << 8)) & 0xffff
  end

  def readCoefficients()
    @dig_t1 = read16s_le(0x88)
    @dig_t2 = read16_le(0x8a)
    @dig_t3 = read16_le(0x8c)

    @dig_p1 = read16s_le(0x8e)
    @dig_p2 = read16_le(0x90)
    @dig_p3 = read16_le(0x92)
    @dig_p4 = read16_le(0x94)
    @dig_p5 = read16_le(0x96)
    @dig_p6 = read16_le(0x98)
    @dig_p7 = read16_le(0x9a)
    @dig_p8 = read16_le(0x9c)
    @dig_p9 = read16_le(0x9e)
  end

  def setSampling(mode = 0x03, tempSampling = 0x05, pressSampling = 0x05, filter = 0x00, duration = 0x00)
    meas_mode = mode
    meas_osrs_t = tempSampling
    meas_osrs_p = pressSampling
    
    conf_filter = filter
    conf_t_sb = duration

    conf = (conf_t_sb << 5) | (conf_filter << 2) | 0
    @i2c.write(@address, [0xf5, conf])

    meas = (meas_osrs_t << 5) | (meas_osrs_p << 2) | meas_mode
    @i2c.write(@address, [0xf4, meas])
  end

  def readTemperature()
    adc_T = read24(0xfa)
    adc_T >>= 4

    var1 = ((((adc_T >> 3) - (@dig_t1 << 1))) * (@dig_t2)) >> 11
    var2 = (((((adc_T >> 4) - (@dig_t1)) * ((adc_T >> 4) - (@dig_t1))) >> 12) * (@dig_t3)) >> 14

    @t_fine = var1 + var2

    t = (@t_fine * 5 + 128) >> 8
    t / 100.0
  end

  def readPressure()
    readTemperature()

    adc_P = read24(0xf7)
    return BMP280.convert_pressure(adc_P, @t_fine, @dig_p1, @dig_p2, @dig_p3, @dig_p4, @dig_p5, @dig_p6, @dig_p7, @dig_p8, @dig_p9)
  end
end