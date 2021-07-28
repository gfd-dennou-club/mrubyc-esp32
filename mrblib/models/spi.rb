class SPI
    def initialize(mosi, miso, clk, cs)
        p "SPI initializing..."
        SPI.bus_initialize(mosi, miso, clk, cs)
    end

    def write(*data)
        p data
        data.each do |d|
            if(d.is_a?(Array))
                d.each do |_d|
                    p _d
                    SPI.write_byte(_d)
                end
            else
                p d
                SPI.write_byte(d)
            end
        end
    end

    def read()
        SPI.read_byte()
    end

end