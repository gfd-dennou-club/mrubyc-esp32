def tof_init(i2c)
  p 'tof init'
end

def tof(i2c, display)
  p "tof now"
  sleep 1
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
display = ILI934X.new(23, 18, 14, 27, 33, 32)
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
      tof(i2c, display)
    else
      tof_init(i2c)
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
