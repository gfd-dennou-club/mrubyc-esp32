#!/usr/bin/env ruby
# coding: utf-8

require 'serialport'
require 'net/http'
require 'uri'
require 'json'

#データパケット送信時間
#ゲートウェイのノード番号
#自身のノード番号
#マージン時間
WAITTIME = 5587
GWNODENUM = 7
MARGINTIME = 0

#インターバル時間の計算
#interval_time = ( GWNODENUM - NODENUM - 1 ) * ( WAITTIME + MARGINTIME ) 

#ポートの設定
serialport = '/dev/serial0'

#シリアルポートオープン
#ボーレート 115200bps
#データ長 8bit
#ストップビット 1bit
begin 
  sp = SerialPort.new(serialport, 115200, 8, 1, 0)
  p "ok"
rescue
  p "could not open serial port."
  exit false
end

#1000ミリ秒待ってデータがこない場合タイムアウト
#2つのスレッドをスムーズに行うためにできるだけ小さい値に設定
sp.read_timeout = 1000

p "sleep"
sleep 10

interval_time = 1

#受け取ったデータを格納するための配列
data = Array.new(20){Array.new()}
i = 0
array = []

#スレッド1
#データを受け取り続ける
t1 = Thread.new do
  while true
    begin
         
      #データを受け取る
      #chompで末尾の改行コードを削除
      line = sp.readline.chomp
      line.force_encoding('UTF-8')
      line = line.encode('UTF-8', :invalid => :replace, :undef => :replace, :replace => '?')
      p line
      rssi = line[2,2]
     # rssi = line[0,4]
      p rssi

      #受信したデータの処理
      if line =~ /Rc/
        rcvdata = /Rc/.match(line)
        rcvdata = rcvdata.post_match
        #Receive Dataのあとの()を削除
        rcvdata = rcvdata.gsub(/[\(\)]/,'')
        array = rcvdata.split(",")
#        prevnode = array[5]
        prevnode = array[3]
#        p prevnode
        #prevnodeに自ノードが含まれていたら破棄
        unless ( prevnode =~ /#{GWNODENUM}/)
          data[i] = rcvdata.split(",")
          #受け取ったときの時間を格納
          data[i][5] = Time.now

          data[i][2] << "#{rssi}:"
          p ""
          p "受信データ"
          p data
          i = i + 1
        end
      end

    rescue EOFError
      next
    end
  end
end

#スレッド2
#受け取ったデータの処理
t2 = Thread.new do
  while true
    #最初の配列にデータあれば処理開始
    if data[0][0] != nil 
      flag = true
#      if (data[0][6] != nil && data[0][0].slice(4,8) != nil)
      if (data[0][5] != nil && data[0][0] != nil)
        time = data[0][5]
#        node = data[0][0].slice(4,8)
        node = data[0][0]
#      p time
#      p node
      end
    
      if time != nil
        #インターバル時間中にデータを受け取るか
        #受け取ったらデータを破棄
        while
          t_now = Time.now
          t_delta = t_now - time
#         p t_now
#         p t_delta

          #配列内を見て受け取ったデータと同じデータがあれば削除
          #最初に受け取ったデータは消したくないため飛ばす
          data.each_with_index do |str,j|
            if j == 0
              next
            end
            str.each_with_index do |elm,k|
              if (elm =~ /#{node}/)
                 puts " "
#                if k == 0
#                  p data
#                end
                data.delete(str)
                data.push(Array.new())
#                p data
                flag = false
                i = i - 1
#                p i
                next
              end
            end
          end 
         
          #インターバル時間経過したか
          if t_delta > interval_time
#            p "インターバル時間経過"
            puts " "
            break
          end
#         sleep 5
        end
      end

      #インターバル時間内に同じデータを受け取っていないときデータを送信
      if (flag )
        #受信データの末尾に自ノード番号を追加
#        if data[0][5]
#          data[0][5] << "#{GWNODENUM}"
#        else
#          data[0][5] = GWNODENUM
#        end
        if(data[0][0]!="2011")
        p "データベースに保存"
        hash = {}
#        hash[:'rssi'] = (data[0][0].slice(0,4)).to_i(16) #add rssi
#        hash[:'id'] = data[0][0].slice(4,8)              #add id
        hash[:'id'] = data[0][0]              #add id
        hash[:'gpstime'] = data[0][1]                    #add gpstime
        hash[:'rssi'] = data[0][2]                   #add 
        hash[:'route'] = data[0][3]                      #add 重複検出の番号
        hash[:'receiveT'] = data[0][5]                      #add 
        jhash = hash.to_json
        p jhash

        url = 'http://10.164.5.195/multiB.php'
        escapedurl = URI.escape(url)
        uri = URI.parse(escapedurl)
        http = Net::HTTP.new(uri.host, uri.port)
        http.use_ssl = url.start_with?('https')?true:false
        req = Net::HTTP::Post.new(uri.request_uri)
        req["Content-Type"] = "application/json"
        req.body = jhash
        res = http.request(req)
        #puts "code -> #{res.code}"
        #puts "msg -> #{res.message}"
        #puts "body -> #{res.body}"
        #http request
        end 
        p "送信データ"
        p "RC#{data[0][0]},#{data[0][1]},#{data[0][2]},#{data[0][3]},chk"
        sp.print "Rc#{data[0][0]},#{data[0][1]},#{data[0][2]},#{data[0][3]},chk,"
        sp.print "\r\n"

        p ""
#        p data
      end

      #データを削除  
      data.delete_at(0)
      data.push(Array.new())
      i = i - 1
#      p i
#      p data
    else 
      next
    end 
  end
end

#スレッドの実行
t1.join
t2.join

#シリアルポートクローズ
sp.close

