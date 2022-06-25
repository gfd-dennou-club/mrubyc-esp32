# coding: utf-8
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

