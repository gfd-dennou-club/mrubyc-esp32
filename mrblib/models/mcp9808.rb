# coding: utf-8

class MCP9808

  REG_MANUF_ID = 0x06
  REG_DEVICE_ID = 0x07
  REG_CONFIG = 0x01
  REG_CONFIG_SHUTDOWN = 0x0100
  REG_AMBIENT_TEMP = 0x05
  I2CADDR_DEFAULT = 0x18

  def initialize(i2c)
    @i2c = i2c
  end

  def read16(reg)
    @i2c.write(MCP9808::I2CADDR_DEFAULT, reg)
    data = @i2c.read_integer(MCP9808::I2CADDR_DEFAULT, 2)
    (data[0] << 8) | data[1]
  end

  def write16(reg, value)
    @i2c.write(MCP9808::I2CADDR_DEFAULT, [reg, ( value >> 8 ) & 0xFF, value & 0xFF])
  end

  def shutdown_wake(sw)
    conf_register = read16(MCP9808::REG_CONFIG);
    if sw then
      conf_shutdown = conf_register | MCP9808::REG_CONFIG_SHUTDOWN
      write16(MCP9808::REG_CONFIG, conf_shutdown)
    else
      conf_shutdown = conf_register & ~MCP9808::REG_CONFIG_SHUTDOWN
      write16(MCP9808::REG_CONFIG, conf_shutdown)
    end
  end

  def init_begin
    if read16(MCP9808::REG_MANUF_ID) != 0x0054
      print "error1\n"
      return false
    end
    if read16(MCP9808::REG_DEVICE_ID) != 0x0400
      print "error2\n"
      return false
    end
    write16(MCP9808::REG_CONFIG, 0x0)
    true
  end

 def read_temp_c
    t = read16(MCP9808::REG_AMBIENT_TEMP)
    if t != 0xFFFF
      temp = t & 0x0FFF
      temp /= 16.0

      if ( t & 0x1000 ) != 0
        temp -= 256
      end
    end
    temp
  end
end
