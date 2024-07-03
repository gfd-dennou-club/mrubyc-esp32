/*! @file
  @brief
  mruby/c I2C class for ESP32
  initialize メソッドは Ruby コード側で定義する
*/

#include "mrbc_esp32_i2c.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"

static char* TAG = "I2C";

/*! @def
  I2C Read 操作で用意されるバッファの長さ。
*/
#define MAX_READ_LEN  32

typedef struct I2C_HANDLE {
  i2c_port_t id;   //ID 
  int freq;        //周波数
  int scl_pin;     //SCLピン番号
  int sda_pin;     //SDAピン番号
} I2C_HANDLE;


/*! constructor
  ESP32 は I2C の物理ユニットが 1 つしかないので ID は無視する (scl = 22, sda = 21)

  i2c = I2C.new( )		
  i2c = I2C.new( scl_pin=22, sda_pin=21, frequency:100000 )
*/
static void mrbc_esp32_i2c_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  //構造体へ入力. デフォルト値の設定
  I2C_HANDLE hndl;
  hndl.id      = 0;
  hndl.freq    = 100000;
  hndl.scl_pin = 22;
  hndl.sda_pin = 21;

  //オプション解析
  MRBC_KW_ARG(frequency, freq, scl_pin, sda_pin);
  if( MRBC_ISNUMERIC(frequency) ) {
    hndl.freq = MRBC_TO_INT(frequency);
  }
  if( MRBC_ISNUMERIC(freq) ) {
    hndl.freq = MRBC_TO_INT(freq);
  }
  if( MRBC_ISNUMERIC(scl_pin) ) {
    hndl.scl_pin = MRBC_TO_INT(scl_pin);
  }
  if( MRBC_ISNUMERIC(sda_pin) ) {
    hndl.sda_pin = MRBC_TO_INT(sda_pin);
  }

  ESP_LOGI(TAG, "I2C initial");
  ESP_LOGI(TAG, "id:      %i", hndl.id);
  ESP_LOGI(TAG, "freq:    %i", hndl.freq);
  ESP_LOGI(TAG, "scl_pin: %i", hndl.scl_pin);
  ESP_LOGI(TAG, "sda_pin: %i", hndl.sda_pin);

  //インスタンス作成
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(I2C_HANDLE));

  // instance->data を構造体へのポインタとみなして、値を代入する。
  *((I2C_HANDLE *)(v[0].instance->data)) = hndl;
  
  //initialize を call
  mrbc_instance_call_initialize( vm, v, argc );
  
  vTaskDelay(100 / portTICK_PERIOD_MS);  //wait
}

/*! initializer

*/
static void mrbc_esp32_i2c_initialize(mrbc_vm *vm, mrbc_value v[], int argc)
{
  i2c_mode_t mode = I2C_MODE_MASTER; //master 固定

  I2C_HANDLE hndl = *((I2C_HANDLE *)(v[0].instance->data));
  
  i2c_config_t config = {
    .mode = mode,
    .scl_io_num = hndl.scl_pin,
    .sda_io_num = hndl.sda_pin,
    .scl_pullup_en = true,
    .sda_pullup_en = true,
    .master.clk_speed = hndl.freq,
  };

  ESP_ERROR_CHECK( i2c_param_config(hndl.id, &config) );
  ESP_ERROR_CHECK( i2c_driver_install(hndl.id, mode, 0, 0, 0) );
}

/*! read( i2c_adrs_7, read_bytes, *param ) -> String

  @param i2c_adrs_7  I2C スレーブアドレス
  @param read_bytes  read データ長、MAX_READ_LEN 以下である必要あり
  @return     read   データ、Fixnum を収めた Array 
*/
static void
mrbc_esp32_i2c_read(mrb_vm* vm, mrb_value* v, int argc)
{
  mrbc_value result;
  i2c_port_t port;
  int addr, len;
  uint8_t buf[MAX_READ_LEN];

  I2C_HANDLE hndl = *((I2C_HANDLE *)(v[0].instance->data));

  // port (id) の取得
  port = hndl.id;

  //オプション処理
  addr = GET_INT_ARG(1);
  len  = GET_INT_ARG(2);

  // len が MAX_READ_LEN 以下であることを検査
  assert( len <= MAX_READ_LEN );

  // Array インスタンスを生成
  result = mrbc_array_new(vm, len);

  // I2C コマンドオブジェクト作成
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  ESP_ERROR_CHECK( i2c_master_start(cmd) );
  ESP_ERROR_CHECK( i2c_master_write_byte(cmd, (addr << 1) | 1, I2C_MASTER_ACK) );
  for ( int x = 0; x < len; ++x ) {
    ESP_ERROR_CHECK( i2c_master_read_byte(cmd, buf + x, ((x < len - 1) ? I2C_MASTER_ACK : I2C_MASTER_NACK)) );
  }
  ESP_ERROR_CHECK( i2c_master_stop(cmd) );
  ESP_ERROR_CHECK( i2c_master_cmd_begin(port, cmd, 1 / portTICK_PERIOD_MS));
  i2c_cmd_link_delete(cmd);

  // Array インスタンス result に Fixnum インスタンスとして read データをセット
  for ( int x = 0; x < len; ++x ) {
    mrbc_array_set(&result, x, &mrbc_fixnum_value(buf[x]));
  }
  // Array インスタンス result を本メソッドの返り値としてセット
  SET_RETURN(result);
}


/*! write( i2c_adrs_7 , *outputs ) -> Integer

  @param i2c_adrs_7 I2C スレーブアドレス
  @param outputs write データ、Fixnum を収めた Array インスタンス
*/
static void
mrbc_esp32_i2c_write(mrb_vm* vm, mrb_value* v, int argc)
{
  mrbc_value tmp;
  i2c_port_t port;
  int addr;
  int len;

  I2C_HANDLE hndl = *((I2C_HANDLE *)(v[0].instance->data));
  
  // 変数 port を参照
  port = hndl.id;
  addr = GET_INT_ARG(1);

  // 第２引数 data が Array 型であることを検査し、長さを取得
  assert( v[2].tt == MRBC_TT_ARRAY );
  len = mrbc_array_size(&v[2]);

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  ESP_ERROR_CHECK( i2c_master_start(cmd) );
  ESP_ERROR_CHECK( i2c_master_write_byte(cmd, addr << 1, I2C_MASTER_ACK) );
  for ( int x = 0; x < len; ++x ) {
    // 第２引数 data から x 番目の要素を取得、型が Fixnum であることを検査
    tmp = mrbc_array_get(&v[2], x);
    assert( tmp.tt == MRBC_TT_FIXNUM );
    // 取得した要素を送信コマンドリンクに登録
    ESP_ERROR_CHECK( i2c_master_write_byte(cmd, tmp.i, I2C_MASTER_ACK) );
  }
  ESP_ERROR_CHECK( i2c_master_stop(cmd) );
  ESP_ERROR_CHECK( i2c_master_cmd_begin(port, cmd, 1 / portTICK_PERIOD_MS));
  i2c_cmd_link_delete(cmd);
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
  mrbc_define_method(vm, i2c, "readfrom",   mrbc_esp32_i2c_read);
}
