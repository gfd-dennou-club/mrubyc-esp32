class SPI
    def initialize(mosi, miso, clk, cs, dc, rst = -1, bl = -1)
        p "SPI initializing..."
        @mode = 0
        GPIO.new(cs, GPIO::OUT, -1, 0)
        GPIO.new(dc, GPIO::OUT, -1, 0)
        if(rst >= 0)
            @rst = GPIO.new(rst, GPIO::OUT, -1, 0)
            sleep 0.1
            @rst.write(1)
        end
        GPIO.new(bl, GPIO::OUT, -1, 0) if bl >= 0
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

    def read()
        SPI.read_byte()
    end
end