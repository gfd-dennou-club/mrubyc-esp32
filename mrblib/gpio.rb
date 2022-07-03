# coding: utf-8

class GPIO
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

  # for Interupt trigger
  INTR_DISABLE    = 0   # 割り込み無効
  INTR_POSEDGE    = 1   # 立ち上がりエッジ
  INTR_NEGEDGE    = 2   # 立ち下がりエッジ
  INTR_ANYEDGE    = 3   # 両エッジ
  INTR_LOW_LEVEL  = 4   # 入力Low割り込み
  INTR_HIGH_LEVEL = 5   # 入力High割り込み
  
  # 初期化
  def initialize(pin, mode = -1, pull_mode = -1, value = -1)
    @pin = pin
    
    init(mode, pull_mode, value)
  end

  # コンストラクタ外からの再初期化
  def init(mode = -1, pull_mode = -1, value = -1)
    gpio_reset_pin(@pin)
    setmode(mode)
    setpullmode(pull_mode)
    if(value != -1 && (mode == GPIO::OUT || mode == GPIO::OPEN_DRAIN))
      write(value)
    end
  end

  # 出力
  def write(value)
    unless(value == 0 || value == 1)
      puts "invalid value detected"
      return
    end
    gpio_set_level(@pin, value)
    puts "write #{value}"
  end
  
  # write(1) の別名
  def on
    write(1)
  end
  
  # write(0) の別名
  def off
    write(0)
  end
  
  # 入力 値 0 または 1 を取得
  def read
    gpio_get_level(@pin)
  end

  # read の別名
  def value
    gpio_get_level(@pin)
  end
  
  # 入出力方向設定
  def setmode(mode)
    @mode = mode
    case @mode
    when GPIO::OUT then
      gpio_set_mode_output(@pin)
      puts "GPIO output mode #{@pin}"
    when GPIO::IN then
      gpio_set_mode_input(@pin)
      puts "GPIO input mode #{@pin}"
    when GPIO::OPEN_DRAIN then
      gpio_set_mode_open_drain(@pin)
      puts "GPIO open_drain mode #{@pin}"
    end
  end

  # pullmode を設定 hold を解除したければ "nil" を渡す
  def setpullmode(pull_mode)
    case pull_mode
    when GPIO::PULL_UP then
      gpio_set_pullup(@pin)
      puts "GPIO pull_up #{@pin}"
    when GPIO::PULL_DOWN then
      gpio_set_pulldown(@pin)
      puts "GPIO pull_down #{@pin}"
    when GPIO::PULL_HOLD then
      if(@mode == GPIO::OUT || @mode == GPIO::OPEN_DRAIN)
        gpio_set_hold_enable(@pin)
        puts "GPIO hold_enable #{@pin}"
      end
    else
      if(@mode == GPIO::OUT || @mode == GPIO::OPEN_DRAIN)
        gpio_set_hold_disable(@pin)
        puts "GPIO hold_disable #{@pin}"
      end
    end
  end

  # 割り込み
  def intr(mode)
    puts "intr : #{mode}"
    gpio_set_intr(@pin, mode)
  end

  def intr_info
    array = gpio_isr_state
    
    intr = Hash.new
    intr['pin']  = array[0]
    intr['val']  = array[1]

    return intr
  end  

end
