#cording: UTF-8
i2c = I2C.new(22, 21)
tmp007 = TMP007.new(i2c)

if (!tmp007.init(TMP007::CFG_16SAMPLE))
  puts "No sensor found"
else
  while true do
    objt = tmp007.read_obj_temp_c
    puts "Obj Temperature: #{objt} *C"
    diet = tmp007.read_die_temp_c
    puts "Die Temperature: #{diet} *C"
    # r = tmp007.readRawDieTemperature()
    # puts "Raw Die Temperature: #{r}"
    # v = tmp007.readRawVoltage()
    # v *= 156.25
    # v /= 1000
    # puts "Raw Voltage: #{v} uv"
    sleep(4)
  end
end
