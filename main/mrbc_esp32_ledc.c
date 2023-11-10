/*! @file
  @brief
  mruby/c LEDC functions for ESP32
*/

#include "mrbc_esp32_ledc.h"
#include "driver/ledc.h"

/*! timer_config() 本体 : wrapper for ledc_timer_config

  @param rslv   解像度
  @param freq   周波数
  @param sp     スピードモード
  @param num    timer_nun
*/
static void
mrbc_esp32_ledc_timer_config(mrb_vm* vm, mrb_value* v, int argc)
{
  int rslv  = GET_INT_ARG(1);
  int freq  = GET_INT_ARG(2);
  int sp    = GET_INT_ARG(3);
  int num   = GET_INT_ARG(4);
  
  ledc_timer_config_t config = {
    .duty_resolution = rslv,
    .freq_hz         = freq,
    .speed_mode      = sp,
    .timer_num       = num,
    .clk_cfg         = LEDC_AUTO_CLK,
  };

  ledc_timer_config(&config);
}

/*! channel_config() 本体 : wrapper for ledc_channel_config

  @param ch     チャンネル
  @param pin    GPIO ピン番号
  @param sp     スピードモード
*/
static void
mrbc_esp32_ledc_channel_config(mrb_vm* vm, mrb_value* v, int argc)
{
  int ch    = GET_INT_ARG(1);
  int pin   = GET_INT_ARG(2);
  int sp    = GET_INT_ARG(3);
  int num   = GET_INT_ARG(4);
 
  ledc_channel_config_t config = {
    .channel    = ch,
    .duty       = 0,
    .gpio_num   = pin,
    .speed_mode = sp,
    .hpoint     = 0,
    .timer_sel  = num
  };

  ledc_channel_config(&config);

  ledc_fade_func_install(0);
}



/*! set_freq() 本体 : wrapper for ledc_set_freq

  @param sp     スピードモード
  @param ch     チャンネル
  @param freq   周波数
*/
static void
mrbc_esp32_ledc_set_freq(mrb_vm* vm, mrb_value* v, int argc)
{
  int sp   = GET_INT_ARG(1);
  int num  = GET_INT_ARG(2);
  int freq = GET_INT_ARG(3);

  ledc_set_freq(sp, num, freq);
}


/*! set_duty() 本体 : wrapper for ledc_set_duty

  @param sp     スピードモード
  @param ch     チャンネル
  @param duty   デューティー比
*/
static void
mrbc_esp32_ledc_set_duty(mrb_vm* vm, mrb_value* v, int argc)
{
  int sp   = GET_INT_ARG(1);
  int ch   = GET_INT_ARG(2);
  int duty = GET_INT_ARG(3);

  ledc_set_duty(sp, ch, duty);
}


/*! update_duty() 本体 : wrapper for ledc_update_duty

  @param sp     スピードモード
  @param ch     チャンネル
*/
static void
mrbc_esp32_ledc_update_duty(mrb_vm* vm, mrb_value* v, int argc)
{
  int sp   = GET_INT_ARG(1);
  int ch   = GET_INT_ARG(2);
  
  ledc_update_duty(sp, ch);
}

/*! set_duty_and_update() 本体 : wrapper for ledc_set_duty_and_update

  @param sp     スピードモード
  @param ch     チャンネル
*/
static void
mrbc_esp32_ledc_set_duty_and_update(mrb_vm* vm, mrb_value* v, int argc)
{
  int sp   = GET_INT_ARG(1);
  int ch   = GET_INT_ARG(2);
  int duty = GET_INT_ARG(3);
  
  ledc_set_duty_and_update(sp, ch, duty, 0);
}

/*! stop() 本体 : wrapper for ledc_stop

  @param sp     スピードモード
  @param ch     チャンネル
*/
static void
mrbc_esp32_ledc_stop(mrb_vm* vm, mrb_value* v, int argc)
{
  int sp   = GET_INT_ARG(1);
  int ch   = GET_INT_ARG(2);
  int id   = 0; //アイドルレベル
  
  ledc_stop(sp, ch, id);
}


/*! 公開用関数

  @param vm mruby/c VM
*/
void
mrbc_esp32_ledc_gem_init(struct VM* vm)
{
  mrbc_define_method(0, mrbc_class_object, "ledc_timer_config",        mrbc_esp32_ledc_timer_config);
  mrbc_define_method(0, mrbc_class_object, "ledc_channel_config",      mrbc_esp32_ledc_channel_config);  
  mrbc_define_method(0, mrbc_class_object, "ledc_set_freq",            mrbc_esp32_ledc_set_freq);
  mrbc_define_method(0, mrbc_class_object, "ledc_set_duty",            mrbc_esp32_ledc_set_duty);
  mrbc_define_method(0, mrbc_class_object, "ledc_update_duty",         mrbc_esp32_ledc_update_duty);
  mrbc_define_method(0, mrbc_class_object, "ledc_set_duty_and_update", mrbc_esp32_ledc_set_duty_and_update);
  mrbc_define_method(0, mrbc_class_object, "ledc_stop",                mrbc_esp32_ledc_stop);
}
