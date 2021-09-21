def vl53l0x_init(i2c)
  vl53l0x = VL53L0X.new(i2c)
  vl53l0x.init
  vl53l0x.set_timeout(500)
  vl53l0x.set_measurement_timing_budget(200_000)
  vl53l0x
end

def vl53l0x(i2c, display, vl53l0x)
  range = vl53l0x.read_range_single_millimeters
  puts range
  puts " TIMEOUT" if vl53l0x.timeout_occurred
  display.writeString(range.to_s, 50, 50, 20, 35, 28, ILI934X.color(0x27, 0x1c, 0x19), ILI934X.color(0xff, 0xf3, 0xec))
end

def menu(i2c, display)
  p "menu now"
  sleep 1
end

def env2_init(i2c)
  p 'env2 init'
  sht3x = SHT3X.new(i2c)
  bmp280 = BMP280.new(i2c)
  return bmp280, sht3x
end

def env2(i2c, display, bmp280, sht3x)
  temp_s = sht3x.readTemperature
  temp_b = bmp280.readTemperature
  pressure = bmp280.readTemperature
  p "#{temp_s} C"
  p "#{temp_b} C, #{temp_b} Pa" 
  sleep 1
end

swpins = [39, 38, 37]
switches = []
swpins.each do |swpin|
  switches << GPIO.new(swpin, GPIO::IN, GPIO::PULL_UP)
end

i2c = I2C.new(22, 21)
vl53l0x = nil
display = ILI934X.new(23, 18, 14, 27, 33, 32)
sleep 10
# (40...300).each do |y|
#   (30...200).each do |x|
#     display.drawPixel(x, y, ILI934X.color(0xff, 0xf3, 0xec))
#   end
# end
display.drawRectangle(50, 100, 30, 150, ILI934X.color(0xff, 0xf3, 0xec))
display.drawRectangle(100, 270, 30, 150, ILI934X.color(0xff, 0xf3, 0xec))
display.drawRectangle(50, 100, 150, 220, ILI934X.color(0xff, 0xf3, 0xec))
display.drawRectangle(100, 270, 150, 220, ILI934X.color(0xff, 0xf3, 0xec))
state = 1
bmp280 = nil
sht3x = nil
vl53l0x_setup = false
env2_setup = false

while true
  switches.each_with_index do |sw, i|
    if sw.read == 0
      state = i
    end
  end

  # switches.each do |sw|
  #   p sw.get_pin 
  # end
  sleep 0.01

  case state
  when 0
    if vl53l0x_setup
      vl53l0x(i2c, display, vl53l0x)
    else
      vl53l0x = vl53l0x_init(i2c)
      vl53l0x_setup = true
    end
  when 1
    vl53l0x_setup = false
    env2_setup = false
    menu(i2c, display)
  when 2
    if env2_setup
      env2(i2c, display, bmp280, sht3x)
    else
      bmp280, sht3x = env2_init(i2c)
      env2_setup = true
    end
  end
end
