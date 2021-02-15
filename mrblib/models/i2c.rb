class I2C
  # 定数
  MASTER = 0
  SLAVE = 1
  def initialize(port, scl, sda)
    @port = port
    @scl  = scl
    @sda  = sda
  end

  def write(i2c_adrs_7, *data)
    p data
    if(data[0].kind_of?(Array))
      data = data[0]
    end
    self.__write(i2c_adrs_7, data)
  end
  
  def read(i2c_adrs_7, len, *params)
    write(i2c_adrs_7, params) unless params.empty?
    self.__read(i2c_adrs_7, len)
  end
end
