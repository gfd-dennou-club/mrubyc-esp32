i2c = I2C.new(22, 21)
si7021 = Si7021.new(i2c)
if !si7021.begin
  puts "Did not find Si7021 sensor!"
else
  sw = false
  count = 0
  print "Heater State : "
  si7021.heater_status ? puts("ENABLED") : puts("DISENABLE")
  while true do
    puts "Humi : #{si7021.read_humid}"
    puts "Temp : #{si7021.read_temp}"
    sleep 1
    if count == 30
      sw = !sw
      # si7021.set_heater_level(Si7021::HEAT_LEVEL["LOWEST"])
      si7021.heater(sw)
      print "Heater State : "
      si7021.heater_status ? puts("ENABLED") : puts("DISENABLE")
      count = 0
    end
    count += 1
  end
end
