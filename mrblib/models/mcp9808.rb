# coding: utf-8

class MCP9808

  MCP9808_REG_MANUF_ID = 0x06
  MCP9808_REG_DEVICE_ID = 0x07
  MCP9808_REG_CONFIG = 0x01
  MCP9808_REG_CONFIG_SHUTDOWN = 0x0100
  MCP9808_REG_AMBIENT_TEMP = 0x05
  MCP9808_I2CADDR_DEFAULT = 0x18

  def initialize(i2c)
    @i2c = i2c
  end

  def read16(reg)
    @i2c.write(MCP9808::MCP9808_I2CADDR_DEFAULT, reg)
    data = @i2c.read_integer(MCP9808::MCP9808_I2CADDR_DEFAULT, 2)

    return ( data[0] << 8 ) | data[1]
  end

  def write16(reg, value)
    @i2c.write(MCP9808::MCP9808_I2CADDR_DEFAULT, [reg, ( value >> 8 ) & 0xFF, value & 0xFF])
  end

  def shutdown_wake(sw)
    conf_register = read16(MCP9808::MCP9808_REG_CONFIG);
    if sw then
      conf_shutdown = conf_register | MCP9808::MCP9808_REG_CONFIG_SHUTDOWN
      write16(MCP9808::MCP9808_REG_CONFIG, conf_shutdown)
    else
      conf_shutdown = conf_register & ~MCP9808::MCP9808_REG_CONFIG_SHUTDOWN
      write16(MCP9808::MCP9808_REG_CONFIG, conf_shutdown)
    end
  end

  def init_begin()
    if read16(MCP9808::MCP9808_REG_MANUF_ID) != 0x0054 then
      print "error1\n"
      return false
    end
    if read16(MCP9808::MCP9808_REG_DEVICE_ID) != 0x0400 then
      print "error2\n"
      return false
    end

    write16(MCP9808::MCP9808_REG_CONFIG, 0x0)
    true
  end

 def readTempC()
    t = read16(MCP9808::MCP9808_REG_AMBIENT_TEMP)

    if t != 0xFFFF then
      temp = t & 0x0FFF
      temp /= 16.0

      if ( t & 0x1000 ) != 0 then
        temp -= 256
      end
    end

    return temp
  end

end

