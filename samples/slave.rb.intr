
while true
  info = gpio_isr_state[0]
  if info[0] == 1
    puts "Flag: #{info[0]}, Pin: #{info[1]}, State: #{info[2]}"
    $led2.write(info[2])
  end
  sleep_ms 10
end

