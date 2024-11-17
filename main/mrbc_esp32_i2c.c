/*! @file
  @brief
  mruby/c I2C class for ESP32
  initialize メソッドは Ruby コード側で定義する
*/

#include "mrbc_esp32_i2c.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"

static char* TAG = "I2C";

/*! @def
  I2C Read 操作で用意されるバッファの長さ。
*/
#define MAX_READ_LEN  32

//デフォルト値の設定
int i2c_unit    = 0;
int i2c_freq    = 10000;
int i2c_scl_pin = 22;
int i2c_sda_pin = 21;

//プロトタイプ宣言
uint8_t * make_output_buffer(mrb_vm *vm, mrb_value v[], int argc,
                             int start_idx, int *ret_bufsiz);


/*! constructor
  ESP32 は I2C の物理ユニットが 1 つしかないので ID は無視する (scl = 22, sda = 21)

  i2c = I2C.new( )		
  i2c = I2C.new( scl_pin:22, sda_pin:21, frequency:100000 )
*/
static void mrbc_esp32_i2c_new(mrbc_vm *vm, mrbc_value v[], int argc)
{ 
  //I2C インスタンス作成
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(i2c_master_bus_handle_t));

  //initialize を call
  mrbc_instance_call_initialize( vm, v, argc );
  
  vTaskDelay(100 / portTICK_PERIOD_MS);  //wait
}

/*! initializer

*/
static void mrbc_esp32_i2c_initialize(mrbc_vm *vm, mrbc_value v[], int argc)
{
  //オプション解析. unit は使わない．
  MRBC_KW_ARG(frequency, freq, scl_pin, sda_pin, unit);
  if( MRBC_ISNUMERIC(frequency) ) {
    i2c_freq = MRBC_TO_INT(frequency);
  }
  if( MRBC_ISNUMERIC(freq) ) {
    i2c_freq = MRBC_TO_INT(freq);
  }
  if( MRBC_ISNUMERIC(scl_pin) ) {
    i2c_scl_pin = MRBC_TO_INT(scl_pin);
  }
  if( MRBC_ISNUMERIC(sda_pin) ) {
    i2c_sda_pin = MRBC_TO_INT(sda_pin);
  }
  if( MRBC_ISNUMERIC(unit) ){
    if ( MRBC_TO_INT(unit) > 1 ) {
      ESP_LOGE(TAG, "unknown I2C unit number detected");
    }
  }
  
  i2c_master_bus_config_t i2c_mst_config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port   = I2C_NUM_0,
    .scl_io_num = i2c_scl_pin,
    .sda_io_num = i2c_sda_pin,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true,
  };

  i2c_master_bus_handle_t bus_handle;
  ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));
  
  // instance->data を構造体へのポインタとみなして、値を代入する。
  *((i2c_master_bus_handle_t *)(v[0].instance->data)) = bus_handle;

#ifdef CONFIG_USE_MRUBYC_DEBUG
  ESP_LOGI(TAG, "I2C initial");
  ESP_LOGI(TAG, "unit:    %i", I2C_NUM_0);
  ESP_LOGI(TAG, "freq:    %i", i2c_freq);
  ESP_LOGI(TAG, "scl_pin: %i", i2c_scl_pin);
  ESP_LOGI(TAG, "sda_pin: %i", i2c_sda_pin);
#endif
}

static void mrbc_esp32_i2c_write(mrb_vm *vm, mrb_value v[], int argc)
{
  uint8_t *buf = 0;
  int bufsiz = 0;

  // 第一引数はアドレス
  if( argc < 1 || v[1].tt != MRBC_TT_INTEGER ) {
    ESP_LOGE(TAG, "invalid number and/or type of parameters detected");
  }
  int i2c_addr_7 = mrbc_integer(v[1]);

  // 第二引数は書き込みデータ
  buf = make_output_buffer( vm, v, argc, 2, &bufsiz );
  if (!buf){
    SET_RETURN( mrbc_integer_value(bufsiz) );
  }

  // start I2C communication
  i2c_master_bus_handle_t bus_handle = *((i2c_master_bus_handle_t *)(v[0].instance->data));

  i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = i2c_addr_7,
    .scl_speed_hz = i2c_freq,
  };

  //デバイスの追加
  i2c_master_dev_handle_t dev_handle;
  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

  //データ送信
  ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, buf, bufsiz, -1));

  //デバイス解除
  ESP_ERROR_CHECK(i2c_master_bus_rm_device(dev_handle));

  //動的に確保したメモリの解放
  mrbc_free( vm, buf );

  //バッファのサイズを戻す
  SET_RETURN( mrbc_integer_value(bufsiz) );
}


