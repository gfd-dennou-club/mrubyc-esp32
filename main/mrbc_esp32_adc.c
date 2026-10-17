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
  adc_unit_t    unit;     //ユニット
  adc_channel_t channel;  //チャンネル
} ADC_HANDLE;


/*---------------------------------------------------------------
        ADC Calibration  (ESP-IDF のサンプルコードを利用)
--------------------------------------------------------------*/
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
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
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

/*
static void example_adc_calibration_deinit(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}
*/

/*! constructor

  adc = ADC.new( num )		# num: pin number
*/
static void mrbc_esp32_adc_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  //インスタンス作成
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(ADC_HANDLE));
  
  //initialize を call
  mrbc_instance_call_initialize( vm, v, argc );

  vTaskDelay(100 / portTICK_PERIOD_MS);  //wait
}


/*! initialize

*/
static void mrbc_esp32_adc_initialize(mrb_vm *vm, mrb_value *v, int argc){

  int pin          = GET_INT_ARG(1);

  //構造体へ入力
  ADC_HANDLE hndl;
  //  hndl.unit    = select_unit( pin );
  //  hndl.channel = select_channel( pin );
  esp_err_t ret = adc_oneshot_io_to_channel(pin, &hndl.unit, &hndl.channel);
  if (ret == ESP_OK) {
    ESP_LOGD(TAG, "GPIO%d -> ADC Unit %d, Channel %d\n", pin, hndl.unit, hndl.channel);
  } else {
    ESP_LOGE(TAG, "GPIO%d は ADC に対応していません\n", pin);
  }
  
  ESP_LOGD(TAG, "ADC initial");
  ESP_LOGD(TAG, "pin:     %d", pin);
  ESP_LOGD(TAG, "unit:    %d", hndl.unit);
  ESP_LOGD(TAG, "channel: %d", hndl.channel);

  // instance->data を int へのポインタとみなして、値を代入する。
  *((ADC_HANDLE *)(v[0].instance->data)) = hndl;

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
    vTaskDelay(5 / portTICK_PERIOD_MS);  //wait
    //    vTaskDelay(pdMS_TO_TICKS(50));
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
  ESP_LOGD(TAG, "ADC%d Channel[%d] Raw Data: %d", hndl.unit + 1, hndl.channel, adc_reading);

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
  ESP_LOGD(TAG, "ADC%d Channel[%d] Raw Data: %d", hndl.unit + 1, hndl.channel, adc_reading);

  //ユニット毎に処理を分けて実行
  if (hndl.unit == ADC_UNIT_1) {

    //キャリブレーションの実施  → ad1_cali へ入力
    while (flag_adc1_cali == 0) {
      do_calibration = adc_calibration_init(hndl.unit, hndl.channel, ADC_ATTEN, &adc1_cali);
      if (do_calibration) {
	flag_adc1_cali = 1;
      }
    }
    //電圧に変換 (adc1_cali を利用)
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali, adc_reading, &adc_voltage));
    ESP_LOGD(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", hndl.unit + 1, hndl.channel, adc_voltage);
  }
  else if (hndl.unit == ADC_UNIT_2) {

    //キャリブレーションの実施  → adc2_cali へ入力
    while (flag_adc2_cali == 0){
      do_calibration = adc_calibration_init(hndl.unit, hndl.channel, ADC_ATTEN, &adc2_cali);
      if (do_calibration) {
	flag_adc2_cali = 1;
      }
    }
    //電圧に変換 (adc2_cali を利用)
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc2_cali, adc_reading, &adc_voltage));
    ESP_LOGD(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", hndl.unit + 1, hndl.channel, adc_voltage);
  }


  //値を戻す. mV -> V へ変換
  SET_FLOAT_RETURN(adc_voltage * 0.001);
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
