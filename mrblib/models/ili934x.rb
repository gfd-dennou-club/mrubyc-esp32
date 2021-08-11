# class ILI934X
#     COLUMN_SET = 0x2a
#     PAGE_SET = 0x2b
#     RAM_WRITE = 0x2c
#     RAM_READ = 0x2e
#     DISPLAY_ON = 0x29
#     WAKE = 0x11
#     LINE_SET = 0x37
#     MADCTL = 0x36
#     DISPLAY_INVERSION_ON = 0x21

#     _HEIGHT = 320
#     _WIDTH = 240

#     def initialize(mosi, clk, cs, dc, rst, bl) # add:ILI9342C on M5Stack
#       p "ili934x initializing..."
      
#       @spi = SPI.new(mosi, mosi, clk, cs, dc, rst, bl)
#       init(bl)
#     end

#     def init(bl)
#       operations = [
#         [0xc8, [0xff,0x93,0x42]],
#         [0xc0, [0x12,0x12]],
#         [0xc1, [0x03]],
#         [0xb0, [0xe0]],
#         [0xf6, [0x00, 0x01, 0x01]],
#         [0x36, [0x08]],  # Memory Access Control
#         [0x3a, [0x55]],  # Pi0xel Format
#         [0xb6, [0x08,0x82,0x27]],  # Display Function Control
#         [0xe0,  # Set Gamma
#           [0x00,0x0c,0x11,0x04,0x11,0x08,0x37,0x89,0x4c,0x06,0x0c,0x0a,0x2e,0x34,0x0f]],
#         [0xe1,  # Set Gamma
#           [0x00,0x0b,0x11,0x05,0x13,0x09,0x33,0x67,0x48,0x07,0x0e,0x0b,0x2e,0x33,0x0f]]
#       ]
#       operations.each do |command, data|
#         @spi.write_command(command)
#         @spi.write_data(data)
#       end
#       p "ppppp"
#       @spi.write_command(0x11)
#       @spi.write_command(0x29)
#       if(bl >= 0)
#         @bl = GPIO.new(bl, GPIO::OUT)
#         @bl.write(1) # add:ILI9342C on M5Stack
#       end
#       p "init finish"
#     end

#     def _data(data)
#       # p "1"
#         @dc.write(1)
#       # p "2"
#         @cs.write(0)
#       # p "3"
#       p data
#         @spi.write(data)
#       # p "4"
#         @cs.write(1)
#       # p "5"
#     end

#     def _block(x0, y0, x1, y1, data=nil)
#         _write(ILI934X::COLUMN_SET, ustruct.pack(">HH", x0, x1))
#         _write(ILI934X::PAGE_SET, ustruct.pack(">HH", y0, y1))
#         if data == nil
#             return _read(ILI934X::RAM_READ, (x1 - x0 + 1) * (y1 - y0 + 1) * 3)
#         end
#         _write(ILI934X::RAM_WRITE, data)
#     end

#     def _read(command, count)
#         @dc.write(0)
#         @cs.write(0)
#         @spi.write(bytearray([command]))
#         data = @spi.read(count)
#         @cs.write(1)
#         return data
#     end

#     def drawPixel(x, y, color=nil)
#         if x < 0 || x > ILI934X::_WIDTH || y < 0 || y > ILI934X::_HEIGHT
#             return
#         end
#         @spi.write_command(0x2a)
#         @spi.write_address(x, x)
#         @spi.write_command(0x2b)
#         @spi.write_address(y, y)
#         @spi.write_command(0x2c)
#         @spi.write_data_word(color)
#     end

#     # def fill_rectangle(x, y, w, h, color):
#     #     x = min(@width - 1, max(0, x))
#     #     y = min(@height - 1, max(0, y))
#     #     w = min(@width - x, max(1, w))
#     #     h = min(@height - y, max(1, h))
#     #     @_block(x, y, x + w - 1, y + h - 1, b'')
#     #     chunks, rest = divmod(w * h, 512)
#     #     if chunks:
#     #         data = ustruct.pack(">H", color) * 512
#     #         for count in range(chunks):
#     #             @_data(data)
#     #     data = ustruct.pack(">H", color) * rest
#     #     @_data(data)

#     # def fill(color):
#     #     @fill_rectangle(0, 0, @width, @height, color)

#     # def char(char, x, y, color=0xffff, background=0x0000):
#     #     buffer = bytearray(8)
#     #     framebuffer = framebuf.FrameBuffer1(buffer, 8, 8)
#     #     framebuffer.text(char, 0, 0)
#     #     color = ustruct.pack(">H", color)
#     #     background = ustruct.pack(">H", background)
#     #     data = bytearray(2 * 8 * 8)
#     #     for c, byte in enumerate(buffer):
#     #         for r in range(8):
#     #             if byte & (1 << r):
#     #                 data[r * 8 * 2 + c * 2] = color[0]
#     #                 data[r * 8 * 2 + c * 2 + 1] = color[1]
#     #             else:
#     #                 data[r * 8 * 2 + c * 2] = background[0]
#     #                 data[r * 8 * 2 + c * 2 + 1] = background[1]
#     #     @_block(x, y, x + 7, y + 7, data)

#     # def text(text, x, y, color=0xffff, background=0x0000, wrap=nil,
#     #          vwrap=nil, clear_eol=False):
#     #     if wrap is nil:
#     #         wrap = @width - 8
#     #     if vwrap is nil:
#     #         vwrap = @height - 8
#     #     tx = x
#     #     ty = y

#     #     def new_line():
#     #         nonlocal tx, ty

#     #         tx = x
#     #         ty += 8
#     #         if ty >= vwrap:
#     #             ty = y

#     #     for char in text:
#     #         if char == '\n':
#     #             if clear_eol and tx < wrap:
#     #                 @fill_rectangle(tx, ty, wrap - tx + 7, 8, background)
#     #             new_line()
#     #         else:
#     #             if tx >= wrap:
#     #                 new_line()
#     #             @char(char, tx, ty, color, background)
#     #             tx += 8
#     #     if clear_eol and tx < wrap:
#     #         @fill_rectangle(tx, ty, wrap - tx + 7, 8, background)

#     # def scroll(dy=nil):
#     #     if dy is nil:
#     #         return @_scroll
#     #     @_scroll = (@_scroll + dy) % @height
#     #     @_write(_LINE_SET, ustruct.pack('>H', @_scroll))

#     # def self.color(r, g, b)
#     #   return (r & 0xf8) << 8 | (g & 0xfc) << 3 | b >> 3
#     # end
# end