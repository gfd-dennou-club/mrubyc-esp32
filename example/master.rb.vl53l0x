# https://github.com/pololu/vl53l0x-arduino/blob/master/examples/Continuous/Continuous.ino

# i2c = I2C.new(22, 21)
# vl53l0x = VL53L0X.new(i2c)
# vl53l0x.set_timeout(500)
# if !vl53l0x.init
#   puts "Failed to detect and initialize sensor!"
# else
#   vl53l0x.start_continuous
#   while true
#     puts vl53l0x.read_range_continuous_millimeters
#     puts " TIMEOUT" if vl53l0x.timeout_occurred
#     sleep 0.01
#   end
# end



# https://github.com/pololu/vl53l0x-arduino/blob/master/examples/Single/Single.ino

# i2c = I2C.new(22, 21)
# vl53l0x = VL53L0X.new(i2c)
# vl53l0x.set_timeout(500)
# if !vl53l0x.init
#   puts "Failed to detect and initialize sensor!"
# else
#   vl53l0x.set_measurement_timing_budget(200000);
#   while true
#     puts vl53l0x.read_range_single_millimeters
#     puts " TIMEOUT" if vl53l0x.timeout_occurred
#     sleep 0.1
#   end
# end



# http://msms1003.blogspot.com/2017/12/vl53l0x.html

# i2c = I2C.new(22, 21)
# vl53l0x = VL53L0X.new(i2c)
# vl53l0x.set_timeout(500)
# vl53l0x.init
# vl53l0x.set_signal_rate_limit(0.1)
# vl53l0x.set_vcsel_purse_period(0, 18)
# vl53l0x.set_vcsel_purse_period(1, 14)
# vl53l0x.start_continuous
#
# while true
#   puts vl53l0x.read_range_continuous_millimeters
#   puts " TIMEOUT" if vl53l0x.timeout_occurred
#   sleep 0.1
# end



# https://note.suzakugiken.jp/pololu-tof-tutorial-a/

i2c = I2C.new(22, 21)
vl53l0x = VL53L0X.new(i2c)
vl53l0x.init
vl53l0x.set_timeout(500)

# 長距離モード
# センサの感度を高め, その潜在的な範囲を広げるが, 意図されたターゲット以外の物体からの反射のために不正確なデータを得る可能性がある
# 戻り信号のレート制限を下げる()
# vl53l0x.set_signal_rate_limit(0.1)
# レーザーパルス周期を長くする
# vl53l0x.set_vcsel_purse_period(0, 18)
# vl53l0x.set_vcsel_purse_period(1, 14)

# 精度を下げてより高い速度を得る
# タイミングパジェットを20ミリ秒に減らす
# vl53l0x.set_measurement_timing_budget(20_000)

# 速度を落としてより高い精度を得る
# タイミングパジェットを200ミリ秒に増やす
vl53l0x.set_measurement_timing_budget(200_000)

while true
  puts vl53l0x.read_range_single_millimeters
  puts " TIMEOUT" if vl53l0x.timeout_occurred
end
