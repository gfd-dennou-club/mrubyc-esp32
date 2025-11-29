/*! @file
  @brief
  mruby/c I2C class for ESP32
  initialize メソッドは Ruby コード側で定義する
*/

#include "mrbc_esp32_i2c.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"

// I2Cデバイスを最大8つまでサポート（必要に応じて数を変更）
#define MAX_I2C_DEVICES 8 

//I2C Read 操作で用意されるバッファサイズ
#define MAX_READ_LEN  32

static char* TAG = "I2C";

// I2Cデバイスハンドルとそのアドレスを保持する構造体
typedef struct {
  uint8_t device_addr;
  i2c_master_dev_handle_t dev_handle;
} i2c_device_entry_t;

// I2Cバスハンドルと、複数のデバイスハンドルを保持するための構造体
typedef struct {
  i2c_master_bus_handle_t bus_handle;
  int i2c_freq;
  i2c_device_entry_t devices[MAX_I2C_DEVICES];
  int device_count;
} i2c_handles_t;


/*! @def
  I2C Read 操作で用意されるバッファの長さ。
*/

//デフォルト値の設定


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
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(i2c_handles_t));

  //initialize を call
  mrbc_instance_call_initialize( vm, v, argc );
  
  vTaskDelay(100 / portTICK_PERIOD_MS);  //wait
}

/*! initializer

*/
static void mrbc_esp32_i2c_initialize(mrbc_vm *vm, mrbc_value v[], int argc)
{
  //構造体の定義
  i2c_handles_t handles = {0}; // 構造体全体をゼロクリア
  handles.i2c_freq = 10000;
  handles.device_count = 0;
  int i2c_scl_pin = 22;
  int i2c_sda_pin = 21;  

  //オプション解析. unit は使わない．
  MRBC_KW_ARG(frequency, freq, scl_pin, sda_pin, unit);
  if( MRBC_ISNUMERIC(frequency) ) {
    handles.i2c_freq = MRBC_TO_INT(frequency);
  }
  if( MRBC_ISNUMERIC(freq) ) {
    handles.i2c_freq = MRBC_TO_INT(freq);
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
  
  //バスの設定
  i2c_master_bus_config_t bus_config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port   = I2C_NUM_0,
    .scl_io_num = i2c_scl_pin,
    .sda_io_num = i2c_sda_pin,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true,
  };

  //  i2c_master_bus_handle_t bus_handle;
  ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &handles.bus_handle));
  if (handles.bus_handle != NULL) {
    ESP_LOGI(TAG, "I2C bus initialized.");
  } else {
    ESP_LOGE(TAG, "I2C bus not initialized.");
    return;
  }
  
  // instance->data を構造体へのポインタとみなして、値を代入する。
  *((i2c_handles_t *)(v[0].instance->data)) = handles;

#ifdef CONFIG_USE_MRUBYC_DEBUG
  ESP_LOGI(TAG, "I2C initial");
  ESP_LOGI(TAG, "unit:    %i", I2C_NUM_0);
  ESP_LOGI(TAG, "freq:    %i", i2c_freq);
  ESP_LOGI(TAG, "scl_pin: %i", i2c_scl_pin);
  ESP_LOGI(TAG, "sda_pin: %i", i2c_sda_pin);
#endif
}

// I2Cデバイスハンドルを検索または作成するヘルパー関数
// 成功した場合はハンドルを返し、失敗した場合は NULL を返す。
static i2c_master_dev_handle_t get_or_create_dev_handle(i2c_handles_t *handles, int i2c_addr_7)
{
    // 1. 既存のデバイスハンドルを検索
    for (int i = 0; i < handles->device_count; i++) {
        if (handles->devices[i].device_addr == i2c_addr_7) {
            // 見つかった場合は既存のハンドルを返す
            return handles->devices[i].dev_handle;
        }
    }

    // 2. デバイスハンドルが存在しない場合、新しく作成
    if (handles->device_count >= MAX_I2C_DEVICES) {
        ESP_LOGE(TAG, "Exceeded max I2C device limit (%d).", MAX_I2C_DEVICES);
        return NULL;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = i2c_addr_7,
        .scl_speed_hz = handles->i2c_freq, // 保持していた周波数を適用
    };

    i2c_master_dev_handle_t new_dev_handle;
    
    // デバイスの追加（初回のみ実行）
    ESP_LOGI(TAG, "Creating I2C device handle for address 0x%X", i2c_addr_7);
    
    if (ESP_OK != i2c_master_bus_add_device(handles->bus_handle, &dev_cfg, &new_dev_handle)) {
         ESP_LOGE(TAG, "Failed to add I2C device 0x%X", i2c_addr_7);
         return NULL;
    }

    //デバイスの存在チェック
    ESP_LOGI(TAG, "Probing I2C device 0x%X...", i2c_addr_7);
    
    // タイムアウト値を設定 (例: 50ms)
    const int PROBE_TIMEOUT_MS = 50; 
    
    // i2c_master_probe(バスハンドル, アドレス, タイムアウト) に変更
    esp_err_t probe_ret = i2c_master_probe(
        handles->bus_handle, // 1. I2Cバスハンドルを渡す
        i2c_addr_7,          // 2. デバイスアドレスを渡す
        PROBE_TIMEOUT_MS     // 3. タイムアウトを渡す
    ); 

    if (probe_ret != ESP_OK) {
        // プローブ失敗 (デバイスが存在しない、またはACKが返ってこない)
        ESP_LOGE(TAG, "I2C device 0x%X not found! (Probe failed with code: 0x%X).", i2c_addr_7, probe_ret);
        
        // 作成したデバイスハンドルを即座に解放
        i2c_master_bus_rm_device(new_dev_handle);
        
        return NULL;
    }
    
    // 作成したハンドルを管理リストに追加
    handles->devices[handles->device_count].device_addr = i2c_addr_7;
    handles->devices[handles->device_count].dev_handle = new_dev_handle;
    handles->device_count++;

    return new_dev_handle;
}


