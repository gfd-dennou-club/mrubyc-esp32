/*! @file
  @brief
  mruby/c SPI class for ESP32
*/

#include "mrbc_esp32_spi.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"

static char* TAG = "SPI";

#define DMA_CHAN  SPI3_HOST

int spi_mosi_pin = 23;
int spi_miso_pin = 18;
int spi_clk_pin  = 14;
int spi_freq = SPI_MASTER_FREQ_40M;
int spi_mode = 3;


/*! constructor

  spi = SPI.new( )	
  spi = SPI.new( mosi:23, miso:18, clk:14, cs:27 )

  @param   mosi_pin MOSI Pin Number
           miso MISO Pin Number
           clk SPI Clock Pin Number
           cs   CS Pin Number
*/
static void mrbc_esp32_spi_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  //オプション解析
  MRBC_KW_ARG(frequency, freq, mosi_pin, miso_pin, clk_pin, mode, unit);
  if( MRBC_ISNUMERIC(frequency) ) {
    spi_freq = MRBC_TO_INT(frequency);
  }
  if( MRBC_ISNUMERIC(freq) ) {
    spi_freq = MRBC_TO_INT(freq);
  }
  if( MRBC_ISNUMERIC(mosi_pin) ) {
    spi_mosi_pin = MRBC_TO_INT(mosi_pin);
  }
  if( MRBC_ISNUMERIC(miso_pin) ) {
    spi_miso_pin = MRBC_TO_INT(miso_pin);
  }
  if( MRBC_ISNUMERIC(clk_pin) ) {
    spi_clk_pin = MRBC_TO_INT(clk_pin);
  }
  if( MRBC_ISNUMERIC(mode) ) {
    spi_mode = MRBC_TO_INT(mode);
  }
  if( MRBC_ISNUMERIC(unit) ){
    if ( MRBC_TO_INT(unit) > 1 ) {
      ESP_LOGE(TAG, "unknown SPI unit number detected");
    }
  }

  //インスタンス作成
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(spi_host_device_t));

  //initialize を call
  mrbc_instance_call_initialize( vm, v, argc );
  
  vTaskDelay(100 / portTICK_PERIOD_MS);  //wait
}


/*! initializer

*/
static void mrbc_esp32_spi_initialize(mrbc_vm *vm, mrbc_value v[], int argc)
{
  spi_host_device_t spi_unit = SPI3_HOST;
  spi_bus_config_t bus_cfg = {
    .mosi_io_num = spi_mosi_pin,
    .miso_io_num = spi_miso_pin,  
    .sclk_io_num = spi_clk_pin,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
  };
  
  ESP_ERROR_CHECK(spi_bus_initialize(spi_unit, &bus_cfg, DMA_CHAN));
  
  // instance->data を構造体へのポインタとみなして、値を代入する。
  *((spi_host_device_t *)(v[0].instance->data)) = spi_unit;
  
  ESP_LOGI(TAG, "SPI initial");
  ESP_LOGI(TAG, "unit: %d", spi_unit);
  ESP_LOGI(TAG, "mosi: %d", spi_mosi_pin);
  ESP_LOGI(TAG, "miso: %d", spi_miso_pin);
  ESP_LOGI(TAG, "clk:  %d", spi_clk_pin);
}


/*! Register SPI Class.p
 */
void
mrbc_esp32_spi_gem_init(struct VM* vm)
{
  //クラスUART定義
  mrbc_class *spi = mrbc_define_class(vm, "SPI", mrbc_class_object);
  
  //メソッド定義
  mrbc_define_method(vm, spi, "new",        mrbc_esp32_spi_new);
  mrbc_define_method(vm, spi, "initialize", mrbc_esp32_spi_initialize);
  //  mrbc_define_method(vm, spi, "write",      mrbc_esp32_spi_write);
  //  mrbc_define_method(vm, spi, "read",       mrbc_esp32_spi_read);
}
