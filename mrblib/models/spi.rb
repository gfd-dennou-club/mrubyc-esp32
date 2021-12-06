class SPI
    def initialize(mosi, miso, clk, cs, dc, rst = -1, bl = -1)
        p "SPI initializing..."
        @cs = GPIO.new(cs, GPIO::OUT)
        @cs.write(0)
        @dc = GPIO.new(dc, GPIO::OUT)
        @dc.write(0)
        @mode = 0
        if(rst >= 0)
            @rst = GPIO.new(rst, GPIO::OUT)
            @rst.write(0)
            sleep 0.1
            @rst.write(1)
        end
        if(bl >= 0)
            @bl = GPIO.new(bl, GPIO::OUT)
            @bl.write(0)
        end
        SPI.bus_initialize(mosi, miso, clk, cs, dc, rst, bl)
        p "SPI initializing finished."
    end

    def set_command_mode
        @dc.write(0)
    end

    def set_data_mode
        @dc.write(1)
    end

    # Accept augements form such as
    # write(data1, data2, ...)
    # write([data1, data2, ...]) 
    def write(*data)
        if(data[0].is_a?(Array))
            SPI.write_byte(data[0], data[0].length)
        else
            SPI.write_byte(data, data.length)
        end
    end

    def write_command(data)
        set_command_mode()
        write(data)
    end
    
    def write_data(*data)
        set_data_mode()
        write(data)
    end
    
    def write_address(addr1, addr2)
        data = Array.new(4)
        data[0] = (addr1 >> 8) & 0xFF
        data[1] = addr1 & 0xFF
        data[2] = (addr2 >> 8) & 0xFF
        data[3] = addr2 & 0xFF
        set_data_mode()
        write(data)
    end

    def write_data_word(data)
        data_array = Array.new(2)
        data_array[0] = (data >> 8) & 0xFF
        data_array[1] = data & 0xFF
        set_data_mode()
        write(data_array)
    end
    
    def write_color(color, size)
        set_data_mode()
        SPI.__write_color(color, size)
    end

    def read()
        SPI.read_byte()
    end
end