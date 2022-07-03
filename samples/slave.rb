# coding: utf-8
###
### Hello World (with master)
### 
=begin 
i = 1
while true
  puts "hello world from ESP32 (slave) #{i}" 
  sleep 10

  if ( i % 2 == 0 )
    $msg = "hoge"
  else
    $msg = "peke"
  end
  
  i += 1
end
=end

###
### GPIO PIN or SWITCH (with slave)
###
=begin
#初期化
pre = {'pin' => 0, 'val' => 0}

while true
  now = GPIO.intr_info

  if now != pre
    if now['pin'] == 34
      $led1.write(now['val'])
    elsif now['pin'] == 35
      $led2.write(now['val'])
    end
  end
  sleep_ms 10

  pre = now
end
=end
