# VL53L0X

# Referense:
# https://github.com/pololu/vl53l0x-arduino/blob/master/VL53L0X.cpp
# https://github.com/pololu/vl53l0x-arduino/blob/master/VL53L0X.h
# http://note.suzakugiken.jp/pololu-tof-tutorial-a/2/

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
    # インスタンスの生成
    #   Parameters:
    #     i2c : 通信に使用するi2cオブジェクト

    @i2c = i2c
    @address = VL53L0X::ADDRESS
    @io_timeout = 0
    @did_timeout = false
  end

  def set_address(new_addr)
    # i2cアドレスの変更
    #   Parameters:
    #     new_addr : i2cアドレス

    write_reg(0x8A, new_addr & 0x7f)
    @address = new_addr
  end

  def get_address
    # i2cアドレスの取得
    #   Returns:
    #     i2cアドレス(7bit)

    @address
  end

  def init(io_2v8 = true)
    # センサの初期化
    #   Parameters:
    #     io_2v8 : true  2v8モードにする
    #              false iv8モードにする
    #   Returns:
    #     true  初期化成功
    #     false 初期化失敗

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
    return false unless perform_single_ref_calibration(0x40)

    write_reg(0x01, 0x02)
    return false unless perform_single_ref_calibration(0x00)

    write_reg(0x01, 0xe8)

    true
  end

  def write_reg(reg, value)
    # センサレジスタに8bitの値を書き込む
    #   Parameters:
    #     reg : レジスタアドレス
    #     value : 書き込む値(8bit)

    @i2c.write(@address, [reg, value])
  end

  def write_reg_16bit(reg, value)
    # センサレジスタに16bitの値を書き込む
    #   Parameters:
    #     reg : レジスタアドレス
    #     value : 書き込む値(16bit)

    @i2c.write(@address, [reg, (value >> 8) & 0xff, value & 0xff])
  end

  def write_reg_32bit(reg, value)
    # センサレジスタに32bitの値を書き込む
    #   Parameters:
    #     reg : レジスタアドレス
    #     value : 書き込む値(32bit)

    @i2c.write(@address, [reg, (value >> 24) & 0xff, (value >> 16) & 0xff, (value >> 8) & 0xff, value & 0xff])
  end

  def read_reg(reg)
    # センサレジスタから8bitの値を読み込む
    #   Parameters:
    #     reg : レジスタアドレス
    #   Returns:
    #     センサレジスタの値(8bit)

    @i2c.write(@address, reg)
    data = @i2c.read_integer(@address, 1)
    data[0]
  end

  def read_reg_16bit(reg)
    # センサレジスタから16bitの値を読み込む
    #   Parameters:
    #     reg : レジスタアドレス
    #   Returns:
    #     センサレジスタの値(16bit)

    @i2c.write(@address, reg)
    data = @i2c.read_integer(@address, 2)
    (data[0] << 8) | data[1]
  end

  def read_reg_32bit(reg)
    # センサレジスタから32bitの値を読み込む
    #   Parameters:
    #     reg : レジスタアドレス
    #   Returns:
    #     センサレジスタの値(32bit)

    @i2c.write(@address, reg)
    data = @i2c.read_integer(@address, 4)
    (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]
  end

  def write_multi(reg, src)
    # 指定したレジスタを基点に指定した配列から値を書き込む
    #   Parameters:
    #     reg : レジスタアドレス
    #     src : 配列(8bit)

    @i2c.write(@address, reg)
    @i2c.write(@address, src)
  end

  def read_multi(reg, count)
    # 指定したレジスタを基点に指定したバイト数のレジスタの値を配列で返す
    #   Parameters:
    #     reg : レジスタアドレス
    #     count : バイト数
    #   Returns:
    #     読み込んだデータを保持した配列(8bit)

    @i2c.write(@address, reg)
    dst = []
    (0...count).each do |i|
      data = @i2c.read_integer(@address, 1)
      dst[i] = data[0]
    end
    dst
  end

  def calc_macro_period(vcsel_period_pclks)
    # VCSELの周期(PCLK)から周期(ns)を算出
    #   Returns:
    #     周期(ns)

    ((2304 * vcsel_period_pclks * 1655) + 500) / 1000
  end

  def check_timeout_expired
    # タイムアウトが有効(0以外)のときの時間切れの確認
    #   Returns:
    #     true  時間内
    #     false 時間切れ

    (@io_timeout > 0) && (self.millis - @timeout_start_ms > @io_timeout)
  end

  def set_signal_rate_limit(limit_mcps = 0.25)
    # 戻り信号のレート制限を1秒あたりのメガカウント(MCPS)で指定した値に設定
    #   Parameters:
    #     limit_mcps : 指定する値(MCPS)
    #   Returns:
    #     true  値が有効
    #     false 値が無効

    if limit_mcps < 0.0 || limit_mcps > 511.99
      false
    else
      write_reg_16bit(0x44, (limit_mcps * (1 << 7)).to_i)
      true
    end
  end

  def get_signal_rate_limit
    # 現在の戻り信号のレート制限(MCPS)を取得
    #   Returns:
    #     レート制限(MCPS)

    read_reg_16bit(0x44).to_f / (1 << 7)
  end

  def set_measurement_timing_budget(budget_us)
    # 測定タイミングパジェットを指定
    # 測定タイミングパジェットは1回の距離測定に許容される時間
    #   Parameters:
    #     budget_us : 指定する測定タイミングパジェット(μs)
    #   Returns:
    #     true  値が有効
    #     false 値が無効

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
    # 測定タイミングパジェットを取得
    #   Returns:
    #     測定タイミングパジェット(μs)

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

  def set_vcsel_purse_period(type, period_pclks)
    # 指定した同期タイプの垂直共振器面発光レーザー(VCSEL)のパルス周期(PCLK)を指定
    #   Parameters:
    #     type : 周期タイプ  0 (VcselPeriodPreRange)
    #                        1 (VcselPeriodFinalRange)
    #     period_pclks : 指定する周期(PCLK)

    vcsel_period_reg = ((period_pclks) >> 1) - 1
    init_sequence_step_enables
    init_sequence_step_timeouts

    if type == 0
      case period_pclks
      when 12
        write_reg(0x57, 0x18)
      when 14
        write_reg(0x57, 0x30)
      when 16
        write_reg(0x57, 0x40)
      when 18
        write_reg(0x57, 0x50)
      else
        return false
      end

      write_reg(0x56, 0x08)

      write_reg(0x50, vcsel_period_reg)

      new_pre_range_timeout_mclks = timeout_microseconds_to_mclks(@msrc_dss_tcc_us, period_pclks)
      write_reg(0x46, new_pre_range_timeout_mclks > 256 ? 255 : new_pre_range_timeout_mclks - 1)

    elsif type == 1
      case period_pclks
      when 8
        write_reg(0x48, 0x10)
        write_reg(0x47, 0x08)
        write_reg(0x32, 0x02)
        write_reg(0x30, 0x0c)
        write_reg(0xff, 0x01)
        write_reg(0x30, 0x30)
        write_reg(0xff, 0x00)
      when 10
        write_reg(0x48, 0x28)
        write_reg(0x47, 0x08)
        write_reg(0x32, 0x03)
        write_reg(0x30, 0x09)
        write_reg(0xff, 0x01)
        write_reg(0x30, 0x20)
        write_reg(0xff, 0x00)
      when 12
        write_reg(0x48, 0x38)
        write_reg(0x47, 0x08)
        write_reg(0x32, 0x03)
        write_reg(0x30, 0x08)
        write_reg(0xff, 0x01)
        write_reg(0x30, 0x20)
        write_reg(0xff, 0x00)
      when 14
        write_reg(0x48, 0x48)
        write_reg(0x47, 0x08)
        write_reg(0x32, 0x03)
        write_reg(0x30, 0x07)
        write_reg(0xff, 0x01)
        write_reg(0x30, 0x20)
        write_reg(0xff, 0x00)
      else
        return false
      end

      write_reg(0x70, vcsel_period_reg)

      new_final_range_timeout_mclks = timeout_microseconds_to_mclks(@final_range_us, period_pclks)

      new_final_range_timeout_mclks += @pre_range_mclks if @pre_range

      write_reg_16bit(0x71, encode_timeout(new_final_range_timeout_mclks))

    else
      return false
    end

    set_measurement_timing_budget(@measurement_timing_budget_us)
    sequence_config = read_reg(0x01)
    write_reg(0x01, 0x02)
    perform_single_ref_calibration(0x0)
    write_reg(0x01, sequence_config)

    true
  end

  def get_vcsel_pulse_period(type)
    # 指定した周期タイプの垂直共振器面発光レーザ(VCSEL)のパルス周期(PCLK)を取得
    #   Parameters:
    #     type : 周期タイプ  0 (VcselPeriodPreRange)
    #                        1 (VcselPeriodFinalRange)
    #   Returns:
    #     パルス周期(PCLK)

    if type == 0
      (read_reg(0x50) + 1) << 1
    elsif type == 1
      (read_reg(0x70) + 1) << 1
    else
      255
    end
  end

  def start_continuous(period_ms = 0)
    # 連続距離測定を開始
    #   period_msが0なら, できる限り高レートで測定を行う
    #   0以外なら測定間隔を指定した時間にして測定を行う
    #   Parameters:
    #     period_ms : 測定間隔(ms)

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

  def stop_continuous
    # 連続距離測定を停止

    write_reg(0x00, 0x01)
    write_reg(0xFF, 0x01)
    write_reg(0x00, 0x00)
    write_reg(0x91, 0x00)
    write_reg(0x00, 0x01)
    write_reg(0xFF, 0x00)
  end

  def read_range_continuous_millimeters
    # 連続距離測定時の測定値を取得
    #   Returns:
    #     測定値(mm)

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
    # 単発距離測定を実行し, 測定値を取得
    #   Returns:
    #     測定値(mm)

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
    # タイムアウト期間(センサが準備できていない場合に読み取り操作が中止されるまでの期間)を設定
    # 0のときタイムアウトを無効
    #   Parameters:
    #     timeout : タイムアウト期間(ms)

    @io_timeout = timeout
  end

  def get_timeout
    # タイムアウト期間を取得
    #   Returns:
    #     タイムアウト期間(ms)

    @io_timeout
  end

  def start_timeout
    # タイムアウトの開始

    @timeout_start_ms = self.millis
  end

  def timeout_occurred
    # 前回のtimeout_occurredが呼び出された後にタイムアウトが発生したか取得
    #   Returns:
    #     true  タイムアウトが発生
    #     false タイムアウトが未発生

    tmp = @did_timeout
    @did_timeout = false
    tmp
  end

  def get_spad_info
    # SPAD(single photon avalanche diode)の数とタイプを取得
    #   Returns:
    #     配列[bool(取得の成功失敗), SPADの数, SPADのタイプ]

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
    type_is_aperture = (tmp >> 7) & 0x01 != 0

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
    # シーケンスステップの可能, 不可能を取得, 設定

    sequence_config = read_reg(0x01)
    @tcc = ((sequence_config >> 4) & 0x01) != 0
    @dss = ((sequence_config >> 3) & 0x01) != 0
    @msrc = ((sequence_config >> 2) & 0x01) != 0
    @pre_range = ((sequence_config >> 6) & 0x01) != 0
    @final_range = ((sequence_config >> 7) & 0x01) != 0
  end

  def init_sequence_step_timeouts
    # シーケンスステップのタイムアウトを取得, 設定

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
    # シーケンスステップのタイムアウトを算出
    #   Parameters:
    #     reg_val : レジスタ値
    #   Returns:
    #     タイムアウト(MCLKS)

    ((reg_val & 0x00ff) << ((reg_val & 0xff00) >> 8)) + 1
  end

  def encode_timeout(timeout_mclks)
    # MCLKS単位のタイムアウトからレジスタ値をエンコード
    #   Parameters:
    #     timeout_mclks : タイムアウト(MCLKS)
    #   Returns:
    #     レジスタ値(8bit)

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
    # VCSELの周期をPCLKで指定し, シーケンスステップのタイムアウトをMCLKSからμsに変換
    #   Parameters:
    #     timeout_period_mclks : タイムアウト(MCLKS)
    #     vcsel_period_pclks : VCSELの周期(PCLK)
    #   Returns:
    #     タイムアウト(μs)

    macro_period_ns = calc_macro_period(vcsel_period_pclks)
    ((timeout_period_mclks * macro_period_ns) + 500) / 1000
  end

  def timeout_microseconds_to_mclks(timeout_period_us, vcsel_period_pclks)
    # VCSELの周期をPCLKで指定し, シーケンスステップのタイムアウトをμsからMCLKSに変換
    #   Parameters:
    #     timeout_period_us : タイムアウト(μs)
    #     vcsel_period_pclks : VCSELの周期(PCLK)
    #   Returns:
    #     タイムアウト(MCLKS)

    macro_period_ns = calc_macro_period(vcsel_period_pclks)
    ((timeout_period_us * 1000) + (macro_period_ns / 2)) / macro_period_ns
  end

  def perform_single_ref_calibration(vhv_init_byte)
    # 校正を行う
    #   Parameters:
    #     vhv_init_byte : byte指定
    #   Returns:
    #     true  成功
    #     false 失敗

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
