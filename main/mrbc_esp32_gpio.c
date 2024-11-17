/*! @file
  @brief
  mruby/c GPIO functions for ESP32
*/

#include "mrbc_esp32_gpio.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

static char* TAG = "GPIO";

/*! constructor(pin, mode, pull_mode)
  
  gpio = GPIO.new(pin, GPIO::IN, GPIO::PULL_UP )
  gpio = GPIO.new(pin, GPIO::OUT )
*/
static void mrbc_esp32_gpio_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(gpio_num_t));

  //initialize を call
  mrbc_instance_call_initialize( vm, v, argc );

  vTaskDelay(100 / portTICK_PERIOD_MS);  //wait
}

/*! initializer()

 */
static void mrbc_esp32_gpio_initialize(mrbc_vm *vm, mrbc_value v[], int argc)
{
  gpio_num_t  pin  = GET_INT_ARG(1);
  gpio_mode_t mode = GET_INT_ARG(2);
  gpio_pull_mode_t pull_mode = GET_INT_ARG(3);

  // instance->data をgpio_num_t 型 へのポインタとみなして、値を代入する。
  *((gpio_num_t *)(v[0].instance->data)) = GET_INT_ARG(1); 

#ifdef CONFIG_USE_MRUBYC_DEBUG
  ESP_LOGI(TAG, "pin:  %i", pin);
  ESP_LOGI(TAG, "mode: %i", mode);
  ESP_LOGI(TAG, "pull: %i", pull_mode);
#endif
  
  ESP_ERROR_CHECK( gpio_reset_pin(pin) );
  ESP_ERROR_CHECK( gpio_set_direction(pin, mode) );
  
  if (pull_mode > 0){
    ESP_ERROR_CHECK( gpio_set_pull_mode(pin, pull_mode) );
  }
}


/*! setmode(pin, mode, pull_mode)
  
  @param pin GPIO ピン番号
  @param mode，GPIO::IN, GPIO::OUT
  @param pull_mode，GPIO::PULL_UP, GPIO::PULL_DOWN, GPIO::PILL_DRAIN
*/
static void
mrbc_esp32_gpio_setmode(mrb_vm* vm, mrb_value* v, int argc){
  gpio_num_t  pin  = *((gpio_num_t *)(v[0].instance->data));
  gpio_mode_t mode = GET_INT_ARG(1);
  gpio_pull_mode_t pull_mode = GET_INT_ARG(2);

#ifdef CONFIG_USE_MRUBYC_DEBUG
  ESP_LOGI(TAG, "pin:  %i", pin);
  ESP_LOGI(TAG, "mode: %i", mode);
  ESP_LOGI(TAG, "pull: %i", pull_mode);
#endif
  
  ESP_ERROR_CHECK( gpio_set_direction(pin, mode) );
  
  if (pull_mode){
    ESP_ERROR_CHECK( gpio_set_pull_mode(pin, pull_mode) );
  }
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
  mrbc_define_method(0, gpio, "setmode",    mrbc_esp32_gpio_setmode);
  mrbc_define_method(0, gpio, "read",       mrbc_esp32_gpio_get_level);
  mrbc_define_method(0, gpio, "high?",      mrbc_esp32_gpio_get_high);
  mrbc_define_method(0, gpio, "low?",       mrbc_esp32_gpio_get_low);
  mrbc_define_method(0, gpio, "write",      mrbc_esp32_gpio_set_level);
  
  //定数
  mrbc_set_class_const(gpio, mrbc_str_to_symid("IN"),         &mrbc_integer_value(GPIO_MODE_INPUT));
  mrbc_set_class_const(gpio, mrbc_str_to_symid("OUT"),        &mrbc_integer_value(GPIO_MODE_OUTPUT));
  mrbc_set_class_const(gpio, mrbc_str_to_symid("PULL_UP"),    &mrbc_integer_value(GPIO_PULLUP_ONLY));
  mrbc_set_class_const(gpio, mrbc_str_to_symid("PULL_DOWN"),  &mrbc_integer_value(GPIO_PULLDOWN_ONLY));
  mrbc_set_class_const(gpio, mrbc_str_to_symid("OPEN_DRAIN"), &mrbc_integer_value(GPIO_MODE_INPUT_OUTPUT_OD));

  ////////////////////////////////////////////////////
  // MicroPython 同様に Pin クラスも定義しておく
  //
  
  //mrbc_define_class でクラス名を定義
  mrbc_class *gpio2 = mrbc_define_class(0, "Pin", 0);

  //mrbc_define_method でメソッドを定義
  mrbc_define_method(0, gpio2, "new",        mrbc_esp32_gpio_new);
  mrbc_define_method(0, gpio2, "initialize", mrbc_esp32_gpio_initialize);
  mrbc_define_method(0, gpio2, "setmode",    mrbc_esp32_gpio_setmode);
  mrbc_define_method(0, gpio2, "value",      mrbc_esp32_gpio_get_level);
  mrbc_define_method(0, gpio2, "on",         mrbc_esp32_gpio_set_on);
  mrbc_define_method(0, gpio2, "off",        mrbc_esp32_gpio_set_off);  
  
  //定数
  mrbc_set_class_const(gpio2, mrbc_str_to_symid("IN"),         &mrbc_integer_value(GPIO_MODE_INPUT));
  mrbc_set_class_const(gpio2, mrbc_str_to_symid("OUT"),        &mrbc_integer_value(GPIO_MODE_OUTPUT));
  mrbc_set_class_const(gpio2, mrbc_str_to_symid("PULL_UP"),    &mrbc_integer_value(GPIO_PULLUP_ONLY));
  mrbc_set_class_const(gpio2, mrbc_str_to_symid("PULL_DOWN"),  &mrbc_integer_value(GPIO_PULLDOWN_ONLY));
  mrbc_set_class_const(gpio2, mrbc_str_to_symid("OPEN_DRAIN"), &mrbc_integer_value(GPIO_MODE_INPUT_OUTPUT_OD));
}
