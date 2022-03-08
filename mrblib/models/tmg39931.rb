# 参考:https://github.com/Seeed-Studio/Seeed_TMG3993
class TMG39931
  # Enable bits
  ENABLE = {
    PON: 0x01,    # PowerON
    AEN: 0x02,    # ALS Enable
    PEN: 0x04,    # Proximity Enable
    WEN: 0x08,    # Wait Enable
    AIEN: 0x10,   # ALS Interrupt Enable
    PIEN: 0x20,   # Proximity Interrupt Enable
    GEN: 0x40,    # Gesture Enable
    PBEN: 0x80    # Pattern Burst Enable
  }
  # Status bits
  STATUS = {
    AVALID: 1,      # ALS Valid
    PVALID: 1 << 1, # Proximity Valid
    GINT: 1 << 2,   # Gesture Interrupt
    PBINT: 1 << 3,  # Pattern Burst Interrupt
    AINT: 1 << 4,   # ALS Interrupt
    PINT: 1 << 5,   # Proximity Interrupt
    PGSAT: 1 << 6,  # Proximity/Gesture Saturation
    CPSAT: 1 << 7   # Clear Photodiode Saturation
  }

  def initialize(i2c)
    @i2c = i2c
    @address = 0x39
  end

  def init
    sleep 0.06
    return false if get_device_id != 0x2a
    true
  end

  def get_device_id
    read_regs(0x92, 1)[0] >> 2
  end

  def enable_engines(enable_bits)
    pben = false
    data = []
    data[0] = 0x80
    # if (enable_bits & TMG39931::ENABLE_PBEN) != 0
    if (enable_bits & TMG39931::ENABLE[:PBEN]) != 0
      enable_bits = TMG39931::ENABLE[:PBEN] # clear all other engine enable bits if set
      pben = true
    end
    data[1] = TMG39931::ENABLE[:PON] | enable_bits
    write_regs(data)
    sleep 0.07 unless pben # for the prximity&guesture&color ring, we should wait 7ms for the chip to exit sleep mode
  end

  def set_adc_integration_time(atime)
    write_regs([0x81, atime])
  end

  def set_wait_time(wtime)
    write_regs([0x83, wtime])
  end

  def enable_wait_time_12x_factor(enable)
    if enable
      data = [0x83, 0x62]
    else
      data = [0x83, 0x60]
    end
    write_regs(data)
  end

  def get_interrupt_persistence_reg
    read_regs(0x8C, 1)[0]
  end

  def set_interrupt_persistence_reg(pers)
    write_regs([0x8C, pers])
  end

  def get_control_reg
    read_regs(0x8F, 1)[0]
  end

  def set_control_reg(control)
    write_regs([0x8F, control])
  end

  def get_config2
    read_regs(0x90, 1)[0]
  end

  def set_config2(config)
    write_regs([0x90, config])
  end

  def get_config3
    read_regs(0x9F, 1)[0]
  end

  def set_config3(config)
    write_regs([0x9F, config])
  end

  def get_status
    read_regs(0x93, 1)[0]
  end

  def clear_pattern_burst_interrupts
    read_regs(0xE3, 1)[0]
  end

  def force_assert_intp_in
    read_regs(0xE4, 1)[0]
  end

  def clear_proximity_interrupts
    read_regs(0xE5, 1)[0]
  end

  def clear_als_interrupts
    read_regs(0xE6, 1)[0]
  end

  def clear_all_interrupts
    read_regs(0xE7, 1)[0]
  end


  # proximity
  def setup_recommended_config_for_proximity
    # setProximityInterruptThreshold()
    set_proximity_pulse_cnt_len(63, 1)
    config2 = get_config2
    # config2 &= (3 << 4)
    config2 |= (3 << 4) # 300% power
    set_config2(config2)

    # this will change the response speed of the detection
    # smaller value will response quickly, but may raise error detection rate
    # bigger value will response slowly, but will ensure the detection is real.
    set_interrupt_persistence_reg(0xa << 4) # 10 consecutive proximity values out of range
  end

  def set_proximity_interrupt_threshold(low, high)
    write_regs([0x89, low])
    write_regs([0x8b, high])
  end

  def set_proximity_pulse_cnt_len(cnt, len)
    cnt = 63 if (cnt > 63) != 0
    len = 3 if (len > 3) != 0
    write_regs([0x8E, (cnt & 0x3f) | (len & 0xb0)])
  end

  def get_proximity_raw
    read_regs(0x9C, 1)[0]
  end

  # //-- ALS(light)
  def set_als_interrupt_threshold(low, high)
    data = []
    data[0] = 0x84
    data[1] = low & 0xff
    data[2] = low >> 8
    data[3] = high & 0xff
    data[4] = high >> 8
    write_regs(data)
  end

  def get_rgbc_raw
    data = read_regs(0x94, 8)
    rgbc = {}
    rgbc[:c] = (data[1] << 8) | data[0]
    rgbc[:r] = (data[3] << 8) | data[2]
    rgbc[:g] = (data[5] << 8) | data[4]
    rgbc[:b] = (data[7] << 8) | data[6]
    rgbc
  end

  def get_lux(rgbc = {})
    rgbc = get_rgbc_raw if rgbc.empty?
    data = read_regs(0x81, 1)
    ms = (256 - data[0]) * 2.78
    data = get_control_reg
    gain =
      case data & 0x3
      when 0x0
        1
      when 0x1
        4
      when 0x2
        16
      when 0x3
        64
      else
        1
      end
    ir = ((rgbc[:r] + rgbc[:g] + rgbc[:b]) - rgbc[:c]) / 2
    ir = 0 if ir < 0
    y = 0.362 * (rgbc[:r] - ir) + 1 * (rgbc[:g] - ir) + 0.136 * (rgbc[:b] - ir)
    cpl = ms * gain / 412
    (y / cpl).to_i
  end

  def get_cct(rgbc = {})
    rgbc = get_rgbc_raw if rgbc.empty?
    ir = ((rgbc[:r] + rgbc[:g] + rgbc[:b]) - rgbc[:c]) / 2
    ir = 0 if ir < 0
    lr = rgbc[:r]
    # lg = rgbc[:g]
    lb = rgbc[:b]
    min_v = lr < lb ? lr : lb
    ir = min_v - 0.1 if ir < min_v
    rate = (lb - ir) / (lr - ir)
    2745 * rate.to_i + 2242
  end

  def read_regs(addr, len)
    @i2c.write(@address, addr)
    @i2c.read_integer(@address, len)
  end

  def write_regs(data)
    @i2c.write(@address, data)
  end
end
