$led1 = GPIO.new( 13, GPIO::OUT )
$led2 = GPIO.new( 12, GPIO::OUT )
$sw1  = GPIO.new( 34, GPIO::IN, GPIO::PULL_UP )
$sw2  = GPIO.new( 35, GPIO::IN, GPIO::PULL_UP )

$sw1.intr( GPIO::INTR_ANYEDGE )
$sw2.intr( GPIO::INTR_POSEDGE )

num = 0
while true
  $led1.write( num % 2 )
  $led2.write( num % 2 )
  sleep 10
  num += 1
end
