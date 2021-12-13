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
        @dc.write(0)
        write(data)
    end
    
    def write_data(data)
        @dc.write(1)
        write(data)
    end

    def read()
        SPI.read_byte()
    end
end