led = [
  GPIO.new( 13, GPIO::OUT ), 
  GPIO.new( 12, GPIO::OUT ),
  GPIO.new( 14, GPIO::OUT ),
  GPIO.new( 27, GPIO::OUT ),
  GPIO.new( 26, GPIO::OUT ),
  GPIO.new( 25, GPIO::OUT ),
  GPIO.new( 33, GPIO::OUT ),
  GPIO.new( 32, GPIO::OUT )
]
sw = [
  GPIO.new( 34, GPIO::IN ),
  GPIO.new( 35, GPIO::IN ),
  GPIO.new( 18, Pin::IN|GPIO::PULL_UP ),
  GPIO.new( 19, Pin::IN|GPIO::PULL_UP ),
]

puts "**************************"
puts "  GPIO Check (10 times) "
puts "**************************"

i = 0
loop do
  if sw[0].read == 1 or sw[1].read == 1 or sw[2].read == 1 or sw[3].read == 1
    (0..7).each do |k|
      led[k].write( i % 2 )
    end
  else
    (0..7).each do |k|
      led[k].write( 0 )
    end
  end

  i += 1
  sleep 1
end

