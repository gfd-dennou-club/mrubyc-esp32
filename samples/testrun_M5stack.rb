# coding: utf-8
# 概要 M5stack を用いた SPI チェック

def toc(color)
  r = color[0]
  g = color[1]
  b = color[2]
  ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3)
end

spi = SPI.new(miso_pin:19, mosi_pin:23, clk_pin:18)

sdspi = SDSPI.new(spi, cs_pin:4)
sdspi.mount("/sdcard")

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
  while (text = f3.gets)
    puts text
  end
  f3.close

  puts "-------------"
  # rename bar.txt to piyo.txt
  File.rename("/sdcard/herohero.txt", "/sdcard/hogehero.txt")
  
  puts DIRENT.childrenFiles("/sdcard")
  puts ""
  puts DIRENT.childrenDirs("/sdcard")
  puts ""
  puts DIRENT.children("/sdcard")
  puts ""
  
  puts "-------------"
  File.delete("/sdcard/herohero.txt")
  puts DIRENT.childrenFiles("/sdcard")
  puts ""

  puts "-----old style-----"

  fid = STDIO.fopen("/sdcard/herohero.txt", "r")
  cont =STDIO.fgets(fid,64)
  puts cont
  cont =STDIO.fgets(fid,64)
  puts cont
  STDIO.fclose(fid)

  # TEST Deleting files, delete the file of given file name.
  STDIO.remove("/sdcard/bar.txt")
  STDIO.remove("/sdcard/piyo.txt")

  # TEST Opening open.txt.
  puts "open foo.txt"
  # Open with fopen. r is read mode, pass file name and mode.
  fid = STDIO.fopen("/sdcard/foo.txt", "r")
  # Reading contents of file with fgets, second argument is maximum length of reading.
  cont = STDIO.fgets(fid, 64)
  puts cont
  # Closing file with fclose.
  STDIO.fclose(fid)
  
  # TEST Creating bar.txt and write in.
  puts "\nHey, create bar.txt and write down"
  # w is write mode
  fid = STDIO.fopen("/sdcard/bar.txt", "w")
  # Writing in with fputs.
  STDIO.fputs(fid, "Hello mruby/c!")
  # fputc writes only first character.
  STDIO.fputc(fid, "?")
  STDIO.fclose(fid)
  
  # TEST Renaming files.
  puts "\nYo! rename bar.txt to piyo.txt and read it!"
  # rename bar.txt to piyo.txt
  STDIO.rename("/sdcard/bar.txt", "/sdcard/piyo.txt")
  
  # Check piyo.txt is available.
  fid = STDIO.fopen("/sdcard/piyo.txt", "r")
  # Should print "Hello mruby/c!?"
  cont =STDIO.fgets(fid,64)
  
  # TEST Seeking
  puts "\n Let's fseek!"
  # Moving cursor to 5th character(It's 4 counting from 0). Second argument, 0 is from-head, 1 is from-cursor, 2 is from-last.
  STDIO.fseek(fid, 4, 0)
  # Where is current position?(should 4)
  pos = STDIO.fgetpos(fid)
  puts "position is ", pos
  puts "\n fgetc, Yah!"
  # Should print 'o', it's 5th character of 'Hello'.
  puts STDIO.fgetc(fid)
  puts cont
  STDIO.fclose(fid)
  
  # Let's enumearate child directories and files.
  puts "\nGet ls /sdcard!"
  ls = DIRENT.children("/sdcard")
  p ls
  
  # Let's check created date of file.
  puts "\nWhen created? Check it now!"
  t = DIRENT.fileTime("/sdcard/piyo.txt")
  puts "Year:", t.getYear()
  puts "Month:", t.getMonth()

  sleep 0.1

end

# Clear up.
sdspi.umount()

