/*! @file
  @brief
  mruby/c GPIO functions for ESP32
*/

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "mrbc_esp32_gpio.h"

#define ESP_INTR_FLAG_DEFAULT 0

#define INTR_NUM 2

static int isr_state[INTR_NUM];
static int ISR_NOT_STARTED_FLAG = 1;

static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
  uint32_t gpio_num = (uint32_t) arg;
  xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void* arg)
{
  uint32_t io_num;
  for(;;) {
    if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)){
      isr_state[0] = io_num;
      isr_state[1] = gpio_get_level(io_num);
      printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
    }
  }
}

static void gpio_isr_start()
{
  //install gpio isr service
  gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

  //create a queue to handle gpio event from isr
  gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

  //start gpio task
  xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

  //MESSAGE
  printf("START ISR SERVICE\n");
}

static void
mrbc_esp32_gpio_isr_state(mrb_vm* vm, mrb_value* v, int argc)
{
  mrbc_value result = mrbc_array_new(vm, INTR_NUM);
  
  // Array インスタンス result に Fixnum インスタンスとしてデータをセット
  for ( int x = 0; x < INTR_NUM; ++x ) {
    mrbc_array_set(&result, x, &mrbc_fixnum_value(isr_state[x]));
  }
  
  // Array インスタンス result を本メソッドの返り値としてセット
  SET_RETURN(result);
}

static void
mrbc_esp32_gpio_set_intr(mrb_vm* vm, mrb_value* v, int argc)
{
  uint32_t pin = GET_INT_ARG(1);
  uint8_t mode = GET_INT_ARG(2);
  
  printf("pin: %d, mode: %d\n", pin, mode);
  gpio_set_intr_type(pin, mode);

  if (ISR_NOT_STARTED_FLAG) {
    gpio_isr_start();
    ISR_NOT_STARTED_FLAG = 0;
  }
    
  //hook isr handler for specific gpio pin
  gpio_isr_handler_add(pin, gpio_isr_handler, (void*) pin); 
}

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

/*! クラス定義処理を記述した関数

  @param vm mruby/c VM
*/
void
mrbc_esp32_gpio_gem_init(struct VM* vm)
{
  //メソッド定義
  mrbc_define_method(0, mrbc_class_object, "gpio_set_intr",            mrbc_esp32_gpio_set_intr);
  mrbc_define_method(0, mrbc_class_object, "gpio_isr_state",           mrbc_esp32_gpio_isr_state);
  mrbc_define_method(0, mrbc_class_object, "gpio_reset_pin",           mrbc_esp32_gpio_reset_pin);
  mrbc_define_method(0, mrbc_class_object, "gpio_set_pullup",          mrbc_esp32_gpio_set_pullup);
  mrbc_define_method(0, mrbc_class_object, "gpio_set_pulldown",        mrbc_esp32_gpio_set_pulldown);
  mrbc_define_method(0, mrbc_class_object, "gpio_wakeup_enable",       mrbc_esp32_gpio_wakeup_enable);
  mrbc_define_method(0, mrbc_class_object, "gpio_wakeup_disable",      mrbc_esp32_gpio_wakeup_disable);
  mrbc_define_method(0, mrbc_class_object, "gpio_set_floating",        mrbc_esp32_gpio_set_floating);
  mrbc_define_method(0, mrbc_class_object, "gpio_set_hold_enable",     mrbc_esp32_gpio_set_hold_enable);
  mrbc_define_method(0, mrbc_class_object, "gpio_set_hold_disable",    mrbc_esp32_gpio_set_hold_disable);
  mrbc_define_method(0, mrbc_class_object, "gpio_set_mode_input",      mrbc_esp32_gpio_set_mode_input);
  mrbc_define_method(0, mrbc_class_object, "gpio_set_mode_output",     mrbc_esp32_gpio_set_mode_output);
  mrbc_define_method(0, mrbc_class_object, "gpio_set_mode_open_drain", mrbc_esp32_gpio_set_mode_open_drain);
  mrbc_define_method(0, mrbc_class_object, "gpio_set_level",           mrbc_esp32_gpio_set_level);
  mrbc_define_method(0, mrbc_class_object, "gpio_get_level",           mrbc_esp32_gpio_get_level);
}
