/*! @file
  @brief
  mruby/c SPI class for ESP32
*/

#include "mrbc_esp32_spi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"

#define DMA_CHAN    2

static struct RClass* mrbc_class_esp32_spi;
spi_device_handle_t spidev;

/*! Method spi_bus_initialize(mosi, miso, sclk) body : wrapper for spi_bus_initialize.
    @param mosi MOSI Pin Number
           miso MISO Pin Number
           sclk SPI Clock Pin Number
           cs   CS Pin Number
    @return if succeeded, return true, otherwise return false.
 */
static void
mrbc_esp32_spi_bus_initialize(mrb_vm* vm, mrb_value* v, int argc)
{
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = GET_INT_ARG(1),
        .miso_io_num = GET_INT_ARG(2),
        .sclk_io_num = GET_INT_ARG(3),
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000
        };
    esp_err_t ret = spi_bus_initialize(HSPI_HOST, &bus_cfg, DMA_CHAN);
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_MASTER_FREQ_40M,
        .spics_io_num = GET_INT_ARG(4),
        .queue_size = 7,
        .flags = SPI_DEVICE_NO_DUMMY,
    };
    ret = spi_bus_add_device(HSPI_HOST, &devcfg, &spidev);
    if (ret == ESP_OK)
        SET_TRUE_RETURN();
    else
        SET_FALSE_RETURN();
}

/*! Method write_byte(data)
    @param data
 */
static void
mrbc_esp32_spi_write_byte(mrb_vm* vm, mrb_value* v, int argc)
{
    spi_transaction_t transaction;
    esp_err_t ret;
    int data = GET_INT_ARG(1);

    memset(&transaction, 0, sizeof(spi_transaction_t));
    transaction.length = 8;
    transaction.tx_buffer = &data;
    ret = spi_device_transmit(spidev, &transaction);
    assert(ret == ESP_OK);
}

/*! Method read_byte()
    @return recv_data
 */
static void
mrbc_esp32_spi_read_byte(mrb_vm* vm, mrb_value* v, int argc)
{
    spi_transaction_t transaction;
    uint8_t recv_data;
    esp_err_t ret;
    memset(&transaction, 0, sizeof(transaction));
    transaction.length = 8;
    transaction.rx_buffer = &recv_data;
 
    ret=spi_device_polling_transmit(spidev, &transaction);
    assert(ret==ESP_OK);
    SET_INT_RETURN(recv_data);
}

/*! Register SDSPI Class.
 */
void
mrbc_mruby_esp32_spi_gem_init(struct VM* vm)
{
  // クラス I2C 定義
  mrbc_class_esp32_spi = mrbc_define_class(vm, "SPI", mrbc_class_object);
  // 各メソッド定義
  mrbc_define_method(vm, mrbc_class_esp32_spi, "bus_initialize",          mrbc_esp32_spi_bus_initialize);
  mrbc_define_method(vm, mrbc_class_esp32_spi, "write_byte",          mrbc_esp32_spi_write_byte);
  mrbc_define_method(vm, mrbc_class_esp32_spi, "read_byte",           mrbc_esp32_spi_read_byte);
}
