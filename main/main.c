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
#include "mrbc_esp32_gpio.h"
#include "mrbc_esp32_ledc.h"
#include "mrbc_esp32_adc.h"
#include "mrbc_esp32_uart.h"
#include "mrbc_esp32_i2c.h"
#include "mrbc_esp32_wifi.h"
#include "mrbc_esp32_sntp.h"
#include "mrbc_esp32_http_client.h"
#include "mrbc_esp32_sleep.h"
#include "mrbc_esp32_spi.h"

//*********************************************
// ENABLE MASTER files written by mruby/c
//*********************************************
#include "master.h"
#include "slave.h"

//***********************************************************
// BEGIN COMPONENTS 1: INCLUDE CLASS files associated with mruby/c
//
// END COMPONENTS 1
//-----------------------------------------------------------

static const char *TAG = "iotex-esp32-mrubyc";

#define MEMORY_SIZE (1024*70)
#define WAIT_TIME 100

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


void app_main(void) {

  nvs_flash_init();

  mrbc_init(memory_pool, MEMORY_SIZE);

  printf("start GPIO (C)\n");
  mrbc_esp32_gpio_gem_init(0);
  printf("start PWM (C)\n");
  mrbc_esp32_ledc_gem_init(0);
  printf("start ADC (C)\n");
  mrbc_esp32_adc_gem_init(0);
  printf("start I2C (C)\n");
  mrbc_esp32_i2c_gem_init(0);
  printf("start UART (C)\n");
  mrbc_esp32_uart_gem_init(0);
  printf("start WiFi (C) \n");
  mrbc_esp32_wifi_gem_init(0);
  mrbc_esp32_sntp_gem_init(0);
  mrbc_esp32_httpclient_gem_init(0);
  printf("start SLEEP (C) \n");
  mrbc_esp32_sleep_gem_init(0);
  printf("start SPI (C) \n");
  mrbc_esp32_spi_gem_init(0);

//***********************************************************
// BEGIN COMPONENTS 2: INCLUDE CLASS files associated with mruby/c
//
// END COMPONENTS 2
//-----------------------------------------------------------
    
  // Ruby 側のクラス・メソッド定義
  extern const uint8_t mrblib_bytecode[];
  mrbc_run_mrblib(mrblib_bytecode);

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
  mrbc_create_task( slave, 0 );
#else
  printf("SPIFFS mode\n");
  uint8_t *master = load_mrb_file("/spiffs/master.mrbc");
  mrbc_create_task(master, 0);
  uint8_t *slave = load_mrb_file("/spiffs/slave.mrbc");
  mrbc_create_task( slave, 0 );
#endif

  mrbc_run();

}

