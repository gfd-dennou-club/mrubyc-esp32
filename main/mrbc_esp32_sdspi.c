/*! @file
  @brief
  mruby/c SPI connected SD low-level class for ESP32
*/

#include "mrbc_esp32_sdspi.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include <string.h>

#include "driver/sdmmc_host.h"

static char* TAG = "SDSPI";

typedef struct SDSPI_HANDLE {
  spi_host_device_t unit;
  sdmmc_host_t host;
  sdmmc_card_t * card;
  char * mount_point;
  int cs_pin;
} SDSPI_HANDLE;
  
static void mrbc_esp32_sdspi_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(SDSPI_HANDLE));
  mrbc_instance_call_initialize( vm, v, argc );
  vTaskDelay(100 / portTICK_PERIOD_MS);
}

/*! initializer
    Usage: SDSPI.new(spi, cs_pin: 5, format: false)
*/
static void mrbc_esp32_sdspi_initialize(mrbc_vm *vm, mrbc_value v[], int argc)
{
  // 1. ハンドラの初期化
  SDSPI_HANDLE handle;
  sdmmc_host_t h_copy = SDSPI_HOST_DEFAULT();
  handle.host = h_copy;
  handle.card = NULL;
  handle.mount_point = NULL;
  
  // 第1引数の SPI オブジェクトから unit を取得
  handle.unit = *((spi_host_device_t *)(v[1].instance->data));
  handle.cs_pin = -1;
  
  // 2. キーワード引数の解析
  MRBC_KW_ARG(cs_pin, format, mount_point);
  if( MRBC_ISNUMERIC(cs_pin) ) {
    handle.cs_pin = MRBC_TO_INT(cs_pin);
  }
  
  // 失敗時にフォーマットするかどうか (デフォルト false)
  bool format_if_failed = (format.tt == MRBC_TT_TRUE);

  // 3. マウント処理 (ルート "/" に固定)
  const char *target_path;
  if (mount_point.tt == MRBC_TT_STRING) {
    target_path = (const char *)mount_point.string->data;
  } else {
    target_path = "/sd"; // デフォルト値
  }
  
  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = handle.cs_pin;
  slot_config.host_id = handle.unit;
  
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = format_if_failed,
    .max_files = 8,
    .allocation_unit_size = 16 * 1024
  };

  ESP_LOGI(TAG, "Mounting SD card to '%s'...", target_path);
  
  esp_err_t ret = esp_vfs_fat_sdspi_mount(target_path, &handle.host, &slot_config, &mount_config, &handle.card);
  
  if(ret == ESP_OK) {
    handle.mount_point = strdup(target_path);
    ESP_LOGI(TAG, "SD card mounted successfully.");
  } else {
    ESP_LOGE(TAG, "Failed to mount SD card (0x%x).", ret);
    handle.mount_point = NULL;
    handle.card = NULL;
  }

  // インスタンスデータに構造体を保存
  *((SDSPI_HANDLE *)(v[0].instance->data)) = handle;

}

/*! Method umount() body
*/
static void
mrbc_esp32_sdspi_esp_vfs_fat_sdcard_unmount(mrb_vm* vm, mrb_value *v, int argc)
{
  SDSPI_HANDLE *handle = (SDSPI_HANDLE *)(v[0].instance->data);
  
  if(handle->card == NULL || handle->mount_point == NULL) {
    SET_FALSE_RETURN();
    return;
  }

  esp_err_t ret = esp_vfs_fat_sdcard_unmount(handle->mount_point, handle->card);
  if (ret == ESP_OK) {
    free(handle->mount_point);
    handle->mount_point = NULL;
    handle->card = NULL;
    SET_TRUE_RETURN();
  } else {
    SET_FALSE_RETURN();
  }
}

/*! Register SDSPI Class.
 */
void
mrbc_esp32_sdspi_gem_init(struct VM* vm)
{
  mrbc_class *sdspi = mrbc_define_class(0, "SDSPI", 0);
  mrbc_define_method(0, sdspi, "new",        mrbc_esp32_sdspi_new);
  mrbc_define_method(0, sdspi, "initialize", mrbc_esp32_sdspi_initialize); 
  // mount メソッドの定義を削除して隠蔽
  mrbc_define_method(0, sdspi, "umount",     mrbc_esp32_sdspi_esp_vfs_fat_sdcard_unmount);
}
