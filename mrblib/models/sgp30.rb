# coding: utf-8
###### sgp30.rb ######

# coding: utf-8
# SGP30
#
# I2C address : 0x58

class SGP30

  #定数
  ## i2c address
  I2CADDR_DEFAULT = 0x58
  ## commands and constants
  FEATURESET = 0x0020 # The required set for this library
  CRC8_POLYNOMIAL = 0x31  # Seed for SGP30's CRC polynomial
  CRC8_INIT = 0xFF  # Init value for CRC
  WORD_LEN = 2     # 2 bytes per word

  def initialize(i2c)
    @i2c = i2c
  end

  def begin
    # シリアルナンバーを取得
    command = Array.new
    command[0] = 0x36
    command[1] = 0x82
    serialnumber = Array.new(3)
    serialnumber = readWordFromCommand(command,2,10,3)
    if !serialnumber then
      return false
    end

    # sgp30_get_feature_set(0x202F)
    command = Array.new
    command[0] = 0x20
    command[1] = 0x2F
    featureset = readWordFromCommand(command,2,10,1)
    if !featureset then
      return false
    end
    if((featureset[0] & 0xF0) != SGP30::FEATURESET) then
      return false
    end

    reply = IAQinit()
    if !reply then
      return false
    end

    return serialnumber
  end

  def softReset
    command = Array.new
    command[0] = 0x00
    command[1] = 0x06
    return readWordFromCommand(command,2,10)
  end

  def IAQinit
    command = Array.new
    command[0] = 0x20
    command[1] = 0x03
    return readWordFromCommand(command,2,10)
  end

  def IAQmeasure
    command = Array.new
    command[0] = 0x20
    command[1] = 0x08
    reply = Array.new
    reply = readWordFromCommand(command,2,12,2)
    if !reply then
      return false
    end
    return reply
  end

  def IAQmeasureRaw
    command = Array.new
    command[0] = 0x20
    command[1] = 0x50
    reply = Array.new
    reply = readWordFromCommand(command,2,25,2)
    if !reply then
      return false
    end
    return reply
  end

  def getIAQBaseline()
    command = Array.new
    command[0] = 0x20
    command[1] = 0x15
    reply = Array.new
    reply = readWordFromCommand(command,2,10,2)
    if !reply then
      return false
    end
    return reply
  end

  def setIAQBaseline(eco2_base, tvoc_base)
    command = Array.new
    command[0] = 0x20
    command[1] = 0x1e
    command[2] = tvoc_base >> 8
    command[3] = tvoc_base & 0xFF
    crcData = []
    2.times do |i| #2バイト
      crcData.push(command[2+i])
    end
    command[4] = generateCRC(crcData)
    command[5] = eco2_base >> 8
    command[6] = eco2_base & 0xFF
    crcData = []
    2.times do |i| #2バイト
      crcData.push(command[5+i])
    end
    command[7] = generateCRC(crcData)

    return readWordFromCommand(command, 8, 10)
  end

  def setHumidity(absolute_humidity)
    if (absolute_humidity > 256000) then
      return false
    end

    ah_scaled = (absolute_humidity * 256 * 16777) >> 24
    command = Array.new
    command[0] = 0x20
    command[1] = 0x61
    command[2] = ah_scaled >> 8
    command[3] = ah_scaled & 0xFF
    crcData = []
    2.times do |i| #2バイト
      crcData.push(command[2+i])
    end
    command[4] = generateCRC(crcData)

    return readWordFromCommand(command, 5, 10)
  end

  def readWordFromCommand(command,commandLength,delayms,readlen=0)

    #  @i2c.writeを使って書き込み
    cmd = Array.new
    commandLength.times do |n|
      cmd.push(command[n])
    end
    @i2c.__write(SGP30::I2CADDR_DEFAULT, cmd)

    sleep delayms / 1000.0

    if (readlen == 0) then
      return true
    end

    replylen = readlen * (SGP30::WORD_LEN + 1)
    replybuffer = Array.new(replylen)

    #  @i2c.readを使って読み込み
    replybuffer = @i2c.__read(SGP30::I2CADDR_DEFAULT,replylen)

    readdata = Array.new

    readlen.times do |i|
      crcData = []
      2.times do |j|
        crcData.push( replybuffer[i*3 + j] )
      end

      crc = generateCRC(crcData)
      if (crc != replybuffer[i*3 + 2]) then
        return false
      end
      # success! store it
      readdata[i] = replybuffer[i*3]
      readdata[i] = readdata[i] << 8
      readdata[i] = readdata[i] | replybuffer[i*3 +1]
    end

    return readdata
  end

  def generateCRC(data) #dataは要素2の配列
    crc = SGP30::CRC8_INIT

    data.length.times do |i|
      crc = crc ^ data[i]
      8.times do |b|
        if ((crc & 0x80) != 0) then
          crc = ((crc << 1)&0xff) ^ SGP30::CRC8_POLYNOMIAL
        else
          crc = (crc << 1)&0xff
        end
      end
    end
    return crc
  end

end

