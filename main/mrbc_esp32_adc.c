/*! @file
  @brief
  mruby/c ADC functions for ESP32
*/

#include "mrbc_esp32_adc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_err.h"
#include "esp_log.h"
#include <inttypes.h>

//ADC Attenuation
#define ADC_ATTEN   ADC_ATTEN_DB_12

//sampling rate
#define NO_OF_SAMPLES   64

int flag_adc1_init = 0;
int flag_adc2_init = 0;

int flag_adc1_cali = 0;
int flag_adc2_cali = 0;

adc_oneshot_unit_handle_t adc1_handle;
adc_oneshot_unit_handle_t adc2_handle;

adc_cali_handle_t adc1_cali;
adc_cali_handle_t adc2_cali;

static const char *TAG = "ADC";

typedef struct ADC_HANDLE {
  int           pin;      //ピン番号
  adc_unit_t    unit;     //ユニット
  adc_channel_t channel;  //チャンネル
} ADC_HANDLE;


/* select_unit
   ピン番号に対応するユニット番号を取得

 */
adc_unit_t select_unit(int pin){

  if (pin == 36 || pin == 39 || pin == 34 || pin == 35 || pin == 32 || pin == 33){
    return ADC_UNIT_1;
  }
  else if (pin == 4 || pin == 0 || pin == 2 || pin == 15 || pin == 13 || pin == 12 || pin == 14 || pin == 27 || pin == 25 || pin == 26){
    return ADC_UNIT_2;
  }
  else {
    ESP_LOGE(TAG, "unknow ADC pin %d", pin);
    return -1;
  }
}

/* select_channel
  ピン番号に対応するチャンネル番号を取得
  
 */
adc_channel_t select_channel(int pin){
  
  if (pin == 36 || pin == 4){
    return ADC_CHANNEL_0;
  }
  else if (pin == 0) {
    return ADC_CHANNEL_1;
  }
  else if (pin == 2){
    return ADC_CHANNEL_2;
  }
  else if (pin == 39 || pin == 15){
    return ADC_CHANNEL_3;
  }
  else if (pin == 32 || pin == 13){
    return ADC_CHANNEL_4;
  }
  else if (pin == 33 || pin == 12){
    return ADC_CHANNEL_5;
  }
  else if (pin == 34 || pin == 14){
    return ADC_CHANNEL_6;
  }
  else if (pin == 35 || pin == 27){
    return ADC_CHANNEL_7;
  }
  else if (pin == 25){
    return ADC_CHANNEL_8;
  }
  else if (pin == 26){
    return ADC_CHANNEL_9;
  }
  else{
    ESP_LOGE(TAG, "unknow ADC pin %d", pin);
    return -1;
  }
}

/*---------------------------------------------------------------
        ADC Calibration  (ESP-IDF のサンプルコードを利用)

--------------------------------------------------------------*/
static bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
  adc_cali_handle_t handle = NULL;
  esp_err_t ret = ESP_FAIL;
  bool calibrated = false;
  
  if (!calibrated) {
    ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
    adc_cali_line_fitting_config_t cali_config = {
      .unit_id = unit,
      .atten = atten,
      .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
    if (ret == ESP_OK) {
      calibrated = true;
    }
  }
  
  *out_handle = handle;
  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "ADC%d Calibration OK", unit + 1);
  } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
    ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
  } else {
    ESP_LOGE(TAG, "Invalid arg or no memory");
  }
  
  return calibrated;
}


/*! constructor

  adc = ADC.new( num )		# num: pin number
*/
static void mrbc_esp32_adc_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  //構造体へ入力
  ADC_HANDLE hndl;
  hndl.pin      = GET_INT_ARG(1);
  hndl.unit     = select_unit( hndl.pin );
  hndl.channel  = select_channel( hndl.pin );

  ESP_LOGI(TAG, "ADC initial");
  ESP_LOGI(TAG, "pin:     %d", hndl.pin);
  ESP_LOGI(TAG, "unit:    %d", hndl.unit);
  ESP_LOGI(TAG, "channel: %d", hndl.channel);
 
  //インスタンス作成
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(ADC_HANDLE));
  
  // instance->data を int へのポインタとみなして、値を代入する。
  *((ADC_HANDLE *)(v[0].instance->data)) = hndl;
  
  //initialize を call
  mrbc_instance_call_initialize( vm, v, argc );

  vTaskDelay(100 / portTICK_PERIOD_MS);  //wait
}


