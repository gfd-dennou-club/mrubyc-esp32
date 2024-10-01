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

//プロトタイプ宣言
uint8_t * make_output_buffer(mrb_vm *vm, mrb_value v[], int argc,
                             int start_idx, int *ret_bufsiz);

/*! constructor

  spi = SPI.new( )	
  spi = SPI.new( mosi:23, miso:18, clk:14 )

  @param   mosi_pin MOSI Pin Number
           miso MISO Pin Number
           clk SPI Clock Pin Number
*/
static void mrbc_esp32_spi_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
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


/*! Method write_byte(data)
    @param data
*/
static void
mrbc_esp32_spi_write(mrb_vm* vm, mrb_value* v, int argc)
{
  uint8_t *buf = 0;
  int bufsiz = 0;
  
  //第一引数は書き込みデータ
  buf = make_output_buffer( vm, v, argc, 1, &bufsiz );
  if (!buf){
    SET_RETURN( mrbc_integer_value(bufsiz) );
  }

  // start SPI communication
  spi_host_device_t spi_unit = *((spi_host_device_t *)(v[0].instance->data));
  
  spi_device_interface_config_t dev_cfg = {
    .clock_speed_hz = spi_freq,
    //    .spics_io_num = spi_cs,
    .queue_size = 7,
    .flags = SPI_DEVICE_NO_DUMMY,
  };

  //デバイスの追加
  spi_device_handle_t spi_handle;
  ESP_ERROR_CHECK(spi_bus_add_device(spi_unit, &dev_cfg, &spi_handle));

  //データ送信
  spi_transaction_t transaction;
  memset(&transaction, 0, sizeof( spi_transaction_t ));
  transaction.length = bufsiz * 8;
  transaction.tx_buffer = buf;
  ESP_ERROR_CHECK(spi_device_transmit(spi_handle, &transaction));
  
  //デバイス解除
  ESP_ERROR_CHECK(spi_bus_remove_device(spi_handle));

  //動的に確保したメモリの解放
  mrbc_free( vm, buf );
  
  //バッファのサイズを戻す
  SET_RETURN( mrbc_integer_value(bufsiz) );
}


/*! Method read_byte()
    @return recv_data
 */
static void
mrbc_esp32_spi_read(mrb_vm* vm, mrb_value* v, int argc)
{
  uint8_t *buf = 0;
  int bufsiz = 0;
  mrbc_value ret = mrbc_nil_value();
  
  // Get parameter
  if( argc < 1 || v[1].tt != MRBC_TT_INTEGER ){
    ESP_LOGE(TAG, "invalid number and/or type of parameters detected");
  }    
  int read_bytes = mrbc_integer(v[1]);

  if( read_bytes < 0 ) {
    ESP_LOGE(TAG, "invalid number of read_bytes detected");
  }

  if( argc > 1 ) {
    buf = make_output_buffer( vm, v, argc, 2, &bufsiz );
    if( !buf ) {
      SET_RETURN(ret);
    }
  }
 
  ret = mrbc_string_new(vm, 0, read_bytes);
  uint8_t *read_buf = (uint8_t *)mrbc_string_cstr(&ret);
  
  // start SPI communication
  spi_host_device_t spi_unit = *((spi_host_device_t *)(v[0].instance->data));
  
  spi_device_interface_config_t dev_cfg = {
    .clock_speed_hz = spi_freq,
    //    .spics_io_num = spi_cs,
    .queue_size = 7,
    .flags = SPI_DEVICE_NO_DUMMY,
  };

  //デバイスの追加
  spi_device_handle_t spi_handle;
  ESP_ERROR_CHECK(spi_bus_add_device(spi_unit, &dev_cfg, &spi_handle));

  //データ受信
  spi_transaction_t transaction;
  
  if( buf == 0 ) {

    //read する場合
    memset(&transaction, 0, sizeof( spi_transaction_t ));
    transaction.length    = read_bytes * 8;
    transaction.rx_buffer = &read_buf;
    ESP_ERROR_CHECK(spi_device_transmit(spi_handle, &transaction));

  } else {
    
    //write してから read する場合
    memset(&transaction, 0, sizeof( spi_transaction_t ));
    transaction.length    = bufsiz * 8;
    transaction.tx_buffer = buf;
    ESP_ERROR_CHECK(spi_device_transmit(spi_handle, &transaction));

    memset(&transaction, 0, sizeof( spi_transaction_t ));
    transaction.length    = read_bytes;
    transaction.rx_buffer = &read_buf;
    ESP_ERROR_CHECK(spi_device_transmit(spi_handle, &transaction));

  }

  //デバイス解除
  ESP_ERROR_CHECK(spi_bus_remove_device(spi_handle));

  //動的に確保したメモリの解放
  if( buf ) mrbc_free( vm, buf );

  //値を返す
  SET_RETURN(ret);
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
