/*! @file
  @brief
  mruby/c GPIO functions for ESP32

  GPIO_MODE_INPUT  : 1
  GPIO_MODE_OUTPUT : 2
  GPIO_PULLUP_ONLY : 0
  GPIO_PULLDOWN_ONLY : 1;
  GPIO_MODE_INPUT_OUTPUT_OD : 2;
*/

#include "mrbc_esp32_gpio.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

#define GPIO_IN                 0x01 //GPIO_MODE_INPUT = 1
#define GPIO_OUT                0x02 //GPIO_MODE_OUTPUT = 2
#define GPIO_PULL_UP            0x10 //GPIO_PULLUP_ONLY = 0
#define GPIO_PULL_DOWN          0x20 //GPIO_PULLDOWN_ONLY = 1

static char* TAG = "GPIO";

/*! constructor(pin, mode, pull_mode)
  
  gpio = GPIO.new(pin, GPIO::IN | GPIO::PULL_UP )
  gpio = GPIO.new(pin, GPIO::OUT )
*/
static void mrbc_esp32_gpio_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(gpio_num_t));

  //initialize を call
  mrbc_instance_call_initialize( vm, v, argc );

  vTaskDelay(100 / portTICK_PERIOD_MS);  //wait
}

/*! initiaizer()

 */
static void mrbc_esp32_gpio_initialize(mrbc_vm *vm, mrbc_value v[], int argc)
{
  gpio_num_t  pin  = GET_INT_ARG(1);
  gpio_mode_t mode = GET_INT_ARG(2) & 0x0F;
  gpio_pull_mode_t pull_mode = GET_INT_ARG(2) & 0xF0;

  // instance->data をgpio_num_t 型 へのポインタとみなして、値を代入する。
  *((gpio_num_t *)(v[0].instance->data)) = GET_INT_ARG(1); 
  
  // 1. 構造体をゼロ初期化
  gpio_config_t io_conf = {0};

  // 2. ピン番号をビットマスクで指定
  io_conf.pin_bit_mask = (1ULL << pin);
  
  // 3. モードの設定
  io_conf.mode = mode;

  // 4. プルアップ・ダウンの設定
  // デフォルト（何もしない）
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

  // 入力専用ピン(34-39)でない場合のみ、プルアップ/ダウンの要求を反映させる
  bool is_input_only = (pin == 34 || pin == 35 || pin == 36 || pin == 39);

  if (!is_input_only && mode == GPIO_MODE_INPUT) {
    if (pull_mode == GPIO_PULL_UP) {
      io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    } else if (pull_mode == GPIO_PULL_DOWN) {
      io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    }
  }

  // 5. 割り込みはとりあえず無効
  io_conf.intr_type = GPIO_INTR_DISABLE;

  // 6. 設定を反映
  ESP_ERROR_CHECK(gpio_config(&io_conf));

  //出力
  ESP_LOGD(TAG, "pin: %d, mode: %d, pull_up_en: %d", pin, io_conf.mode, io_conf.pull_up_en);
  //  ESP_LOGD(TAG, "pin:  %i", pin);
  //  ESP_LOGD(TAG, "mode: %i", mode);
  //  ESP_LOGD(TAG, "pull: %x", pull_mode);
  //  ESP_LOGD(TAG, "pull: %i", pull_bits);

}


/*! set_level(pin, level) 

  @param pin   GPIO ピン番号
  @param level ピンレベル、0 : low / 1 : high
*/
static void
mrbc_esp32_gpio_set_level(mrb_vm* vm, mrb_value* v, int argc)
{
  gpio_num_t pin   = *((gpio_num_t *)(v[0].instance->data));
  uint32_t   level = GET_INT_ARG(1);

  if (!(level == 0 || level == 1)) {
    ESP_LOGE(TAG, "invalid value detected");
  }

  ESP_ERROR_CHECK( gpio_set_level(pin, level) );
}

/*!  set_on(pin)

  @param pin   GPIO ピン番号
*/
static void
mrbc_esp32_gpio_set_on(mrb_vm* vm, mrb_value* v, int argc)
{
  gpio_num_t pin   = *((gpio_num_t *)(v[0].instance->data));
  uint32_t   level = 1;

  ESP_ERROR_CHECK( gpio_set_level(pin, level) );
}

