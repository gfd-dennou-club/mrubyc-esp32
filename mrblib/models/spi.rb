class SPI

    def initialize(mosi, miso, clk, cs, dc, rst = -1, bl = -1)
        p "SPI initializing..."
        @cs = GPIO.new(cs, GPIO::OUT)
        @cs.write(0)
        @dc = GPIO.new(dc, GPIO::OUT)
        @dc.write(0)
        if(rst >= 0)
            @rst = GPIO.new(rst, GPIO::OUT)
            @rst.write(1)
            sleep 0.1
            @rst.write(0)
        end
        if(bl >= 0)
            @bl = GPIO.new(bl, GPIO::OUT)
            @bl.write(0)
        end
        SPI.bus_initialize(mosi, miso, clk, cs, dc, rst, bl)
        @dc = GPIO.new(dc, GPIO::OUT)
        p "SPI initializing finished."
    end

    # Accept augements form such as
    # write(data1, data2, ...)
    # write([data1, data2, ...]) 
    def write(*data)
        # p "write"
        # p data
        # data.each do |d|
        #     if(d.is_a?(Array))
        #         d.each do |_d|
        #             # p _d
        #             SPI.write_byte(_d)
        #         end
        #     else
        #         # p d
        #         SPI.write_byte(d)
        #     end
        # end
        # p "write end"
        if(data[0].is_a?(Array))
            SPI.write_byte(data[0], data[0].length)
            p data[0].length
        else
            SPI.write_byte(data, data.length)
            p data.length
        end
        # p "write end"
    end

    def write_command(data)
        p "write_commnad"
        p data
        @dc.write(0)
        write(data)
    end
    
    def write_data(*data)
        p "write_data"
        p data
        @dc.write(1)
        write(data)
    end
    
    def write_address(addr1, addr2)
        data = Array.new(4)
        data[0] = (addr1 >> 8) & 0xFF
        data[1] = addr1 & 0xFF
        data[2] = (addr2 >> 8) & 0xFF
        data[3] = addr2 & 0xFF
        @dc.write(1)
        write(data)
    end

    def write_data_word(data)
        data_array = Array.new(2)
        data_array[0] = (data >> 8) & 0xFF
        data_array[1] = data & 0xFF
        @dc.write(1)
        write(data_array)
    end
    
    def write_color(color, size)
        data = Array.new(size * 2)
        size_ = size * 2 - 1
        (0..(size - 1)).each do |s|
            data[s * 2] = (color >> 8) & 0xff;
            data[s * 2 + 1] = color & 0xff;
        end
        @dc.write(1)
        write(data)
    end

    def read()
        SPI.read_byte()
    end

end