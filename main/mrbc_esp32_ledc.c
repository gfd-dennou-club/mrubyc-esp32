/*! @file
  @brief
  mruby/c LEDC functions for ESP32
*/

#include "mrbc_esp32_ledc.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include <inttypes.h>

static char* TAG = "LEDC";

typedef struct LEDC_HANDLE {
  int pin;                //ピン番号
  ledc_timer_t timer;     //タイマー
  ledc_channel_t channel; //チャンネル
  uint32_t freq_ini;      //周波数(初期)
  uint32_t duty_ini;      //デューティー比(初期)
} LEDC_HANDLE;


/*! constructor

  pwm1 = PWM.new( num )		# num: pin number
  pwm1 = PWM.new( 1, frequency:440, duty:30, timer:1, channel:1 )
*/
static void mrbc_esp32_ledc_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  //構造体へ入力. チェンネルとタイマーのデフォルト値はゼロ．
  LEDC_HANDLE hndl;
  hndl.pin      = GET_INT_ARG(1);
  hndl.timer    = 0;
  hndl.channel  = 0;
  hndl.freq_ini = 440;
  hndl.duty_ini = 0;

  //オプション解析
  MRBC_KW_ARG(frequency, freq, duty, timer, channel);
  if( MRBC_ISNUMERIC(frequency) ) {
    hndl.freq_ini = MRBC_TO_INT(frequency);
  }
  if( MRBC_ISNUMERIC(freq) ) {
    hndl.freq_ini = MRBC_TO_INT(freq);
  }
  if( MRBC_ISNUMERIC(duty) ) {
    hndl.duty_ini = MRBC_TO_INT(duty);
  }
  if( MRBC_ISNUMERIC(timer) ) {
    hndl.timer = MRBC_TO_INT(timer);
  }
  if( MRBC_ISNUMERIC(channel) ) {
    hndl.channel = MRBC_TO_INT(channel);
  }
#ifdef debug_mrubyc_esp32  
  ESP_LOGI(TAG, "PWM initial");
  ESP_LOGI(TAG, "pin:       %d", hndl.pin);
  ESP_LOGI(TAG, "timer:     %d", hndl.timer);
  ESP_LOGI(TAG, "channel:   %d", hndl.channel);
  ESP_LOGI(TAG, "freq(ini): [%"PRIu32"]", hndl.freq_ini);
  ESP_LOGI(TAG, "duty(percent)(ini): [%"PRIu32"]", hndl.duty_ini);
#endif
  
  //インスタンス作成
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(LEDC_HANDLE));

  // instance->data を int へのポインタとみなして、値を代入する。
  *((LEDC_HANDLE *)(v[0].instance->data)) = hndl;
  
  //initialize を call
  mrbc_instance_call_initialize( vm, v, argc );
  
  vTaskDelay(100 / portTICK_PERIOD_MS);  //wait
}