static void mrbc_esp32_i2c_read(mrb_vm *vm, mrb_value v[], int argc)
{
  uint8_t *buf = 0;
  int bufsiz = 0;
  mrbc_value ret = mrbc_nil_value();
  
  // Get parameter
  if( argc < 2 || v[1].tt != MRBC_TT_INTEGER || v[2].tt != MRBC_TT_INTEGER ){
    ESP_LOGE(TAG, "invalid number and/or type of parameters detected");
  }  
  int i2c_adrs_7 = mrbc_integer(v[1]);
  int read_bytes = mrbc_integer(v[2]);
  
  if( read_bytes < 0 ) {
    ESP_LOGE(TAG, "invalid number of read_bytes detected");
  }

  if( argc > 2 ) {
    buf = make_output_buffer( vm, v, argc, 3, &bufsiz );
    if( !buf ) {
      SET_RETURN(ret);
    }
  }

  // Start I2C communication
  ret = mrbc_string_new(vm, 0, read_bytes);
  uint8_t *read_buf = (uint8_t *)mrbc_string_cstr(&ret);

  // start I2C communication
  i2c_master_bus_handle_t bus_handle = *((i2c_master_bus_handle_t *)(v[0].instance->data));
  
  i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = i2c_adrs_7,
    .scl_speed_hz = i2c_freq,
  };

  //デバイスの追加
  i2c_master_dev_handle_t dev_handle;
  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
  
  //データ受信
  if( buf == 0 ) {
    //read する場合
    ESP_ERROR_CHECK(i2c_master_receive(dev_handle, read_buf, read_bytes, -1));
  } else {
    //write してから read する場合
    ESP_ERROR_CHECK(i2c_master_transmit_receive(dev_handle, buf, bufsiz, read_buf, read_bytes, -1));
  }

  //デバイス解除
  ESP_ERROR_CHECK(i2c_master_bus_rm_device(dev_handle));

  //動的に確保したメモリの解放
  if( buf ) mrbc_free( vm, buf );

  //値を返す
  SET_RETURN(ret);
}


static void mrbc_esp32_i2c_readfrom(mrb_vm *vm, mrb_value v[], int argc)
{
  mrbc_value result;
  int addr, len;
  uint8_t buf[MAX_READ_LEN];

  addr = GET_INT_ARG(1);
  len  = GET_INT_ARG(2);

  // len が MAX_READ_LEN 以下であることを検査
  assert( len <= MAX_READ_LEN );

  // start I2C communication
  i2c_master_bus_handle_t bus_handle = *((i2c_master_bus_handle_t *)(v[0].instance->data));
  
  i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = addr,
    .scl_speed_hz = i2c_freq,
  };
  
  i2c_master_dev_handle_t dev_handle;
  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
  
  //データ受信
  ESP_ERROR_CHECK(i2c_master_receive(dev_handle, buf, len, -1));
  
  //デバイス解除
  ESP_ERROR_CHECK(i2c_master_bus_rm_device(dev_handle));

  // Array インスタンスを生成
  result = mrbc_array_new(vm, len);

  // Array インスタンス result に Fixnum インスタンスとして read データをセット
  for ( int x = 0; x < len; ++x ) {
    mrbc_array_set(&result, x, &mrbc_fixnum_value(buf[x]));
  }
  
  // Array インスタンス result を本メソッドの返り値としてセット
  SET_RETURN(result);
}


/*! クラス定義処理を記述した関数
  この関数を呼ぶことでクラス I2C が定義される

  @param vm mruby/c VM
*/
void
mrbc_esp32_i2c_gem_init(struct VM* vm)
{
  //mrbc_define_class でクラス名を定義
  mrbc_class *i2c = mrbc_define_class(0, "I2C", 0);

  // 各メソッド定義
  mrbc_define_method(vm, i2c, "new",        mrbc_esp32_i2c_new);
  mrbc_define_method(vm, i2c, "initialize", mrbc_esp32_i2c_initialize);
  mrbc_define_method(vm, i2c, "write",      mrbc_esp32_i2c_write);
  mrbc_define_method(vm, i2c, "read",       mrbc_esp32_i2c_read);
  mrbc_define_method(vm, i2c, "readfrom",   mrbc_esp32_i2c_readfrom);
}
