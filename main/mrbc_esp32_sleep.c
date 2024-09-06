
/*
  スリープ復帰のトリガーは Timer のみサポート
*/

#include "mrbc_esp32_sleep.h"
#include "esp_sleep.h"

static struct RClass* mrbc_class_esp32_sleep;

/*! メソッド deep_sleep(time_in_us) 本体 : wrapper for esp_deep_sleep

  @param time_in_us  ディープスリープを行う秒数(μs)

    復帰後はマイコンが再起動する
*/
static void mrbc_esp32_sleep_deep_sleep(mrb_vm *vm, mrb_value *v, int argc){

  int time_in_us = GET_INT_ARG(1);
  esp_deep_sleep(time_in_us);
}

/*! メソッド light_sleep(time_in_us) 本体 : wrapper for esp_light_sleep

  @param time_in_us  ライトスリープを行う秒数(μs)

    クロックを止めて省電力モードに入る．
    スリープ解除後は開始した行の次から再開される．また，メモリも保持される．
*/
static void mrbc_esp32_sleep_light_sleep(mrb_vm *vm, mrb_value *v, int argc){

  int time_in_us = GET_INT_ARG(1);
  esp_sleep_enable_timer_wakeup(time_in_us);
  esp_light_sleep_start();
}

/*! クラス定義処理を記述した関数
  この関数を呼ぶことでクラス SLEEP が定義される

  @param vm mruby/c VM
*/
void
mrbc_esp32_sleep_gem_init(struct VM* vm)
{
/*
SLEEP.deep_us(time_in_us)
SLEEP.light_us(time_in_us)
*/
  // クラス SLEEP 定義
  mrbc_class_esp32_sleep = mrbc_define_class(vm, "SLEEP", mrbc_class_object);

  // 各メソッド定義（mruby/c ではインスタンスメソッドをクラスメソッドとしても呼び出し可能）
  mrbc_define_method(vm, mrbc_class_esp32_sleep, "deep_us",  mrbc_esp32_sleep_deep_sleep);
  mrbc_define_method(vm, mrbc_class_esp32_sleep, "light_us", mrbc_esp32_sleep_light_sleep);
}
