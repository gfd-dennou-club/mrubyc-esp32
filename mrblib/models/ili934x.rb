class ILI934X
    def initialize(mosi, clk, cs, dc, rst, bl) # add:ILI9342C on M5Stack
      @width = 320
      @height = 240
      @spi = SPI.new(mosi, mosi, clk, cs, dc, rst, bl)
      # return
      if(bl >= 0)
        @bl = GPIO.new(bl, GPIO::OUT)
      end
      init(bl)
    end

    def init(bl)
      @spi.write_command(0xc8)
      @spi.write_data(0xff,0x93,0x42)
      @spi.write_command(0xc0)
      @spi.write_data(0x12,0x12)
      @spi.write_command(0xc1)
      @spi.write_data(0x03)
      @spi.write_command(0xb0)
      @spi.write_data(0xe0)
      @spi.write_command(0xf6)
      @spi.write_data(0x00, 0x01, 0x01)
      @spi.write_command(0x36)
      @spi.write_data(0x08)  # Memory Access Control
      @spi.write_command(0x3a)
      @spi.write_data(0x55)  # Pixel Format
      @spi.write_command(0xb6)
      @spi.write_data(0x08,0x82,0x27)  # Display Function Control
      @spi.write_command(0xe0)
      @spi.write_data(0x00,0x0c,0x11,0x04,0x11,0x08,0x37,0x89,0x4c,0x06,0x0c,0x0a,0x2e,0x34,0x0f)  # Set Gamma
      @spi.write_command(0xe1)
      @spi.write_data(0x00,0x0b,0x11,0x05,0x13,0x09,0x33,0x67,0x48,0x07,0x0e,0x0b,0x2e,0x33,0x0f)  # Set Gamma
      @spi.write_command(0x11)

      sleep 0.12

      @spi.write_command(0x29)
      if(bl >= 0)
        @bl.write(1) # add:ILI9342C on M5Stack
      end
      @spi.write_command(0x21)
      sleep 1 
    end

    def writeChar(char, x, y, height = 7, color = self.color(0, 0, 0), background_color = nil)

      # Generated with TTF2BMH
      # Font MisakiGothic
      # Font Size: 8
      font_width = 3
      font_height = 7
      width = height / 7 * 3
      char_addr1 =
      {
        32 => [0,0,0], 33 => [0,47,0], 34 => [3,0,3], 35 => [63,18,63], 36 => [22,63,26], 37 => [18,8,36], 38 => [26,37,42], 39 => [2,1,0], 40 => [0,62,65], 41 => [65,62,0], 42 => [10,7,10], 43 => [8,62,8], 44 => [64,32,0], 45 => [8,8,8], 46 => [0,32,0], 47 => [16,8,4], 48 => [28,42,28], 49 => [36,62,32], 50 => [50,42,36], 51 => [34,42,20], 52 => [24,20,62], 53 => [46,42,18], 54 => [28,42,18], 55 => [2,58,6]
      }
      char_addr2 =
      {
        56 => [20,42,20], 57 => [36,42,28], 58 => [0,36,0], 59 => [64,36,0], 60 => [8,20,34], 61 => [20,20,20], 62 => [34,20,8], 63 => [2,41,6], 64 => [18,41,30], 65 => [62,9,62], 66 => [63,37,26], 67 => [30,33,33], 68 => [63,33,30], 69 => [63,37,33], 70 => [63,5,1], 71 => [30,33,57], 72 => [63,8,63], 73 => [33,63,33], 74 => [16,32,31], 75 => [63,4,59], 76 => [63,32,32], 77 => [63,6,63], 78 => [63,1,62], 79 => [30,33,30], 80 => [63,9,6]
      }
      char_addr3 =
      {
        81 => [30,33,94], 82 => [63,9,54], 83 => [34,37,25], 84 => [1,63,1], 85 => [63,32,63], 86 => [63,16,15], 87 => [63,24,63], 88 => [51,12,51], 89 => [3,60,3], 90 => [49,45,35], 91 => [0,127,65], 92 => [4,8,16], 93 => [65,127,0], 94 => [2,1,2], 95 => [64,64,64],96 => [0,1,2], 97 => [24,36,60], 98 => [63,36,24], 99 => [24,36,36], 100 => [24,36,63], 101 => [24,44,44], 102 => [4,63,5], 103 => [72,84,60], 104 => [63,4,56]
      }
      char_addr4 =
      {
         105 => [0,61,0], 106 => [64,61,0], 107 => [63,8,52], 108 => [1,63,0], 109 => [60,28,56], 110 => [60,4,56], 111 => [24,36,24], 112 => [124,36,24], 113 => [24,36,124], 114 => [60,8,4], 115 => [40,60,20], 116 => [4,62,36], 117 => [60,32,60], 118 => [60,16,12], 119 => [60,48,60], 120 => [36,24,36], 121 => [76,80,60], 122 => [36,52,44], 123 => [8,54,65], 124 => [0,127,0], 125 => [65,54,8], 126 => [4,12,8]
      }
      code = char.ord
      if code >= 32 && code <= 55 
        bitmap = char_addr1[code]
      elsif code <= 80
        bitmap = char_addr2[code]
      elsif code <= 104
        bitmap = char_addr3[code]
      elsif code <= 126
        bitmap = char_addr4[code]
      end

      width.times do |dx|
        height.times do |dy|
          i = dx * font_width / width
          j = dy * font_height / height 
          if bitmap[i] & (1 << j) != 0
            drawPixel(x + dx, y + dy, color)
          elsif background_color != nil
            drawPixel(x + dx, y + dy, background_color)
          end
        end
      end
    end

    def writeString(str, x, y, margin_x, margin_y, height = 7, color = self.color(0, 0, 0), background_color = nil)
      home_x = x
      str.each_char do |c|
        if c == "\n"
          x = home_x
          y += margin_y
        else
          writeChar(c, x, y, height, color, background_color)
          x += margin_x
        end
      end
    end

    def drawPixel(x, y, color)
      if x < 0 || x > 320 || y < 0 || y > 240
        return
      end
      set_block(x, x, y, y)
      @spi.write_command(0x2c)
      @spi.write_data_word(color)
    end

    def drawRectangle(x1, x2, y1, y2, color)
      set_block(x1, x2, y1, y2)
      @spi.write_command(0x2c)
      (x2 - x1).times do
        @spi.write_color(color, y2 - y1 + 1)
      end
    end

    def fill(color)
      drawRectangle(0, @width, 0, @height, color)
    end

    def set_block(x1, x2, y1, y2)
      @spi.write_command(0x2a)
      @spi.write_address(x1, x2)
      @spi.write_command(0x2b)
      @spi.write_address(y1, y2)
    end

    def self.color(r, g, b)
      return ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3)
    end
end

# ILI934X Extend Methods(not in class becouse of memory issue)
def drawLine(display, x1, x2, y1, y2, color, weight = 1)
  dx = (x2 - x1).abs
  dy = (y2 - y1).abs

  b = dx < dy 
  if dx < dy
    x1, x2, dx, y1, y2, dy = y1, y2, dy, x1, x2, dx
  end
  x1, x2, y1, y2 = x2, x1, y2, y1 if x2 < x1
  sy = (y2 > y1) ? 1 : -1

  e = -dx
  for x in x1..x2 do
    if !b
      drawPoint(display, x, y1, color, weight)
    else
      drawPoint(display, y1, x, color, weight)
    end
    e += 2 * dy
    if ( e >= 0 )
      y1 += sy
      e -= 2 * dx
    end
  end
end

def drawPoint(display, x, y, color, weight = 1)
  x1 = x - weight / 2
  x2 = x + weight / 2
  y1 = y - weight / 2
  y2 = y + weight / 2
  display.drawRectangle(x1, x2, y1, y2, color)
end