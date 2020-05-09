# coding: utf-8-hfs
class PWM

  # 定数
  # タイマーの精度とスピードモードは決め打ち
  # esp-idf/components/driver/include/driver/ledc.h 参照
  LEDC_TIMER_8_BIT = 8
  LEDC_HIGH_SPEED_MODE = 0
  LEDC_DEFAULT_FREQUENCY = 5000

  # 初期化
  def initialize(pin, ch)
    @pin  = pin
    unless(ch)
      @ch = 0
    else
      @ch = ch
    end

    # 定義済みのチャンネルを取ってこれれば引数にチャンネルを与えなくてよくなるのだが.
    # 後日検討.
    
    # タイマーの初期化
    LEDC.timer_config(
      LEDC_TIMER_8_BIT,
      LEDC_DEFAULT_FREQUENCY,
      LEDC_HIGH_SPEED_MODE
    )

    # PWM の初期化
    LEDC.channel_config(
      @ch,
      @pin, 
      LEDC_HIGH_SPEED_MODE
    )
  end
  
  # デューティー比の設定
  def duty( duty )    
    LEDC.set_duty(LEDC_HIGH_SPEED_MODE, @ch, duty)
    LEDC.update_duty(LEDC_HIGH_SPEED_MODE, @ch)

    puts "Set Duty : #{duty}"
  end

  # 周波数の設定
  def freq( freq )
    LEDC.set_duty(LEDC_HIGH_SPEED_MODE, @ch, duty)
    puts "Set Frequency : #{freq}"
  end

end
