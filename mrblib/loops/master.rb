def tof(i2c, display)
  p "tof now"
  sleep 1
end

def menu(i2c, display)
  p "menu now"
  sleep 1
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
display = ILI934X.new(23, 18, 14, 27, 33, 32)
state = 1

while true
  switches.each_with_index do |sw, i|
    if sw.read == 1
      state = i
    end
  end
  case state
  when 0
    tof(i2c, display)
  when 1
    menu(i2c, display)
  when 2
    env2(i2c, display)
  end
end