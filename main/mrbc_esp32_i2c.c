/*! @file
  @brief
  mruby/c I2C class for ESP32
  initialize メソッドは Ruby コード側で定義する
*/

#include "mrbc_esp32_i2c.h"

#include "driver/i2c.h"


/*! @def
  I2C Read 操作で用意されるバッファの長さ。
*/
#define MAX_READ_LEN  32


static struct RClass* mrbc_class_esp32_i2c;
static mrbc_sym symid_port;
static mrbc_sym symid_scl;
static mrbc_sym symid_sda;
static mrbc_sym symid_freq;


/*! メソッド msleep(milisec) 本体 : sleep by milisec

  @param milisec ミリ秒単位でのスリープ時間
*/
static void
mrbc_msleep(mrb_vm* vm, mrb_value* v, int argc)
{
  int ms = GET_INT_ARG(1);
  vTaskDelay(ms / portTICK_PERIOD_MS);
}


/*! メソッド driver_install() 本体 : wrapper for i2c_driver_install
  引数なし
  インスタンス変数 port, scl, sda を参照する
*/
static void
mrbc_esp32_i2c_driver_install(mrb_vm* vm, mrb_value* v, int argc)
{
  mrbc_value tmp;
  i2c_mode_t mode = I2C_MODE_MASTER;
  i2c_port_t port;
  int scl;
  int sda;
  int freq;

  // インスタンス変数 port を参照
  tmp = mrbc_instance_getiv(&v[0], symid_port);
  assert( tmp.tt == MRBC_TT_FIXNUM );
  port = tmp.i;

  // インスタンス変数 scl を参照
  tmp = mrbc_instance_getiv(&v[0], symid_scl);
  assert( tmp.tt == MRBC_TT_FIXNUM );
  scl = tmp.i;

  // インスタンス変数 sda を参照
  tmp = mrbc_instance_getiv(&v[0], symid_sda);
  assert( tmp.tt == MRBC_TT_FIXNUM );
  sda = tmp.i;

  // インスタンス変数 freq を参照
  tmp = mrbc_instance_getiv(&v[0], symid_freq);
  assert( tmp.tt == MRBC_TT_FIXNUM );
  freq = tmp.i;

  i2c_config_t config = {
    .mode = mode,
    .scl_io_num = scl,
    .sda_io_num = sda,
    .scl_pullup_en = true,
    .sda_pullup_en = true,
    .master.clk_speed = freq,
  };

  ESP_ERROR_CHECK( i2c_param_config(port, &config) );
  ESP_ERROR_CHECK( i2c_driver_install(port, mode, 0, 0, 0) );
}


/*! メソッド driver_delete() 本体 : wrapper for i2c_driver_delete
  引数なし
  インスタンス変数 port を参照する
*/
static void
mrbc_esp32_i2c_driver_delete(mrb_vm* vm, mrb_value* v, int argc)
{
  mrbc_value tmp;
  i2c_port_t port;

  // インスタンス変数 port を参照
  tmp = mrbc_instance_getiv(&v[0], symid_port);
  assert( tmp.tt == MRBC_TT_FIXNUM );
  port = tmp.i;

  ESP_ERROR_CHECK( i2c_driver_delete(port) );
}


/*! メソッド write(addr, data) 本体 : wrapper for i2c_master_write_byte sequence
  インスタンス変数 port を参照する

  @param addr I2C スレーブアドレス
  @param data write データ、Fixnum を収めた Array インスタンス
*/
static void
mrbc_esp32_i2c_write(mrb_vm* vm, mrb_value* v, int argc)
{
  mrbc_value tmp;
  i2c_port_t port;
  int addr;
  int len;

  // インスタンス変数 port を参照
  tmp = mrbc_instance_getiv(&v[0], symid_port);
  assert( tmp.tt == MRBC_TT_FIXNUM );
  port = tmp.i;

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

  ESP_ERROR_CHECK( i2c_master_cmd_begin(port, cmd, 1 / portTICK_RATE_MS));

  i2c_cmd_link_delete(cmd);
}


/*! メソッド read(addr, len) 本体 : wrapper for i2c_master_read_byte sequence
  インスタンス変数 port を参照する

  @param addr I2C スレーブアドレス
  @param len  read データ長、MAX_READ_LEN 以下である必要あり
  @return     read データ、Fixnum を収めた Array
*/
static void
mrbc_esp32_i2c_read(mrb_vm* vm, mrb_value* v, int argc)
{
  mrbc_value tmp, result;
  i2c_port_t port;
  int addr, len;
  uint8_t buf[MAX_READ_LEN];

  // インスタンス変数 port を参照
  tmp = mrbc_instance_getiv(&v[0], symid_port);
  assert( tmp.tt == MRBC_TT_FIXNUM );
  port = tmp.i;

  addr = GET_INT_ARG(1);
  len  = GET_INT_ARG(2);

  // len が MAX_READ_LEN 以下であることを検査
  assert( len <= MAX_READ_LEN );
  // Array インスタンスを生成
  result = mrbc_array_new(vm, len);


  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  ESP_ERROR_CHECK( i2c_master_start(cmd) );
  ESP_ERROR_CHECK( i2c_master_write_byte(cmd, (addr << 1) | 1, I2C_MASTER_ACK) );
  for ( int x = 0; x < len; ++x ) {
    ESP_ERROR_CHECK( i2c_master_read_byte(cmd, buf + x, ((x < len - 1) ? I2C_MASTER_ACK : I2C_MASTER_NACK)) );
  }
  ESP_ERROR_CHECK( i2c_master_stop(cmd) );

  ESP_ERROR_CHECK( i2c_master_cmd_begin(port, cmd, 1 / portTICK_RATE_MS));

  i2c_cmd_link_delete(cmd);


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
mrbc_mruby_esp32_i2c_gem_init(struct VM* vm)
{
/*
I2C.new(port, scl, sda)
I2C#driver_install
I2C#driver_delete
I2C#write(addr, data)
I2C#read(addr, len)
*/

  // クラス I2C 定義
  mrbc_class_esp32_i2c = mrbc_define_class(vm, "I2C", mrbc_class_object);
  // 各メソッド定義
  mrbc_define_method(vm, mrbc_class_esp32_i2c, "msleep",         mrbc_msleep);
  mrbc_define_method(vm, mrbc_class_esp32_i2c, "driver_install", mrbc_esp32_i2c_driver_install);
  mrbc_define_method(vm, mrbc_class_esp32_i2c, "driver_delete",  mrbc_esp32_i2c_driver_delete);
  mrbc_define_method(vm, mrbc_class_esp32_i2c, "__write",        mrbc_esp32_i2c_write);
  mrbc_define_method(vm, mrbc_class_esp32_i2c, "__read",         mrbc_esp32_i2c_read);

  // インスタンス変数を参照する際に必要になる Symbol インスタンスを予め生成しておく
  symid_port = str_to_symid("port");
  symid_scl  = str_to_symid("scl");
  symid_sda  = str_to_symid("sda");
  symid_freq = str_to_symid("freq");
}
