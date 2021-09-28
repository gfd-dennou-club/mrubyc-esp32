def vl53l0x_init(i2c)
  vl53l0x = VL53L0X.new(i2c)
  vl53l0x.init
  vl53l0x.set_timeout(500)
  vl53l0x.set_measurement_timing_budget(200_000)
  vl53l0x
end

def vl53l0x(i2c, display, vl53l0x)
  range = vl53l0x.read_range_single_millimeters
  puts "range: #{range}"
  puts " TIMEOUT" if vl53l0x.timeout_occurred
  display.writeString(sprintf("%4d", range), 50, 50, 20, 35, 28, ILI934X.color(0x27, 0x1c, 0x19), ILI934X.color(0xff, 0xf3, 0xec))
end

def menu(i2c, display)
  p "menu now"
  sleep 1
end

def env2_init(i2c)
  p 'env2 init'
  sht3x = SHT3X.new(i2c)
  p "sht3x init"
  bmp280 = BMP280.new(i2c)
  p "bmp280 init"
  return bmp280, sht3x
end

def env2(i2c, display, bmp280, sht3x)
  temp_s = sht3x.readTemperature
  temp_b = bmp280.readTemperature
  pressure = bmp280.readPressure
  p "#{temp_s} C"
  p "#{temp_b} C, #{pressure} Pa" 
  draw_back(display)
  str = sprintf("    %.2f\n    %.2f\n%d", temp_s, temp_b, pressure)
  display.writeString(str, 75, 75, 14, 31, 28, ILI934X.color(0x27, 0x1c, 0x19), nil)
  sleep 1
end

def draw_back(display)
  xs = [70, 130, 190, 250]
  ys = [70, 120, 170]
  3.times do |x|
    2.times do |y|
      display.drawRectangle(xs[x], xs[x + 1], ys[y], ys[y + 1], ILI934X.color(0xff, 0xf3, 0xec))
    end
  end
end

swpins = [39, 38, 37]
switches = []
swpins.each do |swpin|
  switches << GPIO.new(swpin, GPIO::IN, GPIO::PULL_UP)
end

i2c = I2C.new(22, 21)
vl53l0x = nil
display = ILI934X.new(23, 18, 14, 27, 33, 32)
sleep 5
state = 1
bmp280 = nil
sht3x = nil

while true
  switches.each_with_index do |sw, i|
    state = i if sw.read == 0
  end

  case state
  when 0
    if vl53l0x.nil?
      vl53l0x = vl53l0x_init(i2c)
    else
      vl53l0x(i2c, display, vl53l0x)
    end
  when 1
    bmp280 = vl53l0x = nil
    menu(i2c, display)
  when 2
    if bmp280.nil?
      bmp280, sht3x = env2_init(i2c)
    else
      env2(i2c, display, bmp280, sht3x)
    end
  end
end
