# coding: utf-8
class PWM

  # 定数
  # タイマーの精度とスピードモードは決め打ち
  # esp-idf/components/driver/include/driver/ledc.h 参照
  LEDC_TIMER_8_BIT = 8
  LEDC_HIGH_SPEED_MODE = 0
  LEDC_DEFAULT_FREQUENCY = 440
  LEDC_DEFAULT_DUTY = 1 << (PWM::LEDC_TIMER_8_BIT - 1)
  
  # 音階の指定
  NOTE = {
    "NOTE_C" => 0,
    "NOTE_Cs" => 1,
    "NOTE_D" => 2,
    "NOTE_Eb" => 3,
    "NOTE_E" => 4,
    "NOTE_F" => 5,
    "NOTE_Fs" => 6,
    "NOTE_G" => 7,
    "NOTE_Gs" => 8,
    "NOTE_A" => 9,
    "NOTE_Bb" => 10,
    "NOTE_B" => 11
  }

  # 初期化
  def initialize(pin, ch=0, freq = PWM::LEDC_DEFAULT_FREQUENCY, duty = PWM::LEDC_DEFAULT_DUTY)
    if pin.kind_of?(Fixnum)
      @pin = pin
    elsif pin.kind_of?(Pin)
      @pin = pin.pin
    end

    @ch = ch

    # 定義済みのチャンネルを取ってこれれば引数にチャンネルを与えなくてよくなるのだが.
    # 後日検討.
    
    # タイマーの初期化
    LEDC.timer_config(
      PWM::LEDC_TIMER_8_BIT,
      PWM::LEDC_DEFAULT_FREQUENCY,
      PWM::LEDC_HIGH_SPEED_MODE
    )
    
    # PWM の初期化
    LEDC.channel_config(
      @ch,
      @pin, 
      PWM::LEDC_HIGH_SPEED_MODE
    )

    freq(freq)
    duty(duty)
  end

  # デューティー比の設定
  def duty( duty )
    @duty = duty
    LEDC.set_duty(PWM::LEDC_HIGH_SPEED_MODE, @ch, @duty)
    LEDC.update_duty(PWM::LEDC_HIGH_SPEED_MODE, @ch)

    puts "Set Duty : #{@duty}"
  end

  # 周波数の設定
  def freq( freq )
    LEDC.timer_config(
      PWM::LEDC_TIMER_8_BIT,
      freq,
      PWM::LEDC_HIGH_SPEED_MODE
    )
    LEDC.set_duty(PWM::LEDC_HIGH_SPEED_MODE, @ch, @duty)
    LEDC.update_duty(PWM::LEDC_HIGH_SPEED_MODE, @ch)
    puts "Set Frequency : #{freq}"
    puts "Duty is already set: #{@duty}"
  end

  # PWMを無効化
  def deinit()
    LEDC.stop(PWM::LEDC_HIGH_SPEED_MODE, @ch, 0)
    puts "Stop PWM"
  end

  # 音階とオクターブを指定して周波数を設定
  def ledc_write_note(note, octave)
    if(0 > octave && octave <= 8)
      deinit
    else
      freq(((27.5 * (2**((PWM::NOTE[note] + 12 * octave)/12.0))) + 0.5).to_i)
    end
  end
end
