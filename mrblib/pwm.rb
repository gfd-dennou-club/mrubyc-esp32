# coding: utf-8
class PWM

  # 定数
  # タイマーの精度とスピードモードは決め打ち
  # esp-idf/components/driver/include/driver/ledc.h 参照
  LEDC_TIMER_10_BIT = 10
  LEDC_HIGH_SPEED_MODE = 0
  LEDC_LOW_SPEED_MODE = 1
  LEDC_DEFAULT_FREQUENCY = 440
  LEDC_DEFAULT_DUTY = 0
  
  # 初期化
  def initialize( pin, ch = 0, num = -1)

    @pin = pin
    @rslv = PWM::LEDC_TIMER_10_BIT
    @ch   = ch
    @freq = PWM::LEDC_DEFAULT_FREQUENCY
    @duty = LEDC_DEFAULT_DUTY 

    @num = num
    if num == -1
      @num  = ch       
    end
    
    if num < 2
      @sp = PWM::LEDC_HIGH_SPEED_MODE 
    else
      @sp = PWM::LEDC_LOW_SPEED_MODE 
    end
    
    # タイマーの初期化
    ledc_timer_config(
      @rslv,
      @freq,
      @sp,
      @num
    )
    # PWM の初期化
    ledc_channel_config(
      @ch,
      @pin, 
      @sp,
      @num
    )
    
    puts "**** #{@ch} ****"
    puts " #{@rslv}, #{@freq}, #{@sp}, #{@num} "
    puts "***************"

  end

  # デューティー比の設定
  def duty( duty )
    @duty = duty
    ledc_set_duty_and_update(@sp, @ch, @duty)
    puts "Set Duty : #{@duty}"

    puts "**** #{@ch} ****"
    puts " #{@rslv}, #{@freq}, #{@sp}, #{@num} "
    puts "***************"

  end

  # 周波数の設定
  def frequency( freq )
    @freq = freq
    ledc_set_freq(@sp, @num, @freq)

    puts "Set Frequency : #{freq}"

    puts "**** #{@ch} ****"
    puts " #{@rslv}, #{@freq}, #{@sp}, #{@num} "
    puts "***************"

  end

  # 周波数の設定
  def freq( freq )
    frequency( freq )
  end

  # 周期の指定 (マイクロ秒単位)
  def period_us( time )
    freq2 = ( 1000000 / time ).to_i
    puts freq2
    frequency( freq2 )
  end

  # PWMを無効化
  def deinit(  )
    ledc_stop(@sp, @ch)
    puts "Stop PWM"

  end
end
