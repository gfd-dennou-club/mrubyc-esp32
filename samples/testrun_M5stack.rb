###
### 概要 M5stack を用いた SPI チェック
###
def toc(color)
  r = color[0]
  g = color[1]
  b = color[2]
  ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3)
end

spi = SPI.new(miso_pin:19, mosi_pin:23, clk_pin:18)

sdspi = SDSPI.new(spi, cs_pin:4, mount_point:"/sdcard")

display = LCDSPI.new( spi, cs_pin:14, dc_pin:27, rst_pin:33, bl_pin:32 )


20.times do |i|

  p "#{i} : #{255-i*10}, #{i*10}, 0"
  display.rectangle(color:toc([0,i*10,255-i*10]))
  display.circle(x:160, y:120, r:40, color:toc([0, 0, 0]))
  sleep 1
  display.rectangle(x1:0, y1:0, x2:320, y2:240, color:toc([255-i*10,i*10,0]))
  display.line(x1:0, y1:0, x2:320, y2:240, color:toc([0, 255-i*10,0]))
  display.fillcircle(x:160, y:120, r:10, color:toc([0, 0, 0]))
  display.string("Hello World! \n  from ESP32 (mruby/c)", x:12, y:42, pointsize:20, color:toc([0, 0, 0]))
  display.string("Hello World! \n  from ESP32 (mruby/c)", x:10, y:40, pointsize:20, color:toc([255, 255, 255]))

  sleep 2
  
  f1 = File.open("/sdcard/herohero.txt", "w")
  f1.puts("Hello mruby/c! \n")
  f1.close

  f2 = File.open("/sdcard/herohero.txt", "a")
  f2.puts("Hi, mruby/c!\n")
  f2.close

  f2 = File.open("/sdcard/herohero.txt", "a")
  f2.puts("Kon-nichiwa, mruby/c!")
  f2.close

  puts "-----read-----"

  f4 = File.open("/sdcard/herohero.txt", "r") 
  puts f4.read
  f4.close

  puts "-----gets-----"
  
  f3 = File.open("/sdcard/herohero.txt", "r")
  i = 0
  while (text = f3.gets)
    puts "#{i}: #{text}"
    i += 1
  end
  f3.close

  puts "-------------"
  # rename bar.txt to piyo.txt
  File.rename("/sdcard/herohero.txt", "/sdcard/hogehero.txt")
  
  puts Dir.childrenFiles("/sdcard")
  puts ""
  puts Dir.childrenDirs("/sdcard")
  puts ""
  puts Dir.children("/sdcard")
  puts ""
  
  puts "-------------"
  File.delete("/sdcard/herohero.txt")
  puts Dir.childrenFiles("/sdcard")
  puts ""

  sleep 0.1

end

# Clear up.
sdspi.umount()

