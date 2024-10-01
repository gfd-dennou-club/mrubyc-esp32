puts "*********************"
puts " UART check (GPS)    "
puts "*********************"

# GPSの電源を入れる (高専ボードの場合に必要)
gps_pw = GPIO.new(5, GPIO::OUT)
gps_pw.write(0)

#p "UART.new"
# GPS初期化 txPin = 17, rxPin = 16 のため uart_num = 2 とする
gps = UART.new(2, baudrate:9600)

#p "UART.write"
# 出力を RMS のみに
sleep 1
gps.write("$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n")

## 出力をデフォルトに戻すときは以下をコメントアウトすること．
## sleep 1
##gps.write("$PMTK314,-1*04\r\n")

# データの到着まで少し待つ
sleep 2

# read メソッドで 100 バイト分のデータを受け取る
# 指定したバイト数分のデータを受け取れない場合は nill が返る
puts "> gps.read"
p gps.read(100)

# 入力データをclear_tx_bufferで消去する
puts "> gps.clear_tx_buffer"
gps.clear_tx_buffer

# データの到着まで少し待つ
sleep 2

# 4096バイトのデータはないが、
# nonblockのため到着している分のデータが表示される
puts "> gps.read_nonblock(4096)"
puts gps.read(4096, nonblock:1)

# 入力データをclear_tx_bufferで消去する
puts "> gps.clear_tx_buffer"
gps.clear_tx_buffer

# こちらは消去されているため何も表示されない
puts "> gps.read(4096, nonblock:1)"
puts gps.read(4096, nonblock:1)

# 以下、到着したデータを 1 行ずつ読み込んで表示
loop do

  # 入力データをclear_tx_bufferで消去する
  gps.clear_tx_buffer

  # 入力データが来るのを待つ
  sleep 3

  # データ取得・表示
  lines = gps.read(4096, nonblock:1).split('$').pop
  puts "*** #{lines} ***"

  # 待ち
  sleep 7
end

