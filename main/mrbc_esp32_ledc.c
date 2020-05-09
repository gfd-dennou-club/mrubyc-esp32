/*! @file
  @brief
  mruby/c LEDC class for ESP32
  本クラスはインスタンスを生成せず利用する
*/

#include "mrbc_esp32_ledc.h"

#include "driver/ledc.h"


static struct RClass* mrbc_class_esp32_ledc;

static int unreferenced;


/*! メソッド nop(count) 本体 : nop (no operation)

  @param count nop の長さ、ダミー処理ループの回数
*/
static void
mrbc_nop(mrb_vm* vm, mrb_value* v, int argc)
{
  // NO OPERATION
  int max = GET_INT_ARG(1);
  for ( int i = 0 ; i < max ; ++i ) {
    unreferenced += 1;
  }
}


/*! メソッド timer_config() 本体 : wrapper for ledc_timer_config

  @param rslv   解像度
  @param freq   周波数
  @param sp     スピードモード
*/
static void
mrbc_esp32_ledc_timer_config(mrb_vm* vm, mrb_value* v, int argc)
{
  int rslv  = GET_INT_ARG(1);
  int freq  = GET_INT_ARG(2);
  int sp    = GET_INT_ARG(3);
 
  ledc_timer_config_t config = {
    .duty_resolution = rslv,
    .freq_hz         = freq,
    .speed_mode      = sp
  };

  ledc_timer_config(&config);
}

/*! メソッド channel_config() 本体 : wrapper for ledc_channel_config

  @param ch     チャンネル
  @param pin    GPIO ピン番号
  @param sp     スピードモード
*/
static void
mrbc_esp32_ledc_channel_config(mrb_vm* vm, mrb_value* v, int argc)
{
  int ch    = GET_INT_ARG(1);
  int pin   = GET_INT_ARG(2);
  int sp    = GET_INT_ARG(3);
 
  ledc_channel_config_t config = {
    .channel  = ch,
    .duty     = 0,
    .gpio_num = pin,
    .speed_mode = sp
  };

  ledc_channel_config(&config);
}


/*! メソッド set_freq() 本体 : wrapper for ledc_set_freq

  @param sp     スピードモード
  @param ch     チャンネル
  @param freq   周波数
*/
static void
mrbc_esp32_ledc_set_freq(mrb_vm* vm, mrb_value* v, int argc)
{
  int sp   = GET_INT_ARG(1);
  int ch   = GET_INT_ARG(2);
  int freq = GET_INT_ARG(3);

  ledc_set_freq(sp, ch, freq);
}


/*! メソッド set_duty() 本体 : wrapper for ledc_set_duty

  @param sp     スピードモード
  @param ch     チャンネル
  @param duty   デューティー比
*/
static void
mrbc_esp32_ledc_set_duty(mrb_vm* vm, mrb_value* v, int argc)
{
  int sp   = GET_INT_ARG(1);
  int ch   = GET_INT_ARG(2);
  int duty = GET_INT_ARG(3);

  ledc_set_duty(sp, ch, duty);
}


/*! メソッド update_duty() 本体 : wrapper for ledc_update_duty

  @param sp     スピードモード
  @param ch     チャンネル
*/
static void
mrbc_esp32_ledc_update_duty(mrb_vm* vm, mrb_value* v, int argc)
{
  int sp   = GET_INT_ARG(1);
  int ch   = GET_INT_ARG(2);
  
  ledc_update_duty(sp, ch);
}


/*! クラス定義処理を記述した関数
  この関数を呼ぶことでクラス LEDC が定義される

  @param vm mruby/c VM
*/
void
mrbc_mruby_esp32_ledc_gem_init(struct VM* vm)
{
/*
LEDC.timer_config()
LEDC.channel_config()
LEDC.set_freq()
LEDC.set_duty()
LEDC.update_duty()
*/
  // クラス LEDC 定義
  mrbc_class_esp32_ledc = mrbc_define_class(vm, "LEDC", mrbc_class_object);
  
  // 各メソッド定義（mruby/c ではインスタンスメソッドをクラスメソッドとしても呼び出し可能）
  mrbc_define_method(vm, mrbc_class_esp32_ledc, "timer_config",    mrbc_esp32_ledc_timer_config);
  mrbc_define_method(vm, mrbc_class_esp32_ledc, "channel_config",  mrbc_esp32_ledc_channel_config);  
  mrbc_define_method(vm, mrbc_class_esp32_ledc, "set_freq",        mrbc_esp32_ledc_set_freq);
  mrbc_define_method(vm, mrbc_class_esp32_ledc, "set_duty",        mrbc_esp32_ledc_set_duty);
  mrbc_define_method(vm, mrbc_class_esp32_ledc, "update_duty",     mrbc_esp32_ledc_update_duty);
  mrbc_define_method(vm, mrbc_class_esp32_ledc, "nop",             mrbc_nop);
}
