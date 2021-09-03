class SHT3X
    def initialize(i2c)
        @i2c = i2c
        @address = 0x44
    end

    def readTemperature()
        @i2c.write(@address, 0x2c, 0x06)
        sleep 0.5
        data = @i2c.read_integer(@address, 6)
        cTemp = ((((data[0] * 256.0) + data[1]) * 175) / 65535.0) - 45;
        fTemp = (cTemp * 1.8) + 32;
        humidity = ((((data[3] * 256.0) + data[4]) * 100) / 65535.0);
        cTemp
    end
end