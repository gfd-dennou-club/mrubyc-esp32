###
### Wi-Fi
###

# Wi-Fi へアクセス
wlan = WLAN.new()

puts "--- Wi-Fi Scan Start ---"
ap_list = wlan.scan()

if ap_list
  ap_list.each do |ap|
    puts "SSID: #{ap["ssid"]}, RSSI: #{ap["rssi"]}, AuthMode: #{ap["authmode"]}"
  end
else
  puts "Scan failed or no AP found."
end
puts "------------------------"


# 接続
#wlan.connect("ssid", "pass")
#wlan.connect_peap("ssid", "user", "pass")

# 確認
puts "connected: #{wlan.connected?}"
puts "ifconfig: #{wlan.ifconfig}"
puts "mac: #{wlan.mac}"
puts "ip: #{wlan.ip}"

# Web ページ表示
puts HTTP.get("https://www.gfd-dennou.org/")

# 同期
Time.sync_ntp("ntp.nict.jp")  
#Time.sync_ntp()  #デフォルトの NTP サーバに接続する場合はサーバ名を省略可

loop do
  puts "時刻更新"
  time = Time.now
  puts time.datetime
  p time.year, time.mon, time.mday, time.wday, time.hour, time.min, time.sec, time.msec
  sleep 10
end


