/*! @file
  @brief
  mruby/c SPI class for ESP32
*/

#include "mrbc_esp32_spi.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"

static char* TAG = "SPI";

#define DMA_CHAN    2

typedef struct SPI_HANDLE {
  int mosi;
  int miso;
  int clk;
  int cs;
  spi_device_handle_t spidev;
} SPI_HANDLE;


/*! constructor

  spi = SPI.new( )	
  spi = SPI.new( mosi:23, miso:18, clk:14, cs:27 )

  @param   mosi MOSI Pin Number
           miso MISO Pin Number
           clk SPI Clock Pin Number
           cs   CS Pin Number
*/
static void mrbc_esp32_spi_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  //構造体へ入力. デフォルト値の設定
  SPI_HANDLE hndl;
  hndl.mosi = 23;
  hndl.miso = 18;
  hndl.clk = 14;
  hndl.cs   = 27;
  
  //オプション解析
  MRBC_KW_ARG(mosi, miso, clk, cs);
  if( MRBC_ISNUMERIC(mosi) ) {
    hndl.mosi = MRBC_TO_INT(mosi);
  }
  if( MRBC_ISNUMERIC(miso) ) {
    hndl.miso = MRBC_TO_INT(miso);
  }
  if( MRBC_ISNUMERIC(clk) ) {
    hndl.clk = MRBC_TO_INT(clk);
  }
  if( MRBC_ISNUMERIC(cs) ) {
    hndl.cs = MRBC_TO_INT(cs);
  }
  
  ESP_LOGI(TAG, "SPI initial");
  ESP_LOGI(TAG, "mosi: %d", hndl.mosi);
  ESP_LOGI(TAG, "miso: %d", hndl.miso);
  ESP_LOGI(TAG, "clk:  %d", hndl.clk);
  ESP_LOGI(TAG, "cs:   %d", hndl.cs);

  //インスタンス作成
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(SPI_HANDLE));

  // instance->data を構造体へのポインタとみなして、値を代入する。
  *((SPI_HANDLE *)(v[0].instance->data)) = hndl;
  
  //initialize を call
  mrbc_instance_call_initialize( vm, v, argc );
  
  vTaskDelay(100 / portTICK_PERIOD_MS);  //wait
}

/*! initializer

*/
static void mrbc_esp32_spi_initialize(mrbc_vm *vm, mrbc_value v[], int argc)
{
  SPI_HANDLE hndl = *((SPI_HANDLE *)(v[0].instance->data));
  
  spi_bus_config_t bus_cfg = {
    .mosi_io_num = hndl.mosi,
    .miso_io_num = hndl.miso,  //-1
    .sclk_io_num = hndl.clk,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1
  };
  
  esp_err_t ret = spi_bus_initialize(HSPI_HOST, &bus_cfg, DMA_CHAN);
  assert(ret == ESP_OK);
  
  spi_device_interface_config_t dev_cfg = {
    .clock_speed_hz = SPI_MASTER_FREQ_40M,
    .spics_io_num = hndl.cs,
    .queue_size = 7,
    .flags = SPI_DEVICE_NO_DUMMY,
  };
  ret = spi_bus_add_device(HSPI_HOST, &dev_cfg, &hndl.spidev);
  assert(ret == ESP_OK);
}


/*! Method write_byte(data)
    @param data
 */
static void
mrbc_esp32_spi_write(mrb_vm* vm, mrb_value* v, int argc)
{
  SPI_HANDLE hndl = *((SPI_HANDLE *)(v[0].instance->data));
  
  spi_transaction_t transaction;
  esp_err_t ret;
  assert(GET_ARG(1).tt == MRBC_TT_ARRAY);
  mrbc_value *data = GET_ARG(1).array->data;
  uint8_t Data[1024];
  size_t dataLength = GET_INT_ARG(2);
  for (int i = 0; i < dataLength; i++)
    {
      Data[i] = data[i].i;
    }
  memset(&transaction, 0, sizeof(spi_transaction_t));
  transaction.length = 8 * dataLength;
  transaction.tx_buffer = Data;
  ret = spi_device_transmit(hndl.spidev, &transaction);
  assert(ret == ESP_OK);
}


/*! Method read_byte()
    @return recv_data
 */
static void
mrbc_esp32_spi_read(mrb_vm* vm, mrb_value* v, int argc)
{
  SPI_HANDLE hndl = *((SPI_HANDLE *)(v[0].instance->data));
  
  spi_transaction_t transaction;
  uint8_t recv_data;
  esp_err_t ret;

  memset(&transaction, 0, sizeof(transaction));
  transaction.length = 8;
  transaction.rx_buffer = &recv_data;
  
  ret=spi_device_polling_transmit(hndl.spidev, &transaction);
  assert(ret==ESP_OK);
  SET_INT_RETURN(recv_data);
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
  mrbc_define_method(vm, spi, "write",      mrbc_esp32_spi_write);
  mrbc_define_method(vm, spi, "read",       mrbc_esp32_spi_read);
}
