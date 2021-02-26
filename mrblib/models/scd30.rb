# coding: utf-8
###### sgp30.rb ######

# coding: utf-8
# SENSIRION SCD30
#
# I2C address : 0x61

=begin

データシート
https://media.digikey.com/pdf/Data%20Sheets/Sensirion%20PDFs/CD_AN_SCD30_Interface_Description_D1.pdf

参考
Arduino/libraries/Adafruit_SCD30/Adafruit_SCD30.cpp

=end

class SCD30
  
  I2CADDR_DEFAULT =  0x61                       #SCD30 default i2c address
  CHIP_ID = 0x60                                #SCD30 default device id from WHOAMI  
  WHOAMI = [0xD1,0x00]                          # Chip ID register
  CMD_READ_MEASUREMENT = [0x03,0x00]            # Main data register
  CMD_CONTINUOUS_MEASUREMENT = [0x00,0x10]      # Command to start continuous measurement
  CMD_STOP_MEASUREMENTS = [0x01,0x04]           # Command to stop measurements
  CMD_SET_MEASUREMENT_INTERVAL = [0x46,0x00]    # Command to set measurement interval
  CMD_GET_DATA_READY = [0x02,0x02]              # Data ready reg
  CMD_AUTOMATIC_SELF_CALIBRATION = [0x53,0x06]  # enables/disables auto calibration
  CMD_SET_FORCED_RECALIBRATION_REF = [0x52,0x04]# Forces calibration with given value
  CMD_SET_TEMPERATURE_OFFSET = [0x54,0x03]      # Specifies the temp offset
  CMD_SET_ALTITUDE_COMPENSATION = [0x51,0x02]   # Specifies altitude offset
  CMD_SOFT_RESET = [0xD3,0x04]                  # Soft reset!
  CMD_READ_REVISION = [0xD1,0x00]               # Firmware revision number
  CRC8_POLYNOMIAL = 0x31                        # Seed for SCD30's CRC polynomial
  CRC8_INIT = 0xFF                              # Init value for CRC

  # Initialization
  #
  def initialize(i2c)
    @i2c = i2c
  end

  # @brief Performs a software reset initializing registers to their power on state.
  #
  def reset
    return @i2c.__write( SCD30::I2CADDR_DEFAULT, SCD30::CMD_SOFT_RESET )
    sleep (0.1)
  end

  # @brief Ask the sensor if new data is ready to read
  # @return true: data is available false: no new data available 
  #
  def dataReady
    return ( readRegister(SCD30::CMD_GET_DATA_READY) == 1 )
  end

  # @brief Set the amount of time between measurements
  # @param interval The time between measurements in seconds. Must be from 2-1800
  #        seconds. The default value set on sensor initialization is 2 seconds.
  # @return true: success false: failure 
  #
  def setMeasurementInterval( interval )
    if ( (interval < 2) || (interval > 1800) ) 
      return false
    end
    return sendCommand(SCD30::CMD_SET_MEASUREMENT_INTERVAL, interval)
  end

  # @brief Read the current amount of time between measurements
  # @return uint16_t The current measurement interval in seconds.
  #
  def getMeasurementInterval
    return readRegister(SCD30::CMD_SET_MEASUREMENT_INTERVAL)
  end

  # @brief Gets the enable status of the SCD30's self calibration routine
  # @return true: enabled false: disabled
  #
  def selfCalibrationEnabled
    return (readRegister(SCD30::CMD_AUTOMATIC_SELF_CALIBRATION) == 1)
  end

  # @brief Enable or disable the SCD30's self calibration routine
  # @param enabled true: enable false: disable
  # @return true: success false: failure
  #
  def selfCalibrationEnabled(enabled) 
    return sendCommand(SCD30::CMD_AUTOMATIC_SELF_CALIBRATION, enabled)
  end

  # @brief Tell the SCD30 to start taking measurements continuously
  # @param pressure an optional pressure offset to correct for in millibar (mBar)
  # @return true: succes false: failure
  #
  def startContinuousMeasurement(pressure) 
    return sendCommand(SCD30::CMD_CONTINUOUS_MEASUREMENT, pressure)
  end

  # @brief Read the current ambient pressure offset
  # @return uint16_t  current ambient pressure offset in millibar (mBar)
  #
  def getAmbientPressureOffset
    return readRegister(SCD30::CMD_CONTINUOUS_MEASUREMENT)
  end

  # @brief Set the altitude offset that the SCD30 should correct for
  #  **Note:** This value is saved to the SCD30's internal storage and is reloaded
  #            on sensor power up.
  # @param altitude The altitude offset in meters above sea level.
  # @return true: success false: failure
  def setAltitudeOffset(altitude) 
    return sendCommand(SCD30::CMD_SET_ALTITUDE_COMPENSATION, altitude)
  end

  # @brief Get the current altitude offset
  # @return uint16_t The current altitude offset value in meters above sea level.
  #
  def getAltitudeOffset
    return readRegister(SCD30::CMD_SET_ALTITUDE_COMPENSATION)
  end

  # @brief Set a temperature offset
  # @param temp_offset The **positive** temperature offset to set in hundreths of a degree C ie:
  #       1015 => 10.15 degrees C
  #        31337 => 313.37 degrees C
  # **Note:** This value is saved to the SCD30's internal storage and is reloaded  on sensor power up.
  # @return true: success false: failure
  #
  def setTemperatureOffset(temp_offset) 
    return sendCommand(SCD30::CMD_SET_TEMPERATURE_OFFSET, temp_offset)
  end

  # @brief Get the current temperature offset in hundreths of a degree C
  # @return uint16_t the current temperature offset
  #
  def getTemperatureOffset
    return readRegister(SCD30::CMD_SET_TEMPERATURE_OFFSET)
  end
  
  # @brief Force the SCD30 to recalibrate with a given reference value
  # @param reference The calibration reference value in ppm from 400-2000 ppm.
  # **Note:** This value is saved to the SCD30's internal storage and is reloaded on sensor power up.
  # **Setting a reference value and forcing recalibration will override any previous automatic self-calibration.**
  # @return true: success false: failure
  #
  def forceRecalibrationWithReference(reference) 
    if ((reference < 400) || (reference > 2000)) 
      return false
    end
    return sendCommand(SCD30::CMD_SET_FORCED_RECALIBRATION_REF, reference)
  end

  # @brief Get the current forced recalibration reference value
  # @return uint16_t The current reference value in ppm
  #
  def getForcedCalibrationReference
    return readRegister(SCD30::CMD_SET_FORCED_RECALIBRATION_REF)
  end

  # @brief  Updates the measurement data for all sensors simultaneously
  # @return true: success false: failure
  #
  def read
    @i2c.__write(SCD30::I2CADDR_DEFAULT, SCD30::CMD_READ_MEASUREMENT )
    sleep(0.1)

    buf = @i2c.__read(SCD30::I2CADDR_DEFAULT, 18)   # 18 バイト分読み込み

    # CRC のチェック
    [0,3,6,9,12,15].each do |i|
      if crc8([buf[i], buf[i+1]]) != buf[i+2]
