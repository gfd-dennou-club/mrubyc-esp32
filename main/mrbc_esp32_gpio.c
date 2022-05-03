/*! @file
  @brief
  mruby/c GPIO functions for ESP32
*/

#include "mrbc_esp32_gpio.h"
#include "driver/gpio.h"
#define PIN_NUM 39


static struct RClass* mrbc_class_esp32_gpio;

static int unreferenced;

static int pins_state[PIN_NUM + 1];



/*!  reset_pin(pin) 本体 : wrapper for gpio_reset_pin

  @param pin GPIO ピン番号
*/
static void
mrbc_esp32_gpio_reset_pin(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  gpio_reset_pin(pin);
}

/*!  set_pullup(pin) 本体 : wrapper for gpio_set_pull_mode
  GPIO_PULLUP_ONLY 専用

  @param pin GPIO ピン番号
*/
static void
mrbc_esp32_gpio_set_pullup(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
}

/*!  set_pulldown(pin) 本体 : wrapper for gpio_set_pull_mode
  GPIO_PULLUP_DOWN 専用

  @param pin GPIO ピン番号
*/
static void
mrbc_esp32_gpio_set_pulldown(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  gpio_set_pull_mode(pin, GPIO_PULLDOWN_ONLY);
}

/*!  wakeup_enable(pin) 本体 : wrapper for gpio_wakeup_enable
  GPIO_PULLUP_ONLY 専用

  @param pin   GPIO ピン番号
  @param level Sleepモードから帰ってくるレベル
*/
static void
mrbc_esp32_gpio_wakeup_enable(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  gpio_int_type_t level = GET_INT_ARG(2);
  gpio_wakeup_enable(pin, level);
}

/*!  wakeup_disable(pin) 本体 : wrapper for gpio_wakeup_disable
  GPIO_PULLUP_ONLY 専用

  @param pin   GPIO ピン番号
*/
static void
mrbc_esp32_gpio_wakeup_disable(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  gpio_wakeup_disable(pin);
}

/*!  set_floating(pin) 本体 : wrapper for gpio_set_pull_mode
  GPIO_FLOATING 専用

  @param pin GPIO ピン番号
*/
static void
mrbc_esp32_gpio_set_floating(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  gpio_set_pull_mode(pin, GPIO_FLOATING);
}

/*!  set_hold_enable(pin) 本体 : wrapper for gpio_hold_en

  @param pin GPIO ピン番号
*/
static void
mrbc_esp32_gpio_set_hold_enable(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  gpio_hold_en(pin);
}

/*!  set_hold_enable(pin) 本体 : wrapper for gpio_hold_dis

  @param pin GPIO ピン番号
*/
static void
mrbc_esp32_gpio_set_hold_disable(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  gpio_hold_dis(pin);
}

/*!  set_mode_input(pin) 本体 : wrapper for gpio_set_direction
  GPIO_MODE_INPUT 専用

  @param pin GPIO ピン番号
*/
static void
mrbc_esp32_gpio_set_mode_input(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  gpio_pad_select_gpio(pin);
  gpio_set_direction(pin, GPIO_MODE_INPUT);
  //  gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
}


/*!  set_mode_input(pin) 本体 : wrapper for gpio_set_direction
  GPIO_MODE_OUTPUT 専用

  @param pin GPIO ピン番号
*/
static void
mrbc_esp32_gpio_set_mode_output(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  gpio_pad_select_gpio(pin);
  gpio_set_direction(pin, GPIO_MODE_OUTPUT);
}

/*!  set_mode_open_drain(pin) 本体 : wrapper for gpio_set_direction
  GPIO_MODE_INPUT_OUTPUT_OD 専用

  @param pin GPIO ピン番号
*/
static void
mrbc_esp32_gpio_set_mode_open_drain(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  gpio_pad_select_gpio(pin);
  gpio_set_direction(pin, GPIO_MODE_INPUT_OUTPUT_OD);
}

/*!  set_level(pin, level) 本体 : wrapper for gpio_set_level

  @param pin   GPIO ピン番号
  @param level ピンレベル、0 : low / 1 : high
*/
static void
mrbc_esp32_gpio_set_level(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin   = GET_INT_ARG(1);
  int level = GET_INT_ARG(2);
  gpio_set_level(pin, level);
}


/*!  get_level(pin) 本体 : wrapper for gpio_get_level

  @param pin GPIO ピン番号
  @return    ピンレベル、0 : low / 1 : high
*/
static void
mrbc_esp32_gpio_get_level(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  // Fixnum インスタンスを本関数の返り値としてセット、値は gpio_get_level(pin) と同値
  SET_INT_RETURN(gpio_get_level(pin));
}

/*!  get_pin_state(pin) IRQ 制御用の状態管理関数

  @param pin GPIO ピン番号
  @return    トリガーの状態
*/
static void
mrbc_esp32_gpio_get_pin_state(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  int value = gpio_get_level(pin);
  int result = 4 << value;
  if(value != pins_state[pin])
    result |= 1 << pins_state[pin];
  pins_state[pin] = value;
  SET_INT_RETURN(result);
}

/*! クラス定義処理を記述した関数

  @param vm mruby/c VM
*/
void
mrbc_esp32_gpio_gem_init(struct VM* vm)
{
  //ISR ハンドラーサービスのインストール
  gpio_install_isr_service(0);

  //メソッド定義
  mrbc_define_method(vm, mrbc_class_esp32_gpio, "gpio_reset_pin",           mrbc_esp32_gpio_reset_pin);
  mrbc_define_method(vm, mrbc_class_esp32_gpio, "gpio_set_pullup",          mrbc_esp32_gpio_set_pullup);
  mrbc_define_method(vm, mrbc_class_esp32_gpio, "gpio_set_pulldown",        mrbc_esp32_gpio_set_pulldown);
  mrbc_define_method(vm, mrbc_class_esp32_gpio, "gpio_wakeup_enable",       mrbc_esp32_gpio_wakeup_enable);
  mrbc_define_method(vm, mrbc_class_esp32_gpio, "gpio_wakeup_disable",      mrbc_esp32_gpio_wakeup_disable);
  mrbc_define_method(vm, mrbc_class_esp32_gpio, "gpio_set_floating",        mrbc_esp32_gpio_set_floating);
  mrbc_define_method(vm, mrbc_class_esp32_gpio, "gpio_set_hold_enable",     mrbc_esp32_gpio_set_hold_enable);
  mrbc_define_method(vm, mrbc_class_esp32_gpio, "gpio_set_hold_disable",    mrbc_esp32_gpio_set_hold_disable);
  mrbc_define_method(vm, mrbc_class_esp32_gpio, "gpio_set_mode_input",      mrbc_esp32_gpio_set_mode_input);
  mrbc_define_method(vm, mrbc_class_esp32_gpio, "gpio_set_mode_output",     mrbc_esp32_gpio_set_mode_output);
  mrbc_define_method(vm, mrbc_class_esp32_gpio, "gpio_set_mode_open_drain", mrbc_esp32_gpio_set_mode_open_drain);
  mrbc_define_method(vm, mrbc_class_esp32_gpio, "gpio_set_level",           mrbc_esp32_gpio_set_level);
  mrbc_define_method(vm, mrbc_class_esp32_gpio, "gpio_get_level",           mrbc_esp32_gpio_get_level);
  mrbc_define_method(vm, mrbc_class_esp32_gpio, "gpio_get_pin_state",       mrbc_esp32_gpio_get_pin_state);
}
