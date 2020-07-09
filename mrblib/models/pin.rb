# coding: utf-8-hfs
class Pin

  # 定数
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

  # for drive
  LOW_POWER = 8
  MED_POWER = 9
  HIGH_POWER = 10

  # for irq trigger
  IRQ_RISING     = 0b001
  IRQ_FALLING    = 0b010
  IRQ_LOW_LEVEL  = 0b100
  IRQ_HIGH_LEVEL = 0b101

  # 初期化
  def initialize(pin, mode = -1, pull_mode = -1, value = -1)
    @pin = pin
    @mode = mode
    
    init(@mode, pull_mode, value)
  end

  # コンストラクタ外からの再初期化
  def init(mode = -1, pull_mode = -1, value = -1)
    if(mode == Pin::OUT || mode == Pin::OPEN_DRAIN)
      if(value == 0)
        off()
      elsif(value == 1)
        on()
      end
    end
    set_mode(mode)
    set_pull_mode(pull_mode)
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

  # mode を設定
  def set_mode(mode)
    @mode = mode
    case @mode
    when Pin::OUT then
      GPIO.set_mode_output(@pin)
      puts "GPIO output mode #{@pin}"
    when Pin::IN then
      GPIO.set_mode_input(@pin)
      puts "GPIO input mode #{@pin}"
    when Pin::OPEN_DRAIN then
      GPIO.set_mode_open_drain(@pin)
      puts "GPIO open_drain mode #{@pin}"
    end
  end

  # pullmode を設定 hold を解除したければ "nil" を渡す
  def set_pull_mode(pull_mode)
    case pull_mode
    when Pin::PULL_UP then
      GPIO.set_pullup(@pin)
      puts "GPIO pull_up #{@pin}"
    when Pin::PULL_DOWN then
      GPIO.set_pulldown(@pin)
      puts "GPIO pull_down #{@pin}"
    when Pin::PULL_HOLD then
      if(mode == Pin::OUT || mode == Pin::OPEN_DRAIN)
        GPIO.set_hold_enable(@pin)
        puts "GPIO hold_enable #{@pin}"
      end
    else
      if(mode == Pin::OUT || mode == Pin::OPEN_DRAIN)
        GPIO.set_hold_disable(@pin)
        puts "GPIO hold_disable #{@pin}"
      end
    end
  end

  def irq(handler = nil, trigger = (Pin::IRQ_FALLING | Pin::IRQ_RISING))
    GPIO.set_intr_type(@pin, trigger)
    handler.(self)
    GPIO.isr_handler_add(@pin, handler, self)
  end
end