#        puts "BAD CRC"
        return false
      end
    end
    
    # 数値に変換
    co2  = floatCast( buf[0],  buf[1],  buf[3],  buf[4]  )
    temp = floatCast( buf[6],  buf[7],  buf[9],  buf[10] )
    humi = floatCast( buf[12], buf[13], buf[14], buf[15] )

    # 変換の確認用
    #co2  = floatCast( 0x43, 0xdb, 0x8c, 0x2e )
    #temp = floatCast( 0x41, 0xd9, 0xe7, 0xff )
    #humi = floatCast( 0x42, 0x43, 0x3a, 0x1b )
    #p co2, temp, humi
    
    #戻り値
    readdata = [ co2[0].to_f / 100.0, temp[0].to_f / 100.0, humi[0].to_f / 100.0 ]
    
    return readdata
  end

  def sendCommand( data, argument )
    buffer = Array.new(5)
    buffer[0] = data[0]
    buffer[1] = data[1]
    buffer[2] = argument >> 8
    buffer[3] = argument & 0xFF
    buffer[4] = crc8( [buffer[2], buffer[3]])
    return @i2c.__write(SCD30::I2CADDR_DEFAULT, buffer)
    sleep(0.1)
  end


  def readRegister( buffer )
    # the SCD30 really wants a stop before the read!
    @i2c.__write(SCD30::I2CADDR_DEFAULT, buffer )
    sleep(0.1)
    buf = @i2c.__read(SCD30::I2CADDR_DEFAULT, 3)   # 3 バイト分読み込み

    #CRC check
    if crc8([buf[0], buf[1]]) != buf[2]
      #      puts "readRegister : BAD CRC"
      return false
    end
    return buf[0] << 8 | (buf[1] & 0xFF)
  end
  
  
  def crc8(data) #dataは要素2の配列
    crc = SCD30::CRC8_INIT
    
    data.length.times do |i|
      crc = crc ^ data[i]
      8.times do |b|
        if ((crc & 0x80) != 0) then
          crc = ((crc << 1)&0xff) ^ SCD30::CRC8_POLYNOMIAL
        else
          crc = (crc << 1)&0xff
        end
      end
    end
    return crc
  end
  
 
end
