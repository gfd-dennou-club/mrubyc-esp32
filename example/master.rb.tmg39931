i2c = I2C.new(22,21)
tmg = TMG39931.new(i2c)

if !tmg.init
  puts 'Device not found. Check wiring.'
else
  tmg.setup_recommended_config_for_proximity
  tmg.set_proximity_interrupt_threshold(25, 150) # less than 5cm sill trigger the proximity event
  tmg.set_adc_integration_time(0xdb) # the integration time: 103ms
  tmg.enable_engines(TMG39931::ENABLE[:PON] | TMG39931::ENABLE[:PEN] | TMG39931::ENABLE[:PIEN] | TMG39931::ENABLE[:AEN] | TMG39931::ENABLE[:AIEN])
  last_interrupt_state = -1
  while true
    if (tmg.get_status & (TMG39931::STATUS[:PINT] | TMG39931::STATUS[:AVALID])) != 0
      proximity_raw = tmg.get_proximity_raw  # read the Proximity data will clear the status bit
      if proximity_raw >= 150 && last_interrupt_state != 1
        puts 'Proximity detected!!!'
        puts "Proximity Raw: #{proximity_raw}"
        data = tmg.get_rgbc_raw
        lux = tmg.get_lux(data)
        cct = tmg.get_cct(data)
        puts '-----'
        puts "RGBC Data: #{data[:r]} #{data[:g]} #{data[:b]} #{data[:c]}"
        puts "Lux: #{lux}   CCT: #{cct}"
        puts '-----'
        last_interrupt_state = 1
      elsif proximity_raw <= 25 && last_interrupt_state != 0
        puts 'Proximity removed!!!'
        puts "Proximity Raw: #{proximity_raw}"
        last_interrupt_state = 0
      end
      # don't forget to clear the interrupt bits
      tmg.clear_proximity_interrupts
    end
    sleep 0.01
  end
end
