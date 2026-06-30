
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
puts "> gps.read_nonblock(4096, nonblock:1)"
#puts gps.read(4096, nonblock:1)
puts gps.read(4096)

# 入力データをclear_tx_bufferで消去する
puts "> gps.clear_tx_buffer"
gps.clear_tx_buffer

# こちらは消去されているため何も表示されない
puts "> gps.read(4096, nonblock:1)"
#puts gps.read(4096, nonblock:1)
puts gps.read(4096)

# ==============================================================================
# 実験1: ノンブロッキング (nonblock: 1) の挙動
# ==============================================================================
puts "--- 1. ノンブロッキング実験 ---"

# 受信バッファをクリアして空っぽにする
gps.clear_tx_buffer 

# データが無い瞬間に、ノンブロッキングで4096バイト読み込む
# => 待たずに一瞬で「空の文字列（""）」が返ってくるはずです
puts "ノンブロック読み込み開始..."
p gps.read(4096, nonblock: 1)
puts "ノンブロック読み込み終了（一瞬でここに来ます）"

# ==============================================================================
# 実験2: ブロッキング (通常) の挙動
# ==============================================================================
puts "\n--- 2. ブロッキング実験 ---"

# もう一度受信バッファをクリアして空っぽにする
gps.clear_tx_buffer 

# データが無い瞬間に、通常モードで4096バイト読み込む
# => ここでC言語側の「100msのタイムアウト待ち」が発生するため、
#    プログラムの動きが一瞬（0.1秒）カチッと止まります。
puts "ブロッキング読み込み開始（100ms待ちます）..."
p gps.read(4096)
puts "ブロッキング読み込み終了"

# 以下、到着したデータを 1 行ずつ読み込んで表示
puts "> gps.gets"
while true
  puts  gps.gets()
  sleep 1
end


