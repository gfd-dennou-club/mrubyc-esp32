# coding: utf-8-hfs
class ADC

  # 定数
  # esp-idf/components/driver/include/driver/adc.h 参照
  ATTEN_0DB   = 0
  ATTEN_2DB   = 1
  ATTEN_6DB   = 2
  ATTEN_11DB  = 3

  WIDTH_9BIT  = 0
  WIDTH_10BIT = 1
  WIDTH_11BIT = 2
  WIDTH_12BIT = 3
  
  # 初期化
  def initialize(pin, atten, width)
    CHANNEL_ALL = {
      # ADC1
      "GPIO36" => 0, "GPIO37" => 1, "GPIO38" => 2, "GPIO39" => 3,
      "GPIO32" => 4, "GPIO33" => 5, "GPIO34" => 6, "GPIO35" => 7,
      # ADC2
      "GPIO4" => 0,  "GPIO0"  => 1, "GPIO2"  => 2, "GPIO15" => 3,
      "GPIO13" => 4, "GPIO12" => 5, "GPIO14" => 6, "GPIO27" => 7,
      "GPIO25" => 8, "GPIO26" => 9
    }

    @channel = ADC::CHANNEL_ALL["GPIO#{pin}"]
    @atten = atten
    @width = width       
    @unit  = 1
    if ( pin < 30 )
      @unit = 2
    end

    if (@unit == 1)
      ADC.init_adc1( @channel, @atten, @width )
    elsif(@unit == 2)
      ADC.init_adc2( @channel, @atten, @width )
    end

    puts "*** adc.rb ***"
    puts "  gpio:    #{pin}"
    puts "  channel: #{@channel}"
    puts "  atten:   #{@atten}"
    puts "  width:   #{@width}"
    puts "  unit:    #{@unit}"
  end

  # 値の取得
  def read
    if (@unit == 1)
      ADC.read_adc1( @channel )
    elsif(@unit == 2)
      ADC.read_adc2( @channel, @width )
    end
  end  
end
