/*! @file
  @brief
  mruby/c BMP280 class for ESP32
  initialize メソッドは Ruby コード側で定義する
*/

#include "mrbc_esp32_bmp280.h"

#include "esp_log.h"

/*! @def
  BMP280 Read 操作で用意されるバッファの長さ。
*/
#define MAX_READ_LEN  32


static struct RClass* mrbc_class_esp32_bmp280;

/*! メソッド convert_pressure() 本体 : wrapper for bmp280_driver_install
  @param adc_P  gotten value
  @param t_fine gotten value
*/
static void
mrbc_esp32_bmp280_convert_pressure(mrb_vm* vm, mrb_value* v, int argc)
{
  typedef signed long long int64_t;
  int64_t adc_P  =  (int64_t)GET_INT_ARG(1);
  int64_t t_fine =  (int64_t)GET_INT_ARG(2);
  int64_t dig_P1 =  (int64_t)GET_INT_ARG(3);
  int64_t dig_P2 =  (int64_t)GET_INT_ARG(4);
  int64_t dig_P3 =  (int64_t)GET_INT_ARG(5);
  int64_t dig_P4 =  (int64_t)GET_INT_ARG(6);
  int64_t dig_P5 =  (int64_t)GET_INT_ARG(7);
  int64_t dig_P6 =  (int64_t)GET_INT_ARG(8);
  int64_t dig_P7 =  (int64_t)GET_INT_ARG(9);
  int64_t dig_P8 =  (int64_t)GET_INT_ARG(10);
  int64_t dig_P9 =  (int64_t)GET_INT_ARG(11);
  int64_t var1, var2, p;
  adc_P >>= 4;
  var1 = ((int64_t)t_fine) - 128000;
  var2 = var1 * var1 * (int64_t)dig_P6;
  var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
  var2 = var2 + (((int64_t)dig_P4) << 35);
  var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) +
         ((var1 * (int64_t)dig_P2) << 12);
  var1 =
      (((((int64_t)1) << 47) + var1)) * ((int64_t)dig_P1) >> 33;
  if (var1 == 0) {
    SET_INT_RETURN(0);
    return;
  }
  p = 1048576 - adc_P;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((int64_t)dig_P8) * p) >> 19;

  p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7) << 4);
  SET_INT_RETURN((float)p / 256);
}

/*! クラス定義処理を記述した関数
  この関数を呼ぶことでクラス BMP280 が定義される

  @param vm mruby/c VM
*/
void
mrbc_mruby_esp32_bmp280_gem_init(struct VM* vm)
{
/*
BMP280#convert_pressure(value)
*/

  // クラス BMP280 定義
  mrbc_class_esp32_bmp280 = mrbc_define_class(vm, "BMP280", mrbc_class_object);
  // 各メソッド定義
  mrbc_define_method(vm, mrbc_class_esp32_bmp280, "convert_pressure", mrbc_esp32_bmp280_convert_pressure);
}
