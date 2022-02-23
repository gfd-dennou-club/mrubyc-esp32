#include <stdio.h>

#include "esp_system.h"
#include "esp_log.h"
#include "esp_spi_flash.h"
#include "nvs_flash.h"

#include "mrubyc.h"

//*********************************************
// ENABLE LIBRARY written by C
//*********************************************
#ifdef CONFIG_USE_ESP32_GPIO
#include "mrbc_esp32_gpio.h"
#endif
#ifdef CONFIG_USE_ESP32_LEDC
#include "mrbc_esp32_ledc.h"
#endif
#ifdef CONFIG_USE_ESP32_ADC
#include "mrbc_esp32_adc.h"
#endif
#ifdef CONFIG_USE_ESP32_I2C
#include "mrbc_esp32_i2c.h"
#endif
#ifdef CONFIG_USE_ESP32_WIFI
#include "mrbc_esp32_wifi.h"
#endif
#ifdef CONFIG_USE_ESP32_SLEEP
#include "mrbc_esp32_sleep.h"
#endif
#ifdef CONFIG_USE_ESP32_SNTP
#include "mrbc_esp32_sntp.h"
#endif
#ifdef CONFIG_USE_ESP32_HTTP_CLIENT
#include "mrbc_esp32_http_client.h"
#endif
#ifdef CONFIG_USE_ESP32_SPI_SD
#include "mrbc_esp32_sdspi.h"
#include "mrbc_esp32_stdio.h"
#include "mrbc_esp32_dirent.h"
#endif
#ifdef CONFIG_USE_ESP32_UART
#include "mrbc_esp32_uart.h"
#endif

//*********************************************
// ENABLE CLASSES and MASTER files written by mruby/c
//
// #include "models/[replace with your file].h"
// #include "loops/[replace with your file].h"
//*********************************************
#ifdef CONFIG_USE_ESP32_GPIO
#include "models/gpio.h"
#include "models/irq_handler.h"
#endif
#ifdef CONFIG_USE_ESP32_GPIO_PERIPHERALS_SHT75
#include "models/sht75.h"
#endif
#ifdef CONFIG_USE_ESP32_LEDC
#include "models/pwm.h"
#endif
#ifdef CONFIG_USE_ESP32_ADC
#include "models/adc.h"
#endif
#ifdef CONFIG_USE_ESP32_I2C
#include "models/i2c.h"
#endif
#ifdef CONFIG_USE_ESP32_UART
#include "models/uart.h"
#endif
#ifdef CONFIG_USE_ESP32_I2C_PERIPHERALS_AQM0802A
#include "models/aqm0802a.h"
#endif
#ifdef CONFIG_USE_ESP32_I2C_PERIPHERALS_RC8035SA
#include "models/rc8035sa.h"
#endif
#ifdef CONFIG_USE_ESP32_I2C_PERIPHERALS_SGP30
#include "models/sgp30.h"
#endif
#ifdef CONFIG_USE_ESP32_I2C_PERIPHERALS_SCD30
#include "models/scd30.h"
#endif
#ifdef CONFIG_USE_ESP32_I2C_PERIPHERALS_MCP9808
#include "models/mcp9808.h"
#endif
// master
#include "loops/master.h"
// slave
#ifdef CONFIG_ENABLE_MULTITASK
#include "loops/slave.h"
#endif

#define MEMORY_SIZE (1024 * 40)

static uint8_t memory_pool[MEMORY_SIZE];

//================================================================
/*! cast
  ["01000101001101001110000100111100"].pack('B*').unpack('g') ができないために用意
*/
static void c_floatCast(struct VM *vm, mrbc_value *v, int argc)
{

  float Value;
  uint32_t val = 0;
  uint32_t arg1 = GET_INT_ARG(1);
  uint32_t arg2 = GET_INT_ARG(2);
  uint32_t arg3 = GET_INT_ARG(3);
  uint32_t arg4 = GET_INT_ARG(4);
  mrbc_value result;
  result = mrbc_array_new(vm, 0);

  val |= arg1;
  val <<= 8;
  val |= arg2;
  val <<= 8;
  val |= arg3;
  val <<= 8;
  val |= arg4;
  memcpy(&Value, &val, sizeof(Value));

  mrbc_array_set(&result, 0, &mrbc_fixnum_value(Value * 100.0));
  SET_RETURN(result);
  //   result = *(float*) &tempU32;
}

//================================================================
/*! DEBUG PRINT
 */
