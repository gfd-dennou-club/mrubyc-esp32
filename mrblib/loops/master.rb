def tof_init(i2c, tof)
  tof = VL53L0X.new(i2c)
  tof.init
  tof.set_timeout(500)
  tof.set_measurement_timing_budget(200_000)
  tof
end

def tof(i2c, tof)
  puts tof.read_range_single_millimeters
  puts " TIMEOUT" if tof.timeout_occurred
end

def menu(i2c, display)
  p "menu now"
  sleep 1
end

def env2_init(i2c)
  p 'env2 init'
end

def env2(i2c, display)
  p "env2 now"
  sleep 1
end

swpins = [39, 38, 37]
switches = []
swpins.each do |swpin|
  switches << GPIO.new(swpin, GPIO::IN, GPIO::PULL_UP)
end

i2c = I2C.new(22, 21)
tof = nil
display = ILI934X.new(23, 18, 14, 27, 33, 32)
sleep 10
display.drawRectangle(20, 300, 170, 180, ILI934X.color(0x28, 0xad, 0x35))
state = 1
tof_setup = false
env2_setup = false

while true
  switches.each_with_index do |sw, i|
    if sw.read == 0
      state = i
    end
  end
  case state
  when 0
    if tof_setup
      tof(i2c, tof)
    else
      tof = tof_init(i2c, tof)
      tof_setup = true
    end
  when 1
    tof_setup = false
    env2_setup = false
    menu(i2c, display)
  when 2
    if env2_setup
      env2(i2c, display)
    else
      env2_init(i2c)
      env2_setup = true
    end
  end
end