/*!  set_off(pin)

  @param pin   GPIO ピン番号
*/
static void
mrbc_esp32_gpio_set_off(mrb_vm* vm, mrb_value* v, int argc)
{
  gpio_num_t pin   = *((gpio_num_t *)(v[0].instance->data));
  uint32_t   level = 0;

  ESP_ERROR_CHECK( gpio_set_level(pin, level) );
}

/*!  get_level(pin) 本体 : wrapper for gpio_get_level

  @param pin GPIO ピン番号
  @return    ピンレベル、0 : low / 1 : high
*/
static void
mrbc_esp32_gpio_get_level(mrb_vm* vm, mrb_value* v, int argc)
{
  gpio_num_t pin = *((gpio_num_t *)(v[0].instance->data));

  // Fixnum インスタンスを本関数の返り値としてセット
  // 値は gpio_get_level(pin) と同値
  SET_INT_RETURN( gpio_get_level(pin) );
}

/*!  high?()

  if gpio1.high?() ...
*/
static void
mrbc_esp32_gpio_get_high(mrb_vm* vm, mrb_value* v, int argc)
{
  gpio_num_t pin = *((gpio_num_t *)(v[0].instance->data));

  SET_BOOL_RETURN( gpio_get_level(pin) == 1 );
}

/*!  low?()

  if gpio1.low?() ...
*/
static void
mrbc_esp32_gpio_get_low(mrb_vm* vm, mrb_value* v, int argc)
{
  gpio_num_t pin = *((gpio_num_t *)(v[0].instance->data));

  SET_BOOL_RETURN( gpio_get_level(pin) == 0 );
}

/*! クラス定義処理を記述した関数

  @param vm mruby/c VM
*/
void
mrbc_esp32_gpio_gem_init(struct VM* vm)
{
  ////////////////////////////////////////////////////
  // GPIO クラスの定義
  //

  //mrbc_define_class でクラス名を定義
  mrbc_class *gpio = mrbc_define_class(0, "GPIO", 0);

  //mrbc_define_method でメソッドを定義
  mrbc_define_method(0, gpio, "new",        mrbc_esp32_gpio_new);
  mrbc_define_method(0, gpio, "initialize", mrbc_esp32_gpio_initialize);
  mrbc_define_method(0, gpio, "read",       mrbc_esp32_gpio_get_level);
  mrbc_define_method(0, gpio, "high?",      mrbc_esp32_gpio_get_high);
  mrbc_define_method(0, gpio, "low?",       mrbc_esp32_gpio_get_low);
  mrbc_define_method(0, gpio, "write",      mrbc_esp32_gpio_set_level);
  
  //定数
  mrbc_set_class_const(gpio, mrbc_str_to_symid("IN"),         &mrbc_integer_value(GPIO_IN));
  mrbc_set_class_const(gpio, mrbc_str_to_symid("OUT"),        &mrbc_integer_value(GPIO_OUT));
  mrbc_set_class_const(gpio, mrbc_str_to_symid("PULL_UP"),    &mrbc_integer_value(GPIO_PULL_UP));
  mrbc_set_class_const(gpio, mrbc_str_to_symid("PULL_DOWN"),  &mrbc_integer_value(GPIO_PULL_DOWN));

  ////////////////////////////////////////////////////
  // MicroPython 同様に Pin クラスも定義しておく
  //
  
  //mrbc_define_class でクラス名を定義
  mrbc_class *gpio2 = mrbc_define_class(0, "Pin", 0);

  //mrbc_define_method でメソッドを定義
  mrbc_define_method(0, gpio2, "new",        mrbc_esp32_gpio_new);
  mrbc_define_method(0, gpio2, "initialize", mrbc_esp32_gpio_initialize);
  mrbc_define_method(0, gpio2, "value",      mrbc_esp32_gpio_get_level);
  mrbc_define_method(0, gpio2, "on",         mrbc_esp32_gpio_set_on);
  mrbc_define_method(0, gpio2, "off",        mrbc_esp32_gpio_set_off);  
  
  //定数
  mrbc_set_class_const(gpio2, mrbc_str_to_symid("IN"),         &mrbc_integer_value(GPIO_IN));
  mrbc_set_class_const(gpio2, mrbc_str_to_symid("OUT"),        &mrbc_integer_value(GPIO_OUT));
  mrbc_set_class_const(gpio2, mrbc_str_to_symid("PULL_UP"),    &mrbc_integer_value(GPIO_PULL_UP));
  mrbc_set_class_const(gpio2, mrbc_str_to_symid("PULL_DOWN"),  &mrbc_integer_value(GPIO_PULL_DOWN));
}
