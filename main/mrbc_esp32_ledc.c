/*! @file
  @brief
  mruby/c LEDC functions for ESP32
*/

#include "mrbc_esp32_ledc.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include <inttypes.h>

#define DutyMAX  1024  //duty の最大値 (10 bit)

static char* TAG = "LEDC";

typedef struct LEDC_HANDLE {
  int            pin;      //ピン番号
  ledc_timer_t   timer;    //タイマー
  ledc_channel_t channel;  //チャンネル
  uint8_t        flag;     //初期化フラグ
} LEDC_HANDLE;

ledc_channel_t nowSelectedChannels[4][8]= {0};  //Timer 0-3, channel 0-7


/*! constructor

  pwm1 = PWM.new( num )		# num: pin number
  pwm1 = PWM.new( 1, frequency:440, duty:30, timer:1, channel:1 )
*/
static void mrbc_esp32_ledc_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  //インスタンス作成
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(LEDC_HANDLE));
  
  //initialize を call
  mrbc_instance_call_initialize( vm, v, argc );
  
  vTaskDelay(100 / portTICK_PERIOD_MS);  //wait
}

/*! initializer

*/
static void mrbc_esp32_ledc_initialize(mrbc_vm *vm, mrbc_value v[], int argc)
{
  //  int pin = GET_INT_ARG(1);
  uint32_t freq_ini    = 440;   //周波数(初期)
  uint32_t duty_pc_ini = 0;     //デューティー比(初期)
  
  //構造体へ入力. チェンネルとタイマーのデフォルト値はゼロ．
  LEDC_HANDLE hndl;
  hndl.timer    = 0;
  hndl.channel  = 0; 
  hndl.flag     = 0;
  hndl.pin      = GET_INT_ARG(1);
  
  //オプション解析
  MRBC_KW_ARG(frequency, freq, duty, timer, channel);
  if( MRBC_ISNUMERIC(frequency) ) {
    freq_ini = MRBC_TO_INT(frequency);
  }
  if( MRBC_ISNUMERIC(freq) ) {
    freq_ini = MRBC_TO_INT(freq);
  }
  if( MRBC_ISNUMERIC(duty) ) {
    duty_pc_ini = MRBC_TO_INT(duty);
  }
  if( MRBC_ISNUMERIC(timer) ) {
    hndl.timer = MRBC_TO_INT(timer);
  }
  if( MRBC_ISNUMERIC(channel) ) {
    hndl.channel = MRBC_TO_INT(channel);
  }
  while ( nowSelectedChannels[hndl.timer][hndl.channel] == 1){
    int preChannel =  hndl.channel;
    hndl.channel = ( hndl.channel + 1 ) % 8;
    ESP_LOGW(TAG, "LEDC Channel %d (Timer %d) is duplicated. change to %d", preChannel, hndl.timer, hndl.channel );
  }
  if ( hndl.channel > 7 ){
    ESP_LOGE(TAG, "LEDC Channel >= 7");
  }
  if ( hndl.timer > 3 ){
    ESP_LOGE(TAG, "LEDC Timer >= 4");
  }
  
  // タイマー設定
  ledc_timer_config_t ledc_timer = {
    .speed_mode      = LEDC_HIGH_SPEED_MODE,
    .duty_resolution = LEDC_TIMER_10_BIT,
    .timer_num       = hndl.timer,
    .freq_hz         = freq_ini,
    .clk_cfg         = LEDC_AUTO_CLK,
  };
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
  
  // チャンネル設定
  ledc_channel_config_t ledc_channel = {
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .intr_type  = LEDC_INTR_DISABLE,
    .channel    = hndl.channel, 
    .timer_sel  = hndl.timer,   
    .gpio_num   = hndl.pin,
    .duty       = (uint32_t)(duty_pc_ini * DutyMAX / 100.0),
    .hpoint     = 0,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
  
  // 使用したチャンネルを保管
  nowSelectedChannels[hndl.timer][hndl.channel] = 1;
  
  //初期化終了
  hndl.flag = 1;

  // instance->data を int へのポインタとみなして、値を代入する。
  *((LEDC_HANDLE *)(v[0].instance->data)) = hndl;
  
  ESP_LOGW(TAG, "PWM initial");
  ESP_LOGW(TAG, "pin:       %d", hndl.pin);
  ESP_LOGW(TAG, "timer:     %d", hndl.timer);
  ESP_LOGW(TAG, "channel:   %d", hndl.channel);
  ESP_LOGW(TAG, "freq(ini): [%"PRIu32"]", freq_ini);
  ESP_LOGW(TAG, "duty(percent)(ini): [%"PRIu32"]", duty_pc_ini);
}

/*! ledc_freq( freq ) 

  @param freq   周波数
*/
static void
mrbc_esp32_ledc_freq(mrb_vm* vm, mrb_value* v, int argc)
{
  uint32_t freq = GET_INT_ARG(1);
  LEDC_HANDLE hndl = *((LEDC_HANDLE *)(v[0].instance->data));

  ESP_LOGD(TAG, "timer: %d", hndl.timer);
  ESP_LOGD(TAG, "freq:  [%"PRIu32"]", freq);

  // 周波数の設定
  ESP_ERROR_CHECK( ledc_set_freq(LEDC_HIGH_SPEED_MODE, hndl.timer, freq) );
  vTaskDelay(100 / portTICK_PERIOD_MS);  //wait
}

/*! ledc_duty( duty )

  @param duty   デューティー比 (パーセント)
*/
static void
mrbc_esp32_ledc_duty(mrb_vm* vm, mrb_value* v, int argc)
{
  uint32_t duty_pc = GET_INT_ARG(1);
  uint32_t hpoint = 0;
  LEDC_HANDLE hndl = *((LEDC_HANDLE *)(v[0].instance->data));

  uint32_t duty = (uint32_t) (duty_pc * DutyMAX / 100.0); //10bit

  ESP_LOGD(TAG, "channel:       %d", hndl.channel);
  ESP_LOGD(TAG, "duty(percent): [%"PRIu32"]", duty_pc);
  ESP_LOGD(TAG, "duty:          [%"PRIu32"]", duty);
  
  //デューティー比の設定
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

  ESP_LOGD(TAG, "timer: %d", hndl.timer);
  ESP_LOGD(TAG, "time:  [%"PRIu32"]", time);
  ESP_LOGD(TAG, "freq:  [%"PRIu32"]", freq);
  
  // 周波数の設定. 
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
  uint32_t duty = (uint32_t) (ontime * (freq / 1000000.0) * DutyMAX); //10bit

  ESP_LOGD(TAG, "channel: %d", hndl.channel);
  ESP_LOGD(TAG, "ontime:  [%"PRIu32"]", ontime);
  ESP_LOGD(TAG, "duty:    [%"PRIu32"]", duty);
  
  //デューティー比の設定．
  ESP_ERROR_CHECK( ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, hndl.channel, duty, hpoint) );
  vTaskDelay(100 / portTICK_PERIOD_MS);  //wait

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
