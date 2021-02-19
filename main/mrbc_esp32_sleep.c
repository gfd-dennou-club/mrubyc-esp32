/*! @file
  @brief
  mruby/c SLEEP class for ESP32
  本クラスはインスタンスを生成せず利用する
*/

#include "mrbc_esp32_sleep.h"

#include "esp_sleep.h"

static struct RClass* mrbc_class_esp32_sleep;
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

/*! メソッド deep_sleep(time_in_us) 本体 : wrapper for esp_deep_sleep

  @param time_in_us  ディープスリープを行う秒数(μs)

*/
static void mrbc_esp32_sleep_deep_sleep(mrb_vm *vm, mrb_value *v, int argc){

  int time_in_us = GET_INT_ARG(1);
  esp_deep_sleep(time_in_us);
}

/*! メソッド light_sleep(time_in_us) 本体 : wrapper for esp_light_sleep

  @param time_in_us  ライトスリープを行う秒数(μs)

*/
static void mrbc_esp32_sleep_light_sleep(mrb_vm *vm, mrb_value *v, int argc){

  int time_in_us = GET_INT_ARG(1);
  esp_sleep_enable_timer_wakeup(time_in_us);
  esp_light_sleep_start();
}

/*! メソッド enable_gpio_wakeup() 本体 : wrapper for esp_sleep_enable_gpio_wakeup */
static void mrbc_esp32_sleep_enable_gpio_wakeup(mrb_vm *vm, mrb_value *v, int argc){

  esp_sleep_enable_gpio_wakeup();

}

/*! クラス定義処理を記述した関数
  この関数を呼ぶことでクラス SLEEP が定義される

  @param vm mruby/c VM
*/
void
mrbc_mruby_esp32_sleep_gem_init(struct VM* vm)
{
/*
SLEEP.deep_sleep(time_in_us)
SLEEP.light_sleep(time_in_us)
SLEEP.enable_gpio_wakeup()
*/
  // クラス SLEEP 定義
  mrbc_class_esp32_sleep = mrbc_define_class(vm, "SLEEP", mrbc_class_object);

  // 各メソッド定義（mruby/c ではインスタンスメソッドをクラスメソッドとしても呼び出し可能）
  mrbc_define_method(vm, mrbc_class_esp32_sleep, "deep_sleep",         mrbc_esp32_sleep_deep_sleep);
  mrbc_define_method(vm, mrbc_class_esp32_sleep, "light_sleep",        mrbc_esp32_sleep_light_sleep);
  mrbc_define_method(vm, mrbc_class_esp32_sleep, "enable_gpio_wakeup", mrbc_esp32_sleep_enable_gpio_wakeup);
  mrbc_define_method(vm, mrbc_class_esp32_sleep, "nop",                mrbc_nop);
}