static void mrbc_esp32_i2c_write(mrb_vm *vm, mrb_value v[], int argc)
{
  uint8_t *buf = 0;
  int bufsiz = 0;
  i2c_master_dev_handle_t current_dev_handle = NULL;
  
  // 第一引数はアドレス
  if( argc < 1 || v[1].tt != MRBC_TT_INTEGER ) {
    ESP_LOGE(TAG, "invalid number and/or type of parameters detected");
  }
  int i2c_addr_7 = mrbc_integer(v[1]);

  // 第二引数は書き込みデータ
  buf = make_output_buffer( vm, v, argc, 2, &bufsiz );
  if (!buf){
    SET_RETURN( mrbc_integer_value(bufsiz) );
    return;
  }

  // start I2C communication
  i2c_handles_t *handles = (i2c_handles_t *)v[0].instance->data; 

  if (handles == NULL) {
    ESP_LOGE(TAG, "I2C bus not initialized.");
    if (buf) mrbc_free( vm, buf ); 
    return;
  }
  
  current_dev_handle = get_or_create_dev_handle(handles, i2c_addr_7);

  if (current_dev_handle == NULL) {
    ESP_LOGE(TAG, "Failed to get or create device handle.");
    if (buf) mrbc_free( vm, buf ); 
    return;
  }
  
  //データ送信
  ESP_ERROR_CHECK(i2c_master_transmit(current_dev_handle, buf, bufsiz, -1));

  //動的に確保したメモリの解放
  if (buf) mrbc_free( vm, buf ); 

  //バッファのサイズを戻す
  SET_RETURN( mrbc_integer_value(bufsiz) );
}


static void mrbc_esp32_i2c_readfrom(mrb_vm *vm, mrb_value v[], int argc)
{
  mrbc_value result;
  int i2c_addr_7, len;
  uint8_t buf[MAX_READ_LEN];
  i2c_master_dev_handle_t current_dev_handle = NULL;
  
  i2c_addr_7 = GET_INT_ARG(1);
  len  = GET_INT_ARG(2);

  // len が MAX_READ_LEN 以下であることを検査
  if (len <= 0 || len > MAX_READ_LEN) {
    ESP_LOGE(TAG, "Invalid read length %d. Max is %d.", len, MAX_READ_LEN);
    SET_RETURN(mrbc_nil_value()); // または適切なエラー値を返す
    return;
  }

  // start I2C communication
  i2c_handles_t *handles = (i2c_handles_t *)v[0].instance->data;   

  if (handles == NULL) {
    ESP_LOGE(TAG, "I2C bus not initialized.");
    return;
  }

  current_dev_handle = get_or_create_dev_handle(handles, i2c_addr_7);

  if (current_dev_handle == NULL) {
    ESP_LOGE(TAG, "Failed to get or create device handle.");
    return;
  }
  
  //データ受信
  ESP_ERROR_CHECK(i2c_master_receive(current_dev_handle, buf, len, -1));

  // Array インスタンスを生成
  result = mrbc_array_new(vm, len);
  
  // Array インスタンス result に Fixnum インスタンスとして read データをセット
  for ( int x = 0; x < len; ++x ) {
    mrbc_array_set(&result, x, &mrbc_fixnum_value(buf[x]));
  }
  
  // Array インスタンス result を本メソッドの返り値としてセット
  SET_RETURN(result);
}


static void mrbc_esp32_i2c_read(mrb_vm *vm, mrb_value v[], int argc)
{
  uint8_t *buf = 0;
  int bufsiz = 0;
  mrbc_value ret = mrbc_nil_value();
  i2c_master_dev_handle_t current_dev_handle = NULL;
  
  // Get parameter
  if( argc < 2 || v[1].tt != MRBC_TT_INTEGER || v[2].tt != MRBC_TT_INTEGER ){
    ESP_LOGE(TAG, "invalid number and/or type of parameters detected");
  }  
  int i2c_addr_7 = mrbc_integer(v[1]);
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

  // メモリ確保
  ret = mrbc_string_new(vm, 0, read_bytes);
  uint8_t *read_buf = (uint8_t *)mrbc_string_cstr(&ret);

  // start I2C communication
  //  i2c_handles_t handles = *((i2c_handles_t *)(v[0].instance->data));
  i2c_handles_t *handles = (i2c_handles_t *)v[0].instance->data;   

  if (handles == NULL) {
    ESP_LOGE(TAG, "I2C bus not initialized.");
    if (buf) mrbc_free( vm, buf ); 
    return;
  }

  current_dev_handle = get_or_create_dev_handle(handles, i2c_addr_7);

  if (current_dev_handle == NULL) {
    ESP_LOGE(TAG, "Failed to get or create device handle.");
    if (buf) mrbc_free( vm, buf ); 
    return;
  }

  //データ受信
  if( buf == 0 ) {
    //read する場合
    ESP_ERROR_CHECK(i2c_master_receive(current_dev_handle, read_buf, read_bytes, -1));
  } else {
    //write してから read する場合
    ESP_ERROR_CHECK(i2c_master_transmit_receive(current_dev_handle, buf, bufsiz, read_buf, read_bytes, -1));
  }
  
  //動的に確保したメモリの解放
  if( buf ) mrbc_free( vm, buf );
    
  //値を返す
  SET_RETURN(ret);
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
  mrbc_define_method(vm, i2c, "readfrom",       mrbc_esp32_i2c_readfrom);
}
