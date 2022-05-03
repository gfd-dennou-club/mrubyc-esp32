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

#define SPI_DMA_CHAN host.slot

static struct RClass* mrbc_class_esp32_sdspi;

static sdmmc_host_t host; // init after.
static sdmmc_card_t * card;
static char * mount_point;

/*! Method spi_bus_initialize(mosi, miso, sclk) body : wrapper for spi_bus_initialize.

    @param mosi MOSI Pin Number
           miso MISO Pin Number
           sclk SPI Clock Pin Number
    @return if succeeded, return true, otherwise return false.
 */
static void
mrbc_esp32_sdspi_spi_bus_initialize(mrb_vm* vm, mrb_value* v, int argc)
{
  spi_bus_config_t bus_cfg = {
			      .mosi_io_num = GET_INT_ARG(1),
			      .miso_io_num = GET_INT_ARG(2),
			      .sclk_io_num = GET_INT_ARG(3),
			      .quadwp_io_num = -1,
			      .quadhd_io_num = -1,
			      .max_transfer_sz = 4000
  };
  esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CHAN);
  if(ret == ESP_OK)
    SET_TRUE_RETURN();
  else
    SET_FALSE_RETURN();
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
  if(mount_point != NULL) {
    SET_FALSE_RETURN(); // can mount only one sd card.
    return;
  }
  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = GET_INT_ARG(1);
  slot_config.host_id = host.slot;
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
						   .format_if_mount_failed = (argc >= 3 && GET_ARG(3).tt == MRBC_TT_TRUE),
					  .max_files = 8,
					  .allocation_unit_size = 16 * 1024
  };
  esp_err_t ret = esp_vfs_fat_sdspi_mount((char *)GET_STRING_ARG(2), &host, &slot_config, &mount_config, &card);
  if(ret == ESP_OK)
  {
    mount_point = (char *)malloc((mrbc_string_size(&(v[2])) + 1) * sizeof(char));
    memcpy(mount_point, GET_STRING_ARG(2), mrbc_string_size(&(v[2])) + 1);
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
  if(card == NULL && mount_point == NULL) // not initialized.
  {
    SET_FALSE_RETURN();
    return;
  }
  esp_vfs_fat_sdcard_unmount(mount_point, card);
  free(mount_point);
  mount_point = NULL;
  card = NULL;
  SET_TRUE_RETURN();
}

/*! Method spi_bus_free() body : wrapper for spi_bus_free.
*/
static void
mrbc_esp32_sdspi_spi_bus_free(mrb_vm* vm, mrb_value* v, int argc)
{
  spi_bus_free(host.slot);
}

/*! Register SDSPI Class.
 */
void
mrbc_esp32_sdspi_gem_init(struct VM* vm)
{
  //host = SDSPI_HOST_DEFAULT();
  sdmmc_host_t h_copy = SDSPI_HOST_DEFAULT();
  host = h_copy;
  card = NULL;
  mount_point = NULL;

  mrbc_class_esp32_sdspi = mrbc_define_class(vm, "SDSPI", mrbc_class_object);
  mrbc_define_method(vm, mrbc_class_esp32_sdspi, "spi_bus_initialize", mrbc_esp32_sdspi_spi_bus_initialize);
  mrbc_define_method(vm, mrbc_class_esp32_sdspi, "esp_vfs_fat_sdspi_mount", mrbc_esp32_sdspi_esp_vfs_fat_sdspi_mount);
  mrbc_define_method(vm, mrbc_class_esp32_sdspi, "esp_vfs_fat_sdcard_unmount", mrbc_esp32_sdspi_esp_vfs_fat_sdcard_unmount);
  mrbc_define_method(vm, mrbc_class_esp32_sdspi, "spi_bus_free", mrbc_esp32_sdspi_spi_bus_free);

}