void chip_info()
{
  /* Print chip information */
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  printf("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
         chip_info.cores,
         (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
         (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

  printf("silicon revision %d, ", chip_info.revision);

  printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
         (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
}

static void c_debugprint(struct VM *vm, mrbc_value v[], int argc)
{
  for (int i = 0; i < 79; i++)
  {
    console_putchar('=');
  }
  console_putchar('\n');
  chip_info();
  int total, used, free, fragment;
  mrbc_alloc_statistics(&total, &used, &free, &fragment);
  console_printf("Memory total:%d, used:%d, free:%d, fragment:%d\n", total, used, free, fragment);
  unsigned char *key = GET_STRING_ARG(1);
  unsigned char *value = GET_STRING_ARG(2);
  console_printf("%s:%s\n", key, value);
  heap_caps_print_heap_info(MALLOC_CAP_8BIT);
  heap_caps_print_heap_info(MALLOC_CAP_32BIT);
  for (int i = 0; i < 79; i++)
  {
    console_putchar('=');
  }
  console_putchar('\n');
}

void app_main(void)
{
  nvs_flash_init();
  mrbc_init(memory_pool, MEMORY_SIZE);

  mrbc_define_method(0, mrbc_class_object, "debugprint", c_debugprint);
  mrbc_define_method(0, mrbc_class_object, "floatCast", c_floatCast);
  /*
     !!!! Add your function                            !!!!
     !!!! example: mrbc_mruby_esp32_XXXX_gem_init(0);  !!!!
  */
#ifdef CONFIG_USE_ESP32_GPIO
  printf("start GPIO (C)\n");
  mrbc_mruby_esp32_gpio_gem_init(0);
#endif
#ifdef CONFIG_USE_ESP32_LEDC
  printf("start PWM (C)\n");
  mrbc_mruby_esp32_ledc_gem_init(0);
#endif
#ifdef CONFIG_USE_ESP32_ADC
  printf("start ADC (C)\n");
  mrbc_mruby_esp32_adc_gem_init(0);
#endif
#ifdef CONFIG_USE_ESP32_I2C
  printf("start I2C (C)\n");
  mrbc_mruby_esp32_i2c_gem_init(0);
#endif
#ifdef CONFIG_USE_ESP32_WIFI
  printf("start WiFi (C) \n");
  mrbc_mruby_esp32_wifi_gem_init(0);
#endif
#ifdef CONFIG_USE_ESP32_SLEEP
  printf("start SLEEP (C) \n");
  mrbc_mruby_esp32_sleep_gem_init(0);
#endif
#ifdef CONFIG_USE_ESP32_SNTP
  printf("start SNTP (C) \n");
  mrbc_mruby_esp32_sntp_gem_init(0);
#endif
#ifdef CONFIG_USE_ESP32_HTTP_CLIENT
  printf("start HTTPClient (C) \n");
  mrbc_mruby_esp32_httpclient_gem_init(0);
#endif
#ifdef CONFIG_USE_ESP32_SPI_SD
  printf("start SDSPI and ESP32 stdio (C)\n");
  mrbc_mruby_esp32_sdspi_gem_init(0);
  mrbc_mruby_esp32_stdio_gem_init(0);
  mrbc_mruby_esp32_dirent_gem_init(0);
#endif
#ifdef CONFIG_USE_ESP32_UART
  printf("start UART (C)\n");
  mrbc_mruby_esp32_uart_gem_init(0);
#endif

  /*
     !!!! Add names of your ruby files                              !!!!
     !!!! example: mrbc_create_task( [replace with your task], 0 ); !!!!
  */
#ifdef CONFIG_USE_ESP32_GPIO
  printf("start GPIO (mruby/c class)\n");
  mrbc_create_task(gpio, 0);
  mrbc_create_task(irq_handler, 0);
#endif
#ifdef CONFIG_USE_ESP32_LEDC
  printf("start PWM (mruby/c class)\n");
  mrbc_create_task(pwm, 0);
#endif
#ifdef CONFIG_USE_ESP32_ADC
  printf("start ADC (mruby/c class)\n");
  mrbc_create_task(adc, 0);
#endif
#ifdef CONFIG_USE_ESP32_I2C
  printf("start I2C (mruby/c class)\n");
  mrbc_create_task(i2c, 0);
#endif
#ifdef CONFIG_USE_ESP32_UART
  printf("start UART (mruby/c class)\n");
  mrbc_create_task(uart, 0);
#endif
#ifdef CONFIG_USE_ESP32_GPIO_PERIPHERALS_SHT75
  printf("start SHT75 (mruby/c class)\n");
  mrbc_create_task(sht75, 0);
#endif
#ifdef CONFIG_USE_ESP32_I2C_PERIPHERALS_AQM0802A
  printf("start AQM0802A (mruby/c class)\n");
  mrbc_create_task(aqm0802a, 0);
#endif
#ifdef CONFIG_USE_ESP32_I2C_PERIPHERALS_RC8035SA
  printf("start RC8035SA (mruby/c class)\n");
  mrbc_create_task(rc8035sa, 0);
#endif
#ifdef CONFIG_USE_ESP32_I2C_PERIPHERALS_SGP30
  printf("start SGP30 (mruby/c class)\n");
  mrbc_create_task(sgp30, 0);
#endif
#ifdef CONFIG_USE_ESP32_I2C_PERIPHERALS_SCD30
  printf("start SCD30 (mruby/c class)\n");
  mrbc_create_task(scd30, 0);
#endif
#ifdef CONFIG_USE_ESP32_I2C_PERIPHERALS_MCP9808
  printf("start My MCP9808 (mruby/c class)\n");
  mrbc_create_task(mcp9808, 0);
#endif

  // master
  mrbc_create_task(master, 0);

  // slave
#ifdef CONFIG_ENABLE_MULTITASK
  mrbc_create_task(slave, 0);
#endif

  mrbc_run();
}
