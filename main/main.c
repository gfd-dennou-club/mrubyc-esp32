#include <stdio.h>

#include "esp_system.h"
#include "esp_log.h"
#include "esp_spi_flash.h"
#include "nvs_flash.h"

#include "mrubyc.h"

// ENABLE LIBRARY
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
#ifdef CONFIG_USE_ESP32_SNTP
#include "mrbc_esp32_sntp.h"
#endif
#ifdef CONFIG_USE_ESP32_HTTP_CLIENT
#include "mrbc_esp32_http_client.h"
#endif

// #include "models/[replace with your file].h"
// #include "loops/[replace with your file].h"
#ifdef CONFIG_USE_ESP32_GPIO
#include "models/gpio.h"
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
#include "models/aqm0802a.h"
#include "models/rc8035sa.h"
#endif

#include "models/irq_handler.h"
#include "loops/master.h"


#define MEMORY_SIZE (1024*40)

static uint8_t memory_pool[MEMORY_SIZE];

//================================================================
/*! DEBUG PRINT
*/
void chip_info() {
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

static void c_debugprint(struct VM *vm, mrbc_value v[], int argc){
  for( int i = 0; i < 79; i++ ) { console_putchar('='); }
  console_putchar('\n');
  chip_info();
  int total, used, free, fragment;
  mrbc_alloc_statistics( &total, &used, &free, &fragment );
  console_printf("Memory total:%d, used:%d, free:%d, fragment:%d\n", total, used, free, fragment );
  unsigned char *key = GET_STRING_ARG(1);
  unsigned char *value = GET_STRING_ARG(2);
  console_printf("%s:%s\n", key, value );
  heap_caps_print_heap_info(MALLOC_CAP_8BIT);
  heap_caps_print_heap_info(MALLOC_CAP_32BIT);
  for( int i = 0; i < 79; i++ ) { console_putchar('='); }
  console_putchar('\n');
}


void app_main(void) {
  nvs_flash_init();
  mrbc_init(memory_pool, MEMORY_SIZE);

  mrbc_define_method(0, mrbc_class_object, "debugprint", c_debugprint);

  /* 
     !!!! Add your function                            !!!!
     !!!! example: mrbc_mruby_esp32_XXXX_gem_init(0);  !!!!
  */
#ifdef CONFIG_USE_ESP32_GPIO
  mrbc_mruby_esp32_gpio_gem_init(0);
#endif
#ifdef CONFIG_USE_ESP32_LEDC
  mrbc_mruby_esp32_ledc_gem_init(0);
#endif
#ifdef CONFIG_USE_ESP32_ADC
  mrbc_mruby_esp32_adc_gem_init(0);
#endif
#ifdef CONFIG_USE_ESP32_I2C
  mrbc_mruby_esp32_i2c_gem_init(0);
#endif
#ifdef CONFIG_USE_ESP32_WIFI
  mrbc_mruby_esp32_wifi_gem_init(0);
#endif
#ifdef CONFIG_USE_ESP32_SNTP
  mrbc_mruby_esp32_sntp_gem_init(0);
#endif

  /*
     !!!! Add names of your ruby files                              !!!!
     !!!! example: mrbc_create_task( [replace with your task], 0 ); !!!!
  */
#ifdef CONFIG_USE_ESP32_GPIO
  printf("start GPIO\n");
  mrbc_create_task( gpio, 0 );
#endif
#ifdef CONFIG_USE_ESP32_LEDC
  printf("start PWM\n");
  mrbc_create_task( pwm, 0 );
#endif
#ifdef CONFIG_USE_ESP32_ADC
  printf("start ADC\n");
  mrbc_create_task( adc, 0 );
#endif
#ifdef CONFIG_USE_ESP32_I2C
  printf("start I2C\n");
  mrbc_create_task( i2c, 0 );
  mrbc_create_task( sht75, 0 );
#endif
#ifdef CONFIG_USE_ESP32_WIFI
  printf("start WiFi\n");
  mrbc_create_task( aqm0802a, 0 );
#endif
#ifdef CONFIG_USE_ESP32_SNTP
  printf("start SNTP\n");
  mrbc_create_task( rc8035sa, 0 );
#endif
#ifdef CONFIG_USE_ESP32_HTTP_CLIENT
  printf("start HTTPClient\n");
  mrbc_mruby_esp32_httpclient_gem_init(0);
#endif

  mrbc_create_task( irq_handler, 0 );
  mrbc_create_task( master, 0 );

  mrbc_run();
}
