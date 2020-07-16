callback = Proc.new { |p|
  print(p)
  puts("called")
}

led1 = Pin.new(13, Pin::OUT)
led1.irq(callback)

switch1 = Pin.new(34, Pin::IN)
switch2 = Pin.new(35, Pin::IN)

while true
  if(switch1.value == 1)
    led1.on
  else
    led1.off
  end
  if(switch2.value == 1)
    led1.set_pull_mode(Pin::PULL_HOLD)
  else
    led1.set_pull_mode(nil)
  end
  sleep 1
end