/*! initializer

*/
static void mrbc_esp32_ledc_initialize(mrbc_vm *vm, mrbc_value v[], int argc)
{
  LEDC_HANDLE hndl = *((LEDC_HANDLE *)(v[0].instance->data));
  
  ledc_timer_config_t ledc_timer = {
    .speed_mode      = LEDC_HIGH_SPEED_MODE,
    .duty_resolution = LEDC_TIMER_10_BIT,
    .timer_num       = hndl.timer,
    .freq_hz         = hndl.freq_ini,
    .clk_cfg         = LEDC_AUTO_CLK,
  };
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
  
  ledc_channel_config_t ledc_channel = {
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .intr_type  = LEDC_INTR_DISABLE,
    .channel    = hndl.channel, 
    .timer_sel  = hndl.timer,   
    .gpio_num   = hndl.pin,
    .duty       = (uint32_t)(hndl.duty_ini * 10.24),
    .hpoint     = 0,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

/*! ledc_freq( freq ) 

  @param freq   周波数
*/
static void
mrbc_esp32_ledc_freq(mrb_vm* vm, mrb_value* v, int argc)
{
  uint32_t freq = GET_INT_ARG(1);
  LEDC_HANDLE hndl = *((LEDC_HANDLE *)(v[0].instance->data));

#ifdef debug_mrubyc_esp32  
  ESP_LOGI(TAG, "timer: %d", hndl.timer);
  ESP_LOGI(TAG, "freq:  [%"PRIu32"]", freq);
#endif
  
  // 周波数の設定. 設定に失敗した場合は停止させる．
  //
  ESP_ERROR_CHECK( ledc_set_freq(LEDC_HIGH_SPEED_MODE, hndl.timer, freq) );
  vTaskDelay(100 / portTICK_PERIOD_MS);  //wait
}

/*! ledc_duty( duty )

  @param duty   デューティー比 (パーセント)
*/
static void
mrbc_esp32_ledc_duty(mrb_vm* vm, mrb_value* v, int argc)
{
  uint32_t percent = GET_INT_ARG(1);
  uint32_t hpoint = 0;
  LEDC_HANDLE hndl = *((LEDC_HANDLE *)(v[0].instance->data));

  uint32_t duty = percent * 1024 / 100; //10bit

#ifdef debug_mrubyc_esp32  
  ESP_LOGI(TAG, "channel:       %d", hndl.channel);
  ESP_LOGI(TAG, "duty(percent): [%"PRIu32"]", percent);
  ESP_LOGI(TAG, "duty:          [%"PRIu32"]", percent * 1024 / 100);
#endif
  
  //デューティー比の設定．設定に失敗した場合は停止させる．
  //
  ESP_ERROR_CHECK( ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, hndl.channel, duty, hpoint));
  vTaskDelay(100 / portTICK_PERIOD_MS);  //wait
}


/*! ledc_period_us( us )

  @param us   周期 (マイクロ秒)
*/
static void
mrbc_esp32_ledc_period_us(mrb_vm* vm, mrb_value* v, int argc)
{
  uint32_t time = GET_INT_ARG(1);
  LEDC_HANDLE hndl = *((LEDC_HANDLE *)(v[0].instance->data));

  uint32_t freq = ( 1000000 / time ); //microsec --> Hz

#ifdef debug_mrubyc_esp32
  ESP_LOGI(TAG, "timer: %d", hndl.timer);
  ESP_LOGI(TAG, "time:  [%"PRIu32"]", time);
  ESP_LOGI(TAG, "freq:  [%"PRIu32"]", freq);
#endif
  
  // 周波数の設定. 設定に失敗した場合は停止させる．
  //
  ESP_ERROR_CHECK( ledc_set_freq(LEDC_HIGH_SPEED_MODE, hndl.timer, freq) );
  vTaskDelay(100 / portTICK_PERIOD_MS);  //wait
}


/*! PWM set pulse width by microsecond.

  pwm1.pulse_width_us( 20 )
*/
static void
mrbc_esp32_ledc_pulse_width_us(mrbc_vm *vm, mrbc_value v[], int argc)
{
  uint32_t ontime = GET_INT_ARG(1);
  uint32_t hpoint = 0;
  LEDC_HANDLE hndl = *((LEDC_HANDLE *)(v[0].instance->data));  

  uint32_t freq = ledc_get_freq(LEDC_HIGH_SPEED_MODE, hndl.timer);
  uint32_t duty = ontime * (freq / 1000000.0) * 1024; //10bit

#ifdef debug_mrubyc_esp32  
  ESP_LOGI(TAG, "channel: %d", hndl.channel);
  ESP_LOGI(TAG, "ontime:  [%"PRIu32"]", ontime);
  ESP_LOGI(TAG, "duty:    [%"PRIu32"]", duty);
#endif
  
  //デューティー比の設定．設定に失敗した場合は停止させる．
  //
  ESP_ERROR_CHECK( ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, hndl.channel, duty, hpoint) );
  vTaskDelay(100 / portTICK_PERIOD_MS);  //wait
  
  //  uint32_t duty0 = ledc_get_duty(LEDC_HIGH_SPEED_MODE, hndl.channel);
  //  uint32_t freq0 = ledc_get_freq(LEDC_HIGH_SPEED_MODE, hndl.timer);
}


/*! 公開用関数

  @param vm mruby/c VM
*/
void
mrbc_esp32_ledc_gem_init(struct VM* vm)
{
  //mrbc_define_class でクラス名を定義
  mrbc_class *pwm = mrbc_define_class(0, "PWM", 0);

  //mrbc_define_method でメソッドを定義
  mrbc_define_method(0, pwm, "new",            mrbc_esp32_ledc_new);
  mrbc_define_method(0, pwm, "initialize",     mrbc_esp32_ledc_initialize);
  mrbc_define_method(0, pwm, "frequency",      mrbc_esp32_ledc_freq);
  mrbc_define_method(0, pwm, "freq",           mrbc_esp32_ledc_freq);
  mrbc_define_method(0, pwm, "period_us",      mrbc_esp32_ledc_period_us);
  mrbc_define_method(0, pwm, "duty",           mrbc_esp32_ledc_duty);
  mrbc_define_method(0, pwm, "pulse_width_us", mrbc_esp32_ledc_pulse_width_us);

  //一度初期化すれば良い関数 (インスタンス作成のたびに呼び出さなくて良い)
  ESP_ERROR_CHECK(ledc_fade_func_install(0));
}
