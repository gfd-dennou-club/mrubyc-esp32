class ILI934X
    def initialize(mosi, clk, cs, dc, rst, bl) # add:ILI9342C on M5Stack
      @width = 320
      @height = 240
      @dc = GPIO.new(dc, GPIO::OUT)
      @spi = SPI.new(mosi, mosi, clk, cs, dc, rst, bl)
      init()
      GPIO.new(bl, GPIO::OUT, -1, 1) if bl >= 0
    end

    def write_command(data)
        @dc.write(0)
        @spi.write(data)
    end
    
    def write_data(*data)
        @dc.write(1)
        @spi.write(data)
    end

    def init()
      write_command(0xc0)
      write_data(0x23)

      write_command(0xc1)
      write_data(0x10)

      write_command(0xc5)
      write_data(0x3e, 0x28)

      write_command(0xc7)
      write_data(0x86)

      write_command(0x36)
      write_data(0x08)  # Memory Access Control

      write_command(0x3a)
      write_data(0x55)  # Pixel Format

      write_command(0x21)

      write_command(0xb1)
      write_data(0x00, 0x18)

      write_command(0xb6)
      write_data(0x08, 0xa2, 0x27, 0x00) # Display Function Control

      write_command(0x26)
      write_data(0x01)

      write_command(0xE0) # Positive Gamma Correction
      write_data(0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00)

      write_command(0xE1) # Negative Gamma Correction
      write_data(0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F)

      write_command(0x11) # Sleep Out
      sleep 0.12

      write_command(0x29) # Display ON
    end

    def drawString(x, y, str, height = 12, color = [0, 0, 0], margin_x = 3, margin_y = 3)
      home_x = x
      ch = 0
      size = 0
      str.each_char do |char|
        if char == "\n"
          x = home_x
          y += height
          y += margin_y
          next
        end
        c = char.ord
        cnt = 0
        (-7..-4).each do |i|
          if((c & 1 << (-i)) != 0)
            cnt += 1
          else
            break
          end
        end
        if cnt > 1
          size = cnt
          ch = c & 0xff >> (cnt + 1)
          next
        elsif cnt == 1
          ch <<= 6
          ch |= (c & 0b111111)
          size -= 1
          next if size != 1
        else
          ch = c
        end
        x += SPI.__draw_char(x, y, ch, toc(color), height)
        x += margin_x
      end
    end

    def draw_pixel(x, y, color)
      return if x < 0 || x > 320 || y < 0 || y > 240
      SPI.__draw_pixel(x, y, toc(color))
    end

    def draw_rectangle(x1, y1, x2, y2, color)
      draw_line(x1, y1, x2, y1, color)
      draw_line(x2, y1, x2, y2, color)
      draw_line(x2, y2, x1, y2, color)
      draw_line(x1, y2, x1, y1, color)
    end

    def draw_fillrectangle(x1, y1, x2, y2, color)
      SPI.__draw_fillrectangle(x1, y1, x2, y2, toc(color))
    end

    def draw_circle(x, y, r, color)
      SPI.__draw_circle(x, y, r, toc(color))
    end

    def draw_fillcircle(x, y, r, color)
      SPI.__draw_fillcircle(x, y, r, toc(color))
    end

    def fill(color)
      draw_fillrectangle(0, 0, @width, @height, color)
    end

    def draw_line(x1, y1, x2, y2, color, weight = 1)
      SPI.__draw_line(x1, y1, x2, y2, toc(color))
    end

    def toc(color)
      r = color[0]
      g = color[1]
      b = color[2]
      ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3)
    end
end

# ILI934X Extend Methods(not in class becouse of memory issue)

# def drawPoint(display, x, y, color, weight = 1)
#   x1 = x - weight / 2
#   x2 = x + weight / 2
#   y1 = y - weight / 2
#   y2 = y + weight / 2
#   display.drawRectangle(x1, x2, y1, y2, color)
# end