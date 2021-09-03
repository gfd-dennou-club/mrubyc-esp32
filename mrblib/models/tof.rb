# VL53L0X

# Referense:
# https://github.com/pololu/vl53l0x-arduino/blob/master/VL53L0X.cpp
# https://github.com/pololu/vl53l0x-arduino/blob/master/VL53L0X.h

class VL53L0X
  ADDRESS = 0b0101001
  START_OVERHEAD = 1910
  END_OVERHEAD = 960
  MSRC_OVERHEAD = 660
  TCC_OVERHEAD = 590
  DSS_OVERHEAD = 690
  PRE_RANGE_OVERHEAD = 660
  FINAL_RANGE_OVERHEAD = 550
  MIN_TIMING_BUDGET = 20_000

  def initialize(i2c)
    @i2c = i2c
    @address = VL53L0X::ADDRESS
    @io_timeout = 0
    @did_timeout = false
  end

  def init(io_2v8 = true)
    return false if read_reg(0xc0) != 0xee

    write_reg(0x89, read_reg(0x89) | 0x01) if io_2v8

    write_reg(0x88, 0x00)

    write_reg(0x80, 0x01)
    write_reg(0xff, 0x01)
    write_reg(0x00, 0x00)
    @stop_variable = read_reg(0x91)
    write_reg(0x00, 0x01)
    write_reg(0xff, 0x00)
    write_reg(0x80, 0x00)

    write_reg(0x60, read_reg(0x60) | 0x12)

    set_signal_rate_limit(0.25)
    write_reg(0x01, 0xff)

    info, spad_count, spad_type_is_aperture = get_spad_info
    return false unless info

    ref_spad_map = read_multi(0xb0, 6)

    write_reg(0xff, 0x01)
    write_reg(0x4f, 0x00)
    write_reg(0x4e, 0x2c)
    write_reg(0xff, 0x00)
    write_reg(0xb6, 0xb4)

    first_spad_to_enable = spad_type_is_aperture ? 12 : 0
    spads_enabled = 0

    (0...48).each do |i|
      if i < first_spad_to_enable || spads_enabled == spad_count
        ref_spad_map[i / 8] &= ~(1 << (i % 8))
      elsif ((ref_spad_map[i / 8] >> (i % 8)) & 0x1) != 0
        spads_enabled += 1
      end
    end

    write_multi(0xb0, ref_spad_map)

    write_reg(0xff, 0x01)
    write_reg(0x00, 0x00)

    write_reg(0xff, 0x00)
    write_reg(0x09, 0x00)
    write_reg(0x10, 0x00)
    write_reg(0x11, 0x00)

    write_reg(0x24, 0x01)
    write_reg(0x25, 0xff)
    write_reg(0x75, 0x00)

    write_reg(0xff, 0x01)
    write_reg(0x4e, 0x2c)
    write_reg(0x48, 0x00)
    write_reg(0x30, 0x20)

    write_reg(0xff, 0x00)
    write_reg(0x30, 0x09)
    write_reg(0x54, 0x00)
    write_reg(0x31, 0x04)
    write_reg(0x32, 0x03)
    write_reg(0x40, 0x83)
    write_reg(0x46, 0x25)
    write_reg(0x60, 0x00)
    write_reg(0x27, 0x00)
    write_reg(0x50, 0x06)
    write_reg(0x51, 0x00)
    write_reg(0x52, 0x96)
    write_reg(0x56, 0x08)
    write_reg(0x57, 0x30)
    write_reg(0x61, 0x00)
    write_reg(0x62, 0x00)
    write_reg(0x64, 0x00)
    write_reg(0x65, 0x00)
    write_reg(0x66, 0xa0)

    write_reg(0xff, 0x01)
    write_reg(0x22, 0x32)
    write_reg(0x47, 0x14)
    write_reg(0x49, 0xff)
    write_reg(0x4a, 0x00)

    write_reg(0xff, 0x00)
    write_reg(0x7a, 0x0a)
    write_reg(0x7b, 0x00)
    write_reg(0x78, 0x21)

    write_reg(0xff, 0x01)
    write_reg(0x23, 0x34)
    write_reg(0x42, 0x00)
    write_reg(0x44, 0xff)
    write_reg(0x45, 0x26)
    write_reg(0x46, 0x05)
    write_reg(0x40, 0x40)
    write_reg(0x0e, 0x06)
    write_reg(0x20, 0x1a)
    write_reg(0x43, 0x40)

    write_reg(0xff, 0x00)
    write_reg(0x34, 0x03)
    write_reg(0x35, 0x44)

    write_reg(0xff, 0x01)
    write_reg(0x31, 0x04)
    write_reg(0x4b, 0x09)
    write_reg(0x4c, 0x05)
    write_reg(0x4d, 0x04)

    write_reg(0xff, 0x00)
    write_reg(0x44, 0x00)
    write_reg(0x45, 0x20)
    write_reg(0x47, 0x08)
    write_reg(0x48, 0x28)
    write_reg(0x67, 0x00)
    write_reg(0x70, 0x04)
    write_reg(0x71, 0x01)
    write_reg(0x72, 0xfe)
    write_reg(0x76, 0x00)
    write_reg(0x77, 0x00)

    write_reg(0xff, 0x01)
    write_reg(0x0d, 0x01)

    write_reg(0xff, 0x00)
    write_reg(0x80, 0x01)
    write_reg(0x01, 0xf8)

    write_reg(0xff, 0x01)
    write_reg(0x8e, 0x01)
    write_reg(0x00, 0x01)
    write_reg(0xff, 0x00)
    write_reg(0x80, 0x00)

    write_reg(0x0a, 0x04)
    write_reg(0x84, read_reg(0x84) & ~0x10)
    write_reg(0x0b, 0x01)

    @measurement_timing_budget_us = get_measurement_timing_budget

    write_reg(0x01, 0xe8)

    set_measurement_timing_budget(@measurement_timing_budget_us)

    write_reg(0x01, 0x01)
    return false unless perform_single_refcalibration(0x40)

    write_reg(0x01, 0x02)
    return false unless perform_single_refcalibration(0x00)

    write_reg(0x01, 0xe8)

    true
  end

  def write_reg(reg, value)
    @i2c.write(@address, [reg, value])
  end

  def write_reg_16bit(reg, value)
    @i2c.write(@address, [reg, (value >> 8) & 0xff, value & 0xff])
  end

  def write_reg_32bit(reg, value)
    @i2c.write(@address, [reg, (value >> 24) & 0xff, (value >> 16) & 0xff, (value >> 8) & 0xff, value & 0xff])
  end

  def read_reg(reg)
    @i2c.write(@address, reg)
    data = @i2c.read_integer(@address, 1)
    data[0]
  end

  def read_reg_16bit(reg)
    @i2c.write(@address, reg)
    data = @i2c.read_integer(@address, 2)
    (data[0] << 8) | data[1]
  end

  def read_reg_32bit(reg)
    @i2c.write(@address, reg)
    data = @i2c.read_integer(@address, 4)
    (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]
  end

  def write_multi(reg, src)
    @i2c.write(@address, reg)
    @i2c.write(@address, src)
  end

  def read_multi(reg, count)
    @i2c.write(@address, reg)
    dst = []
    (0...count).each do |i|
      data = @i2c.read_integer(@address, 1)
      dst[i] = data[0]
    end
    dst
  end

  def calc_macro_period(vcsel_period_pclks)
    ((2304 * vcsel_period_pclks * 1655) + 500) / 1000
  end

  def check_timeout_expired
    (@io_timeout > 0) && (self.millis - @timeout_start_ms > @io_timeout)
  end

  def set_signal_rate_limit(limit_mcps)
    if limit_mcps < 0.0 || limit_mcps > 511.99
      false
    else
      write_reg_16bit(0x44, (limit_mcps * (1 << 7)).to_i)
      true
    end
  end

  def get_signal_rate_limit
    read_reg_16bit(0x44) / (1 << 7)
  end

  def set_measurement_timing_budget(budget_us)
    return false if budget_us < VL53L0X::MIN_TIMING_BUDGET

    used_budget_us = VL53L0X::START_OVERHEAD + VL53L0X::END_OVERHEAD

    init_sequence_step_enables
    init_sequence_step_timeouts

    used_budget_us += (@msrc_dss_tcc_us + VL53L0X::TCC_OVERHEAD) if @tcc

    if @dss
      used_budget_us += 2 * (@msrc_dss_tcc_us + VL53L0X::DSS_OVERHEAD)
    elsif @msrc
      used_budget_us += (@msrc_dss_tcc_us + VL53L0X::MSRC_OVERHEAD)
    end

    used_budget_us += (@pre_range_us + VL53L0X::PRE_RANGE_OVERHEAD) if @pre_range

    if @final_range
      used_budget_us += VL53L0X::FINAL_RANGE_OVERHEAD
      return false if used_budget_us > budget_us

      final_range_timeout_us = budget_us - used_budget_us

      final_range_timeout_mclks =
        timeout_microseconds_to_mclks(final_range_timeout_us,
                                      @final_range_vcsel_period_pclks)

      final_range_timeout_mclks += @pre_range_mclks if @pre_range
      write_reg_16bit(0x71, encode_timeout(final_range_timeout_mclks))

      @measurement_timing_budget_us = budget_us
    end
    true
  end

  def get_measurement_timing_budget
    budget_us = VL53L0X::START_OVERHEAD + VL53L0X::END_OVERHEAD
    init_sequence_step_enables
    init_sequence_step_timeouts

    budget_us += (@msrc_dss_tcc_us + VL53L0X::TCC_OVERHEAD) if @tcc

    if @dss
      budget_us += 2 * (@msrc_dss_tcc_us + VL53L0X::DSS_OVERHEAD)
    elsif @msrc
      budget_us += (@msrc_dss_tcc_us + VL53L0X::MSRC_OVERHEAD)
    end

    budget_us += (@pre_range_us + VL53L0X::PRE_RANGE_OVERHEAD) if @pre_range

    budget_us += (@final_range_us + VL53L0X::FINAL_RANGE_OVERHEAD) if @final_range

    @measurement_timing_budget_us = budget_us
    budget_us
  end

  def get_vcsel_pulse_period(type)
    if type == 0
      (read_reg(0x50) + 1) << 1
    elsif type == 1
      (read_reg(0x70) + 1) << 1
    else
      255
    end
  end

  def start_continuous(period_ms = 0)
    write_reg(0x80, 0x01)
    write_reg(0xff, 0x01)
    write_reg(0x00, 0x00)
    write_reg(0x91, @stop_variable)
    write_reg(0x00, 0x01)
    write_reg(0xff, 0x00)
    write_reg(0x80, 0x00)

    if period_ms != 0
      osc_calibrate_val = read_reg_16bit(0xf8)
      period_ms *= osc_calibrate_val if osc_calibrate_val != 0
      write_reg_32bit(0x04, period_ms)
      write_reg(0x00, 0x04)
    else
      write_reg(0x00, 0x02)
    end
  end

  def read_range_continuous_millimeters
    start_timeout
    while (read_reg(0x13) & 0x07) == 0
      if check_timeout_expired
        @did_timeout = true
        return 65_535
      end
    end
    range = read_reg_16bit(0x14 + 10)
    write_reg(0x0b, 0x01)
    range
  end

  def read_range_single_millimeters
    write_reg(0x80, 0x01)
    write_reg(0xFF, 0x01)
    write_reg(0x00, 0x00)
    write_reg(0x91, @stop_variable)
    write_reg(0x00, 0x01)
    write_reg(0xFF, 0x00)
    write_reg(0x80, 0x00)

    write_reg(0x00, 0x01)

    start_timeout
    while (read_reg(0x00) & 0x01) != 0
      if check_timeout_expired
        @did_timeout = true
        return 65_535
      end
    end
    read_range_continuous_millimeters
  end

  def set_timeout(timeout)
    @io_timeout = timeout
  end

  def start_timeout
    @timeout_start_ms = self.millis
  end

  def timeout_occurred
    tmp = @did_timeout
    @did_timeout = false
    tmp
  end

  def get_spad_info
    write_reg(0x80, 0x01)
    write_reg(0xff, 0x01)
    write_reg(0x00, 0x00)

    write_reg(0xff, 0x06)
    write_reg(0x83, read_reg(0x83) | 0x04)
    write_reg(0xff, 0x07)
    write_reg(0x81, 0x01)

    write_reg(0x80, 0x01)

    write_reg(0x94, 0x6b)
    write_reg(0x83, 0x00)
    start_timeout
    while read_reg(0x83) == 0x00
      return [false, nil, nil] if check_timeout_expired

    end
    write_reg(0x83, 0x01)
    tmp = read_reg(0x92)

    count = tmp & 0x7f
    type_is_aperture = ((tmp >> 7) & 0x01 != 0) ? true : false

    write_reg(0x81, 0x00)
    write_reg(0xff, 0x06)
    write_reg(0x83, read_reg(0x83) & ~0x04)
    write_reg(0xff, 0x01)
    write_reg(0x00, 0x01)

    write_reg(0xff, 0x00)
    write_reg(0x80, 0x00)

    [true, count, type_is_aperture]
  end

  def init_sequence_step_enables
    sequence_config = read_reg(0x01)
    @tcc = ((sequence_config >> 4) & 0x01) != 0 ? true : false
    @dss = ((sequence_config >> 3) & 0x01) != 0 ? true : false
    @msrc = ((sequence_config >> 2) & 0x01) != 0 ? true : false
    @pre_range = ((sequence_config >> 6) & 0x01) != 0 ? true : false
    @final_range = ((sequence_config >> 7) & 0x01) != 0 ? true : false
  end

  def init_sequence_step_timeouts
    @pre_range_vcsel_period_pclks = get_vcsel_pulse_period(0)
    @msrc_dss_tcc_mclks = read_reg(0x46) + 1
    @msrc_dss_tcc_us = timeout_mclks_to_microseconds(@msrc_dss_tcc_mclks, @pre_range_vcsel_period_pclks)
    @pre_range_mclks = decode_timeout(read_reg_16bit(0x51))
    @pre_range_us = timeout_mclks_to_microseconds(@pre_range_mclks, @pre_range_vcsel_period_pclks)
    @final_range_vcsel_period_pclks = get_vcsel_pulse_period(1)
    @final_range_mclks = decode_timeout(read_reg_16bit(0x71))

    @final_range_mclks -= @pre_range_mclks if @pre_range
    @final_range_us = timeout_mclks_to_microseconds(@final_range_mclks, @final_range_vcsel_period_pclks)
  end

  def decode_timeout(reg_val)
    ((reg_val & 0x00ff) << ((reg_val & 0xff00) >> 8)) + 1
  end

  def encode_timeout(timeout_mclks)
    ls_byte = 0
    ms_byte = 0
    if timeout_mclks > 0
      ls_byte = timeout_mclks - 1
      while ((ls_byte >> 32) & 0xffff) > 0 || (ls_byte & 0xff00) > 0
        ls_byte >>= 1
        ms_byte += 1
      end
      (ms_byte << 8) | (ls_byte & 0xff)
    else
      0
    end
  end

  def timeout_mclks_to_microseconds(timeout_period_mclks, vcsel_period_pclks)
    macro_period_ns = calc_macro_period(vcsel_period_pclks)
    ((timeout_period_mclks * macro_period_ns) + 500) / 1000
  end

  def timeout_microseconds_to_mclks(timeout_period_us, vcsel_period_pclks)
    macro_period_ns = calc_macro_period(vcsel_period_pclks)
    ((timeout_period_us * 1000) + (macro_period_ns / 2)) / macro_period_ns
  end

  def perform_single_refcalibration(vhv_init_byte)
    write_reg(0x00, 0x01 | vhv_init_byte)

    start_timeout
    while read_reg(0x13) & 0x07 == 0
      return false if check_timeout_expired

    end
    write_reg(0x0b, 0x01)
    write_reg(0x00, 0x00)

    true
  end
end
