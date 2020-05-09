class SHT75

  ACK  = 0
  NACK = 1

  
  def initialize(gpio_sck, gpio_data)
    @gpio_sck  = gpio_sck
    @gpio_data = gpio_data
  end
  

  def sht_init
    GPIO.set_pullup(@gpio_sck)
    GPIO.set_mode_output(@gpio_sck)
    GPIO.set_pullup(@gpio_data)
    GPIO.set_mode_output(@gpio_data)
    
    s_connectionreset()
  end

  
  def sht_get_temp
    s_transstart()
    return get_temperature
  end
  

  def sht_get_humi(t)
    s_transstart()
    return get_humidity(t)
  end

  
  def get_temperature
    GPIO.set_level(@gpio_data, 0)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    GPIO.set_level(@gpio_data, 0)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    GPIO.set_level(@gpio_data, 0)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    GPIO.set_level(@gpio_data, 0)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    GPIO.set_level(@gpio_data, 0)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    GPIO.set_level(@gpio_data, 0)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    GPIO.set_level(@gpio_data, 1)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    GPIO.set_level(@gpio_data, 1)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    GPIO.set_mode_input(@gpio_data)
    ack = GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    sleep(1)

    v = GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    GPIO.set_mode_output(@gpio_data)
    GPIO.set_level(@gpio_data, 0)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)
    GPIO.set_mode_input(@gpio_data)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    GPIO.set_mode_output(@gpio_data)
    GPIO.set_level(@gpio_data, 0)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)
    GPIO.set_mode_input(@gpio_data)

    c = GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    c = (c << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    c = (c << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    c = (c << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    c = (c << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    c = (c << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    c = (c << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    c = (c << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    GPIO.set_mode_output(@gpio_data)
    GPIO.set_level(@gpio_data, 1)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    t = v - 3970
    printf("temperature row: %d, calc: %d.%02d\n", v, t / 100, t % 100)
    printf("ack : %d, crc : %08x\n", ack, c)

    return t
  end

  
  def get_humidity(t)
    GPIO.set_level(@gpio_data, 0)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    GPIO.set_level(@gpio_data, 0)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    GPIO.set_level(@gpio_data, 0)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    GPIO.set_level(@gpio_data, 0)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    GPIO.set_level(@gpio_data, 0)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    GPIO.set_level(@gpio_data, 1)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    GPIO.set_level(@gpio_data, 0)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    GPIO.set_level(@gpio_data, 1)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    GPIO.set_mode_input(@gpio_data)
    ack = GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    sleep(1)

    v = GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    GPIO.set_mode_output(@gpio_data)
    GPIO.set_level(@gpio_data, 0)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)
    GPIO.set_mode_input(@gpio_data)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    v = (v << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    GPIO.set_mode_output(@gpio_data)
    GPIO.set_level(@gpio_data, 0)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)
    GPIO.set_mode_input(@gpio_data)

    c = GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    c = (c << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    c = (c << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    c = (c << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    c = (c << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    c = (c << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    c = (c << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    c = (c << 1) | GPIO.get_level(@gpio_data)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    GPIO.set_mode_output(@gpio_data)
    GPIO.set_level(@gpio_data, 1)
    GPIO.set_level(@gpio_sck, 1)
    GPIO.set_level(@gpio_sck, 0)

    hi = 0.0367 * v - 0.0000015955 * (v * v) - 2.0468
    ht = hi + ((t - 2500) / 100.0) * (0.01 + 0.00008 * v)
    printf("humidity row: %d, calc: %f\n", v, ht)
    printf("ack : %d, crc : %08x\n", ack, c)

    return ht
  end


  def s_connectionreset
=begin
    specific waveform is below:

    SCK  _#_#_#_#_#_#_#_#_#_ _##___##_

    DATA ################### ##_____##
=end

    s_prepare_connectionreset()
    s_transstart()
  end

  def s_prepare_connectionreset
=begin
    specific waveform is below:

    SCK  _#_#_#_#_#_#_#_#_#_

    DATA ###################
=end

    # initial state
    GPIO.set_level(@gpio_data, 1)
    GPIO.set_level(@gpio_sck, 0)

    # 9 pulses
    i = 0
    while i < 9
      i = i + 1
      GPIO.set_level(@gpio_sck, 1)
      GPIO.set_level(@gpio_sck, 0)
    end
  end

  def s_transstart
=begin
    specific waveform is below:

    SCK  _##___##_

    DATA ##_____##
=end

    # initial state
    GPIO.set_level(@gpio_data, 1)
    GPIO.set_level(@gpio_sck, 0)

    # assert clock
    GPIO.set_level(@gpio_sck, 1)

    # negate dala during clock is assert
    GPIO.set_level(@gpio_data, 0)

    # negate clock
    GPIO.set_level(@gpio_sck, 0)

    # assert clock
    GPIO.set_level(@gpio_sck, 1)

    # assert dala during clock is assert
    GPIO.set_level(@gpio_data, 1)

    # negate clock
    GPIO.set_level(@gpio_sck, 0)
  end

end
