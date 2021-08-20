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
    end

    def drawPixel(x, y, color)
      if x < 0 || x > 320 || y < 0 || y > 240
      end
      set_block(x, x, y, y)
      @spi.write_command(0x2c)
      @spi.write_data_word(color)
    end

    def drawLine(x1, x2, y1, y2, color)
      dx = (x2 - x1).abs;
      dy = (y2 - y1).abs;

      sx = ( x2 > x1 ) ? 1 : -1;
      sy = ( y2 > y1 ) ? 1 : -1;

      if (dx > dy) 
        e = -dx;
        for x in x1..x2 do
          drawPixel(x, y1, color)
          e += 2 * dy
          if ( e >= 0 )
            y1 += sy
            e -= 2 * dx
          end
        end
      else
        e = -dy;
        for y in y1..y2 do
          drawPixel(x1, y, color)
          e += 2 * dx
          if ( e >= 0 )
            x1 += sx
            e -= 2 * dy
          end
        end
      end
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

=begin
    def writeChar(char, x, y, color = self.color(255, 255, 255), background = self.color(0, 0, 0))
      buffer = bytearray(8)
      framebuffer = framebuf.FrameBuffer1(buffer, 8, 8)
      framebuffer.text(char, 0, 0)
      color = ustruct.pack(">H", color)
      background = ustruct.pack(">H", background)
      data = bytearray(2 * 8 * 8)
      for c, byte in enumerate(buffer):
          for r in range(8):
              if byte & (1 << r)
                  data[r * 8 * 2 + c * 2] = color[0]
                  data[r * 8 * 2 + c * 2 + 1] = color[1]
              else
                  data[r * 8 * 2 + c * 2] = background[0]
                  data[r * 8 * 2 + c * 2 + 1] = background[1]
              end
            end
          end
      set_block(x, y, x + 7, y + 7)
      @spi.write_command(0x2c)
      @spi.write_data(data)
    end
=end

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