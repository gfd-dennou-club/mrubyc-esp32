#include <stdio.h>

#include "esp_system.h"
#include "esp_log.h"
#include "esp_spi_flash.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include <ctype.h>
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
#ifdef CONFIG_USE_ESP32_SPI
#include "mrbc_esp32_spi.h"
#endif
#ifdef CONFIG_USE_ESP32_UART
#include "mrbc_esp32_uart.h"
#endif
#ifdef CONFIG_USE_ESP32_BLE
#include "mrbc_esp32_ble.h"
#endif

//*********************************************
// ENABLE CLASSES and MASTER files written by mruby/c
//
// #include "models/[replace with your file].h"
// #include "loops/[replace with your file].h"
//*********************************************
#ifdef CONFIG_USE_ESP32_GPIO
#include "models/gpio.h"
#endif
#ifdef CONFIG_USE_ESP32_PIN
#include "models/pin.h"
#endif
#ifdef CONFIG_USE_ESP32_GPIO_IRQHANDLER
#include "models/irq_handler.h"
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
#ifdef CONFIG_USE_ESP32_WIFI
#include "models/wlan.h"
#endif
#ifdef CONFIG_USE_ESP32_SPI
#include "models/spi.h"
#endif
#ifdef CONFIG_USE_ESP32_UART
#include "models/uart.h"
#endif
// master
#ifdef CONFIG_USE_ESP32_FIRMWAREFLASH
//master
#include "loops/master.h"
// slave
#ifdef CONFIG_ENABLE_MULTITASK
#include "loops/slave.h"
#endif
#endif

//***********************************************************
// BEGIN COMPONENTS 1: INCLUDE CLASS files associated with mruby/c
//
// END COMPONENTS 1
//-----------------------------------------------------------


static const char *TAG = "iotex-esp32-mrubyc";

#define MEMORY_SIZE (1024*40)

static uint8_t memory_pool[MEMORY_SIZE];

// SPIFFSでバイナリデータを読み込み
uint8_t * load_mrb_file(const char *filename)
{
  FILE *fp = fopen(filename, "rb");

  if( fp == NULL ) {
    fprintf(stderr, "File not found (%s)\n", filename);
    return NULL;
  }


  /* // get filesize */
  fseek(fp, 0, SEEK_END);
  size_t size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  // allocate memory
  uint8_t *p = malloc(size);
  if( p != NULL ) {
    fread(p, sizeof(uint8_t), size, fp);
  } else {
    fprintf(stderr, "Memory allocate error.\n");
  }
  fclose(fp);
  return p;
}

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

void app_main(void) {
  nvs_flash_init();
  mrbc_init(memory_pool, MEMORY_SIZE);

  mrbc_define_method(0, mrbc_class_object, "debugprint", c_debugprint);
  mrbc_define_method(0, mrbc_class_object, "floatCast", c_floatCast);
  /*
     !!!! Add your function                            !!!!
     !!!! example: mrbc_esp32_XXXX_gem_init(0);  !!!!
  */
#ifdef CONFIG_USE_ESP32_GPIO
  printf("start GPIO (C)\n");
  mrbc_esp32_gpio_gem_init(0);
#endif
#ifdef CONFIG_USE_ESP32_LEDC
  printf("start PWM (C)\n");
  mrbc_esp32_ledc_gem_init(0);
#endif
#ifdef CONFIG_USE_ESP32_ADC
  printf("start ADC (C)\n");
  mrbc_esp32_adc_gem_init(0);
#endif
#ifdef CONFIG_USE_ESP32_I2C
  printf("start I2C (C)\n");
  mrbc_esp32_i2c_gem_init(0);
#endif
#ifdef CONFIG_USE_ESP32_WIFI
  printf("start WiFi (C) \n");
  mrbc_esp32_wifi_gem_init(0);
#endif
#ifdef CONFIG_USE_ESP32_SLEEP
  printf("start SLEEP (C) \n");
  mrbc_esp32_sleep_gem_init(0);
#endif
#ifdef CONFIG_USE_ESP32_SNTP
  printf("start SNTP (C) \n");
  mrbc_esp32_sntp_gem_init(0);
#endif
#ifdef CONFIG_USE_ESP32_HTTP_CLIENT
  printf("start HTTPClient (C) \n");
  mrbc_esp32_httpclient_gem_init(0);
#endif
#ifdef CONFIG_USE_ESP32_SPI
  printf("start SPI (C) \n");
  mrbc_esp32_spi_gem_init(0);
#endif
#ifdef CONFIG_USE_ESP32_UART
  printf("start UART (C)\n");
  mrbc_esp32_uart_gem_init(0);
#endif
#ifdef CONFIG_USE_ESP32_BLE
  printf("start BLET (C)\n");
  mrbc_esp32_ble_gem_init(0);
#endif
  
  /*
     !!!! Add names of your ruby files                              !!!!
     !!!! example: mrbc_create_task( [replace with your task], 0 ); !!!!
  */

#ifdef CONFIG_USE_ESP32_GPIO
  printf("start GPIO (mruby/c class)\n");
  mrbc_create_task( gpio, 0 );
#endif
#ifdef CONFIG_USE_ESP32_PIN
  printf("start PIN (mruby/c class)\n");
  mrbc_create_task( pin, 0 );
#endif  
#ifdef CONFIG_USE_ESP32_GPIO_IRQHANDLER
  printf("start GPIO IRQHandler (mruby/c task)\n");
  mrbc_create_task( irq_handler, 0 );
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
#ifdef CONFIG_USE_ESP32_WIFI
  printf("start WLAN (mruby/c class) \n");
  mrbc_create_task(wlan, 0);
#endif
#ifdef CONFIG_USE_ESP32_SPI
  printf("start SPI (mruby/c class)\n");
  mrbc_create_task( spi, 0 );
#endif
#ifdef CONFIG_USE_ESP32_UART
  printf("start UART (mruby/c class)\n");
  mrbc_create_task(uart, 0);
#endif

//***********************************************************
// BEGIN COMPONENTS 2: INCLUDE CLASS files associated with mruby/c
//
// END COMPONENTS 2
//-----------------------------------------------------------
 
  //master
  vTaskDelay(1000 / portTICK_RATE_MS);
  esp_vfs_spiffs_conf_t conf = {
    .base_path = "/spiffs",
    .partition_label = NULL,
    .max_files = 2,
    .format_if_mount_failed = true
  };
    
  // Use settings defined above to initialize and mount SPIFFS filesystem.
  // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount or format filesystem");
    } else if (ret == ESP_ERR_NOT_FOUND) {
      ESP_LOGE(TAG, "Failed to find SPIFFS partition");
    } else {
      ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
    }
    return;
  }
  size_t total = 0, used = 0;
  ret = esp_spiffs_info(conf.partition_label, &total, &used);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
  } else {
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
  }

  // master
#ifdef CONFIG_USE_ESP32_FIRMWAREFLASH
  printf("FIRMWAREFLASH mode\n");
  mrbc_create_task(master, 0);
#ifdef CONFIG_ENABLE_MULTITASK
  mrbc_create_task( slave, 0 );
#endif
#else
  printf("SPIFFS mode\n");
  uint8_t *master = load_mrb_file("/spiffs/master.mrbc");
  mrbc_create_task(master, 0);
  //slave
#ifdef CONFIG_ENABLE_MULTITASK
  uint8_t *slave = load_mrb_file("/spiffs/slave.mrbc");
  mrbc_create_task( slave, 0 );
#endif
#endif
  mrbc_run();
}

