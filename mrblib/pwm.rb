# coding: utf-8
class PWM

  # 定数
  # タイマーの精度とスピードモードは決め打ち
  # esp-idf/components/driver/include/driver/ledc.h 参照
  LEDC_TIMER_10_BIT = 10
  LEDC_HIGH_SPEED_MODE = 0
  LEDC_DEFAULT_FREQUENCY = 440
  LEDC_DEFAULT_DUTY = 0
  
  # 初期化
  def initialize(pin, ch=0, freq = PWM::LEDC_DEFAULT_FREQUENCY, duty = PWM::LEDC_DEFAULT_DUTY)
    @pin = pin
    @ch = ch
    @freq = freq
    @duty = duty
   
    # タイマーの初期化
    ledc_timer_config(
      PWM::LEDC_TIMER_10_BIT,
      PWM::LEDC_DEFAULT_FREQUENCY,
      PWM::LEDC_HIGH_SPEED_MODE
    )
    
    # PWM の初期化
    ledc_channel_config(
      @ch,
      @pin, 
      PWM::LEDC_HIGH_SPEED_MODE
    )
  end

  # デューティー比の設定
  def duty( duty )
    @duty = duty
    ledc_timer_config(
      PWM::LEDC_TIMER_10_BIT,
      @freq,
      PWM::LEDC_HIGH_SPEED_MODE
    )
    ledc_set_duty(PWM::LEDC_HIGH_SPEED_MODE, @ch, @duty)
    ledc_update_duty(PWM::LEDC_HIGH_SPEED_MODE, @ch)

    puts "Frequency is already set : #{@freq}"
    puts "Set Duty : #{@duty}"
  end

  # 周波数の設定
  def frequency( freq )
    @freq = freq
    ledc_timer_config(
      PWM::LEDC_TIMER_10_BIT,
      @freq,
      PWM::LEDC_HIGH_SPEED_MODE
    )
    ledc_set_duty(PWM::LEDC_HIGH_SPEED_MODE, @ch, @duty)
    ledc_update_duty(PWM::LEDC_HIGH_SPEED_MODE, @ch)

    puts "Set Frequency : #{freq}"
    puts "Duty is already set: #{@duty}"
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
  def deinit()
    ledc_stop(PWM::LEDC_HIGH_SPEED_MODE, @ch, 0)
    puts "Stop PWM"
  end

end
