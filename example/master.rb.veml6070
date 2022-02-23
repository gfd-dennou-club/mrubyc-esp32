#I2C 初期化
i2c = I2C.new(22, 21)
# VEML6070の初期化
veml = VEML6070.new(i2c)
veml.init

while true
  puts "UV data : " + veml.get_uv.to_s # 紫外線の輝度
  puts "UV level : " + veml.get_uv_index # UVインデックス
  sleep 1
end