/*! initialize

*/
static void mrbc_esp32_adc_initialize(mrb_vm *vm, mrb_value *v, int argc){

  ADC_HANDLE hndl = *((ADC_HANDLE *)(v[0].instance->data));
  
  if (hndl.unit == ADC_UNIT_1) {

    //初期化は最初の 1 回のみ実施
    if (flag_adc1_init == 0){
      adc_oneshot_unit_init_cfg_t init_config1 = {
	.unit_id = ADC_UNIT_1,
      };
      ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
      flag_adc1_init = 1; //初期化済み
    }
    
    adc_oneshot_chan_cfg_t config = {
      .bitwidth = ADC_BITWIDTH_DEFAULT,
      .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, hndl.channel, &config));

  } else if (hndl.unit == ADC_UNIT_2) {
    
    //初期化は最初の 1 回のみ実施
    if (flag_adc2_init == 0){
      adc_oneshot_unit_init_cfg_t init_config2 = {
	.unit_id = ADC_UNIT_2,
	.ulp_mode = ADC_ULP_MODE_DISABLE,
      };
      ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config2, &adc2_handle));
      flag_adc2_init = 1; //初期化済み
    }

    adc_oneshot_chan_cfg_t config = {
      .bitwidth = ADC_BITWIDTH_DEFAULT,
      .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, hndl.channel, &config));
  }
}

/* multisampling
   1 回の計測では誤差が大きいので NO_OF_SAMPLES 回の計測を行い平均を得る．
   
 */
static int adc_multisampling(adc_unit_t unit, adc_channel_t channel ){

  int adc_raw = 0;
  int adc_reading = 0;
  
  for (int i = 0; i < NO_OF_SAMPLES; i++) {  
    if (unit == ADC_UNIT_1) {
      ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, channel, &adc_raw));
    }
    else if ( unit == ADC_UNIT_2) {
      ESP_ERROR_CHECK(adc_oneshot_read(adc2_handle, channel, &adc_raw));
    }
    adc_reading += adc_raw;
    vTaskDelay(pdMS_TO_TICKS(50));
  }
  return adc_reading /= NO_OF_SAMPLES;
}


/*! read_raw( )
    値を読み込み raw 値 (で夏に変換前の値) を返す
  
*/
static void mrbc_esp32_adc_rawread(mrb_vm *vm, mrb_value *v, int argc){

  ADC_HANDLE hndl = *((ADC_HANDLE *)(v[0].instance->data));

  //生データ取得
  int adc_reading = adc_multisampling( hndl.unit, hndl.channel );
  ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", hndl.unit + 1, hndl.channel, adc_reading);
  
  //値を戻す
  SET_INT_RETURN(adc_reading);
}

/*! read(), read_coltage()
    値を読み込み電圧値 mV を返す
  
*/
static void mrbc_esp32_adc_read(mrb_vm *vm, mrb_value *v, int argc){
  
  ADC_HANDLE hndl = *((ADC_HANDLE *)(v[0].instance->data));
  int adc_voltage = 0;
  bool do_calibration;
  
  //生データ取得
  int adc_reading = adc_multisampling( hndl.unit, hndl.channel );
  ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", hndl.unit + 1, hndl.channel, adc_reading);

  //ユニット毎に処理を分けて実行
  if (hndl.unit == ADC_UNIT_1) {

    //キャリブレーションの実施  → ad1_cali へ入力
    while (flag_adc1_cali == 0) {
      do_calibration = adc_calibration_init(hndl.unit, ADC_ATTEN, &adc1_cali);
      if (do_calibration) {
	flag_adc1_cali = 1;
      }
    }
    //電圧に変換 (adc1_cali を利用)
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali, adc_reading, &adc_voltage));
    ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", hndl.unit + 1, hndl.channel, adc_voltage);

  }
  else if (hndl.unit == ADC_UNIT_2) {

    //キャリブレーションの実施  → adc2_cali へ入力
    while (flag_adc2_cali == 0){
      do_calibration = adc_calibration_init(hndl.unit, ADC_ATTEN, &adc2_cali);
      if (do_calibration) {
	flag_adc2_cali = 1;
      }
    }
    //電圧に変換 (adc2_cali を利用)
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc2_cali, adc_reading, &adc_voltage));
    ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", hndl.unit + 1, hndl.channel, adc_voltage);
  }

  //値を戻す
  SET_INT_RETURN(adc_voltage);
}

/*! クラス定義処理を記述した関数

  @param vm mruby/c VM
*/
void
mrbc_esp32_adc_gem_init(struct VM* vm)
{
  //mrbc_define_class でクラス名を定義
  mrbc_class *adc = mrbc_define_class(0, "ADC", 0);

  //mrbc_define_method でメソッドを定義
  mrbc_define_method(0, adc, "new",            mrbc_esp32_adc_new);
  mrbc_define_method(0, adc, "initialize",     mrbc_esp32_adc_initialize);
  mrbc_define_method(0, adc, "read",           mrbc_esp32_adc_read);
  mrbc_define_method(0, adc, "read_voltage",   mrbc_esp32_adc_read);
  mrbc_define_method(0, adc, "read_raw",       mrbc_esp32_adc_rawread);
}
