# coding: utf-8-hfs
class Pin

  # 定数
  OUT = 0
  IN  = 1
  PULL_UP = 2

  # 初期化
  def initialize(pin, inout, pullup)
    @pin = pin
    
    if (inout == OUT)
      GPIO.set_mode_output(@pin)
      puts "GPIO output mode #{@pin}"
    end
    
    if (inout == IN)
      GPIO.set_mode_input(@pin)
      puts "GPIO input mode #{@pin}"
      
      if (pullup == PULL_UP)
        GPIO.set_pullup(@pin)
        puts "GPIO pull_up #{@pin}"
      end
    end
  end

  # ピンを "on" (high) レベルに設定
  def on
    GPIO.set_level(@pin, 1)
    puts "turn on"
  end

  # ピンを "off" (low) レベルに設定
  def off
    GPIO.set_level(@pin, 0)
    puts "turn off"
  end

  # 値 0 または 1 を取得
  def value
    GPIO.get_level(@pin)
  end

end
