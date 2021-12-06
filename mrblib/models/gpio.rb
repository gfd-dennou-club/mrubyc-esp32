# coding: utf-8-hfs

# `クラス変数`が働かないためグローバル変数で実装する
$irq_instances = nil
class GPIO
  # 定数
  PIN_COUNT = 39
  # for mode
  OUT = 0
  IN  = 1
  OPEN_DRAIN = 2
  ALT = 3
  ALT_OPEN_DRAIN = 4

  # for pull_mode
  PULL_UP = 5
  PULL_DOWN = 6
  PULL_HOLD = 7

  # for irq trigger
  IRQ_RISING     = 0b0001
  IRQ_FALLING    = 0b0010
  IRQ_LOW_LEVEL  = 0b0100
  IRQ_HIGH_LEVEL = 0b1000

  # 初期化
  def initialize(pin, mode = -1, pull_mode = -1, value = -1)
    @pin = pin
    
    init(mode, pull_mode, value)
  end

  def get_pin
    @pin
  end

  # コンストラクタ外からの再初期化
  def init(mode = -1, pull_mode = -1, value = -1)
    GPIO.reset_pin(@pin)
    setmode(mode)
    setpullmode(pull_mode)
    if(value != -1 && (mode == GPIO::OUT || mode == GPIO::OPEN_DRAIN))
      write(value)
    end
  end

  # wakeupモードの設定
  # 第二引数は、起床するパワーモード(0 or 1)
  def set_wakeup(is_enable, level = 0)
    if(is_enable)
      puts "GPIO wakeup enable in #{@pin}"
      GPIO.wakeup_enable(@pin, level + 4)
    else
      puts "GPIO wakeup disable in #{@pin}"
      GPIO.wakeup_disable(@pin)
    end
  end

  # 出力 `ピンを0か1に設定`
  def write(value)
    unless(value == 0 || value == 1)
      puts "invalid value detected"
      return
    end
    GPIO.set_level(@pin, value)
    puts "write #{value}"
  end

  # 入力 値 0 または 1 を取得
  def read
    GPIO.get_level(@pin)
  end

  # 入出力方向設定
  def setmode(mode)
    @mode = mode
    case @mode
    when GPIO::OUT then
      GPIO.set_mode_output(@pin)
      puts "GPIO output mode #{@pin}"
    when GPIO::IN then
      GPIO.set_mode_input(@pin)
      puts "GPIO input mode #{@pin}"
    when GPIO::OPEN_DRAIN then
      GPIO.set_mode_open_drain(@pin)
      puts "GPIO open_drain mode #{@pin}"
    end
  end

  # pullmode を設定 hold を解除したければ "nil" を渡す
  def setpullmode(pull_mode)
    case pull_mode
    when GPIO::PULL_UP then
      GPIO.set_pullup(@pin)
      puts "GPIO pull_up #{@pin}"
    when GPIO::PULL_DOWN then
      GPIO.set_pulldown(@pin)
      puts "GPIO pull_down #{@pin}"
    when GPIO::PULL_HOLD then
      if(@mode == GPIO::OUT || @mode == GPIO::OPEN_DRAIN)
        GPIO.set_hold_enable(@pin)
        puts "GPIO hold_enable #{@pin}"
      end
    else
      if(@mode == GPIO::OUT || @mode == GPIO::OPEN_DRAIN)
        GPIO.set_hold_disable(@pin)
        puts "GPIO hold_disable #{@pin}"
      end
    end
  end

  # irq ハンドラーの設定
  def irq(handler = nil, trigger = (GPIO::IRQ_FALLING | GPIO::IRQ_RISING))
    $irq_instances = Array.new(GPIO::PIN_COUNT + 1) unless $irq_instances
    @handler = handler
    @trigger = trigger
    $irq_instances[@pin] = self
  end

  # `ハンドラーの発火 0.01秒に一度自動で実行される
  def __check_handler()
    if(GPIO.get_pin_state(@pin) & @trigger != 0)
      @handler.(self)
    end
  end
end
