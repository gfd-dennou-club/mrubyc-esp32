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

#include <stdlib.h>

#include "driver/sdmmc_host.h"

static char* TAG = "SDSPI";

static struct RClass* mrbc_class_esp32_sdspi;

typedef struct SDSPI_HANDLE {
  spi_host_device_t unit;
  sdmmc_host_t host; // init after.
  sdmmc_card_t * card;
  char * mount_point;
  int cs_pin;
} SDSPI_HANDLE;
  
static void mrbc_esp32_sdspi_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  //インスタンス作成
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(SDSPI_HANDLE));

  //initialize を call
  mrbc_instance_call_initialize( vm, v, argc );
  
  vTaskDelay(100 / portTICK_PERIOD_MS);  //wait
}


/*! initializer

*/
static void mrbc_esp32_sdspi_initialize(mrbc_vm *vm, mrbc_value v[], int argc)
{
  //初期化
  SDSPI_HANDLE handle;
  sdmmc_host_t h_copy = SDSPI_HOST_DEFAULT();
  handle.host = h_copy;
  handle.card = NULL;
  handle.mount_point = NULL;
  handle.unit = *((spi_host_device_t *)(v[1].instance->data));
  handle.cs_pin = -1;
  
  //オプション解析
  MRBC_KW_ARG(cs_pin);
  if( MRBC_ISNUMERIC(cs_pin) ) {
    handle.cs_pin = MRBC_TO_INT(cs_pin);
  }

  // instance->data を構造体へのポインタとみなして、値を代入する。
  *((SDSPI_HANDLE *)(v[0].instance->data)) = handle;
  
  ESP_LOGI(TAG, "SDSPI initial");
  ESP_LOGI(TAG, "unit: %d", handle.unit);
  ESP_LOGI(TAG, "cs:   %d", handle.cs_pin);
  ESP_LOGI(TAG, "slot: %d", handle.host.slot);
}


/*! Method esp_vfs_fat_sdcard_mount(chipselect, mountpoint) body : wrapper for esp_vfs_fat_sdcard_mount.
    @param chipselect ChipSelect Pin Number
           mountpoint Mount Point Directory Name
	   (optional) createiffailed  Create partition if mount failed.
    @return if succeeded, return true, otherwise return false;
*/
static void
mrbc_esp32_sdspi_esp_vfs_fat_sdspi_mount(mrb_vm* vm, mrb_value* v, int argc)
{  
  SDSPI_HANDLE handle = *((SDSPI_HANDLE *)(v[0].instance->data));
  ESP_LOGI(TAG, "unit: %d", handle.unit);
  ESP_LOGI(TAG, "cs:   %d", handle.cs_pin);
  
  printf("mount point: %s", (char *)GET_STRING_ARG(1));
  
  if(handle.mount_point != NULL) {
    SET_FALSE_RETURN(); // can mount only one sd card.
    return;
  }

  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = handle.cs_pin;
  slot_config.host_id = handle.unit;
  
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = (argc >= 2 && GET_ARG(2).tt == MRBC_TT_TRUE),
    .max_files = 8,
    .allocation_unit_size = 16 * 1024
  };

  esp_err_t ret = esp_vfs_fat_sdspi_mount((char *)GET_STRING_ARG(1), &handle.host, &slot_config, &mount_config, &handle.card);
  if(ret == ESP_OK)
  {
    handle.mount_point = (char *)malloc((mrbc_string_size(&(v[1])) + 1) * sizeof(char));
    memcpy(handle.mount_point, GET_STRING_ARG(1), mrbc_string_size(&(v[1])) + 1);

    // instance->data を構造体へのポインタとみなして、値を代入する。
    *((SDSPI_HANDLE *)(v[0].instance->data)) = handle;
    
    SET_TRUE_RETURN();
  }
  else 
    SET_FALSE_RETURN(); // if ESP_FAIL, filesystem is invalid, other is sd card should have pull-up resistors in place.
}

/*! Method esp_vfs_fat_sdcard_unmount() body : wrapper for esp_vfs_fat_sdcard_unmount.
    @return if succeeded, return true, otherwise return false;
*/
static void
mrbc_esp32_sdspi_esp_vfs_fat_sdcard_unmount(mrb_vm* vm, mrb_value *v, int argc)
{
  SDSPI_HANDLE handle = *((SDSPI_HANDLE *)(v[0].instance->data));
  
  if(handle.card == NULL && handle.mount_point == NULL) // not initialized.
  {
    SET_FALSE_RETURN();
    return;
  }
  esp_vfs_fat_sdcard_unmount(handle.mount_point, handle.card);
  free(handle.mount_point);
  handle.mount_point = NULL;
  handle.card = NULL;
  SET_TRUE_RETURN();
}

/*
static void
mrbc_esp32_sdspi_spi_bus_free(mrb_vm* vm, mrb_value* v, int argc)
{
  spi_bus_free(host.slot);
}
*/

/*! Register SDSPI Class.
 */
void
mrbc_esp32_sdspi_gem_init(struct VM* vm)
{
  mrbc_class_esp32_sdspi = mrbc_define_class(vm, "SDSPI", mrbc_class_object);
  mrbc_define_method(vm, mrbc_class_esp32_sdspi, "new",        mrbc_esp32_sdspi_new);
  mrbc_define_method(vm, mrbc_class_esp32_sdspi, "initialize", mrbc_esp32_sdspi_initialize); 
  mrbc_define_method(vm, mrbc_class_esp32_sdspi, "mount",  mrbc_esp32_sdspi_esp_vfs_fat_sdspi_mount);
  mrbc_define_method(vm, mrbc_class_esp32_sdspi, "umount", mrbc_esp32_sdspi_esp_vfs_fat_sdcard_unmount);
}
