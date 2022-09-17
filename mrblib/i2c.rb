# coding: utf-8
class I2C
  # 定数
  MASTER = 0
  SLAVE = 1

  # 初期化
  def initialize(scl, sda, port = 0, freq = 400000)
    @port = port
    @scl  = scl
    @sda  = sda
    @freq = freq
    self.driver_install
  end

  # コンストラクタ外からの再初期化
  def init(scl, sda, port = 0, freq = 400000)
    @port = port
    @scl  = scl
    @sda  = sda
    @freq = freq
    self.driver_install
  end

  # IC2ドライバーの削除
  def deinit
    self.driver_delete
  end

  # 書き込み
  def write(i2c_adrs_7, *data)
    if(data[0].kind_of?(String))
      s = data[0]
      data = Array.new
      s.length.times do |n|
        data.push(s[n].ord)
      end
    elsif(data[0].kind_of?(Array))
      data = data[0]
    end
    self.__write(i2c_adrs_7, data)
  end

  # 書き込み. 内容は write と同じ
  def writeto(i2c_adrs_7, *data)
    if(data[0].kind_of?(String))
      s = data[0]
      data = Array.new
      s.length.times do |n|
        data.push(s[n].ord)
      end
    elsif(data[0].kind_of?(Array))
      data = data[0]
    end
    self.__write(i2c_adrs_7, data)
  end

  # 読み込み．出力は strings
  def read(i2c_adrs_7, len, *params)
    write(i2c_adrs_7, params) unless params.empty?
    a = self.__read(i2c_adrs_7, len)
    a.join(',').to_s
    s = ""
    a.length.times do |n|
      s << a[n].chr
    end
    return s
  end

  # 読み込み．出力は int の配列
  def readfrom(i2c_adrs_7, len, *params)
    write(i2c_adrs_7, params) unless params.empty?
    self.__read(i2c_adrs_7, len)
  end
end
