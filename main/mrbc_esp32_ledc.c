/*! @file
  @brief
  mruby/c LEDC functions for ESP32 (ESP-IDF 6.0 Ready)
*/

#include "mrbc_esp32_ledc.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include <inttypes.h>

#define DutyMAX  1024  //duty の最大値 (10 bit)

static char* TAG = "LEDC";

// チャンネルとモードを正しく管理する構造体
typedef struct LEDC_HANDLE {
  int pin;
  int timer;    // タイマー (0-3)
  int mode;     // スピードモード (High or Low)
  int channel;  // チャンネル (0-7)
} LEDC_HANDLE;

// [mode][channel] の使用状況 (最大2モード × 8チャンネル = 最大16ch)
// ※新しいチップ(S3, C3など)の場合は 1モード×8チャンネル のみ利用されます
static uint8_t used_channels[2][8] = {0};

// [mode][timer] の設定済み周波数を保持
static uint32_t configured_timer_freq[2][4] = {0};

/*! constructor
*/
static void mrbc_esp32_ledc_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(LEDC_HANDLE));
  mrbc_instance_call_initialize( vm, v, argc );
  vTaskDelay(100 / portTICK_PERIOD_MS);
}

/*! initializer
*/
static void mrbc_esp32_ledc_initialize(mrbc_vm *vm, mrbc_value v[], int argc)
{
  uint32_t freq_ini    = 440;   
  uint32_t duty_pc_ini = 50;    // デフォルト 50%
  
  int req_timer = -1;
  
  LEDC_HANDLE hndl;
  hndl.pin = GET_INT_ARG(1);
  hndl.timer = -1;
  hndl.mode = -1;
  hndl.channel = -1;
  
  // オプション解析
  MRBC_KW_ARG(frequency, freq, duty, timer);
  if( MRBC_ISNUMERIC(frequency) ) freq_ini = MRBC_TO_INT(frequency);
  if( MRBC_ISNUMERIC(freq) )      freq_ini = MRBC_TO_INT(freq);
  if( MRBC_ISNUMERIC(duty) )      duty_pc_ini = MRBC_TO_INT(duty);
  if( MRBC_ISNUMERIC(timer) )     req_timer = MRBC_TO_INT(timer);

  // --- タイマーの決定 ---
  if (req_timer == -1) {
    hndl.timer = 3; // デフォルトタイマー3
  } else {
    if (req_timer < 0 || req_timer > 3) {
      ESP_LOGE(TAG, "LEDC Timer out of range (0-3)");
      *((LEDC_HANDLE *)(v[0].instance->data)) = hndl; 
      return;
    }
    hndl.timer = req_timer;
  }
  
  // --- モードとチャネルの完全自動マッピング (最大16チャンネル) ---
  int found_mode = -1;
  int found_ch = -1;
  
  // LEDC_SPEED_MODE_MAX はESP32なら2(High/Low)、新チップなら1になります
  for (int m = 0; m < LEDC_SPEED_MODE_MAX; m++) {
    for (int c = 0; c < 8; c++) {
      if (used_channels[m][c] == 0) {
        found_mode = m;
        found_ch = c;
        break;
      }
    }
    if (found_ch != -1) break; // 空きが見つかったら抜ける
  }
  
  if (found_ch == -1) {
    ESP_LOGE(TAG, "No free LEDC channels available! (Max capacity reached)");
    *((LEDC_HANDLE *)(v[0].instance->data)) = hndl; 
    return;
  }
  
  hndl.mode = found_mode;
  hndl.channel = found_ch;
  ESP_LOGI(TAG, "Mapped to Mode %d, Channel %d, Timer %d", hndl.mode, hndl.channel, hndl.timer);
  
  // タイマー設定 (モード別・タイマー別で周波数を管理)
  if (configured_timer_freq[hndl.mode][hndl.timer] != freq_ini) {
    ledc_timer_config_t ledc_timer = {
      .speed_mode      = (ledc_mode_t)hndl.mode,
      .duty_resolution = LEDC_TIMER_10_BIT,
      .timer_num       = (ledc_timer_t)hndl.timer,
      .freq_hz         = freq_ini,
      .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    configured_timer_freq[hndl.mode][hndl.timer] = freq_ini;
  }
  
  // チャンネル設定
  ledc_channel_config_t ledc_channel = {
    .speed_mode = (ledc_mode_t)hndl.mode,
    .intr_type  = LEDC_INTR_DISABLE,
    .channel    = (ledc_channel_t)hndl.channel, 
    .timer_sel  = (ledc_timer_t)hndl.timer,   
    .gpio_num   = hndl.pin,
    .duty       = (uint32_t)(duty_pc_ini * DutyMAX / 100.0),
    .hpoint     = 0,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  // フェード機能はシステム全体で1回だけ初期化
  static bool fade_installed = false;
  if (!fade_installed) {
      esp_err_t err = ledc_fade_func_install(0);
      if (err == ESP_OK || err == ESP_ERR_INVALID_STATE) {
          fade_installed = true;
          ESP_LOGD(TAG, "LEDC fade function installed.");
      }
  }
  
  // 使用済みとしてマーク
  used_channels[hndl.mode][hndl.channel] = 1;
  *((LEDC_HANDLE *)(v[0].instance->data)) = hndl;
}

/*! ledc_freq( freq ) 
*/
static void
mrbc_esp32_ledc_freq(mrb_vm* vm, mrb_value* v, int argc)
{
  LEDC_HANDLE hndl = *((LEDC_HANDLE *)(v[0].instance->data));
  if (hndl.channel < 0) return;

  uint32_t freq = GET_INT_ARG(1);
  if (configured_timer_freq[hndl.mode][hndl.timer] != freq) {
    ESP_ERROR_CHECK( ledc_set_freq((ledc_mode_t)hndl.mode, (ledc_timer_t)hndl.timer, freq) );
    configured_timer_freq[hndl.mode][hndl.timer] = freq;
  }
}

/*! ledc_duty( duty )
*/
static void
mrbc_esp32_ledc_duty(mrb_vm* vm, mrb_value* v, int argc)
{
  LEDC_HANDLE hndl = *((LEDC_HANDLE *)(v[0].instance->data));
  if (hndl.channel < 0) return;

  uint32_t duty_pc = GET_INT_ARG(1);
  uint32_t duty = (uint32_t) (duty_pc * DutyMAX / 100.0);
  
  ESP_ERROR_CHECK( ledc_set_duty_and_update((ledc_mode_t)hndl.mode, (ledc_channel_t)hndl.channel, duty, 0));
}

/*! ledc_period_us( us )
*/
static void
mrbc_esp32_ledc_period_us(mrb_vm* vm, mrb_value* v, int argc)
{
  LEDC_HANDLE hndl = *((LEDC_HANDLE *)(v[0].instance->data));
  if (hndl.channel < 0) return;

  uint32_t time = GET_INT_ARG(1);
  uint32_t freq = ( 1000000 / time );
  
  if (configured_timer_freq[hndl.mode][hndl.timer] != freq) {
    ESP_ERROR_CHECK( ledc_set_freq((ledc_mode_t)hndl.mode, (ledc_timer_t)hndl.timer, freq) );
    configured_timer_freq[hndl.mode][hndl.timer] = freq;
  }
}

/*! PWM set pulse width by microsecond.
*/
static void
mrbc_esp32_ledc_pulse_width_us(mrbc_vm *vm, mrbc_value v[], int argc)
{
  LEDC_HANDLE hndl = *((LEDC_HANDLE *)(v[0].instance->data));  
  if (hndl.channel < 0) return;

  uint32_t ontime = GET_INT_ARG(1);
  uint32_t freq = ledc_get_freq((ledc_mode_t)hndl.mode, (ledc_timer_t)hndl.timer);
  uint32_t duty = (uint32_t) (ontime * (freq / 1000000.0) * DutyMAX);
  
  ESP_ERROR_CHECK( ledc_set_duty_and_update((ledc_mode_t)hndl.mode, (ledc_channel_t)hndl.channel, duty, 0) );
}

/*! 公開用関数
*/
void
mrbc_esp32_ledc_gem_init(struct VM* vm)
{
  mrbc_class *pwm = mrbc_define_class(0, "PWM", 0);
  mrbc_define_method(0, pwm, "new",            mrbc_esp32_ledc_new);
  mrbc_define_method(0, pwm, "initialize",     mrbc_esp32_ledc_initialize);
  mrbc_define_method(0, pwm, "frequency",      mrbc_esp32_ledc_freq);
  mrbc_define_method(0, pwm, "freq",           mrbc_esp32_ledc_freq);
  mrbc_define_method(0, pwm, "period_us",      mrbc_esp32_ledc_period_us);
  mrbc_define_method(0, pwm, "duty",           mrbc_esp32_ledc_duty);
  mrbc_define_method(0, pwm, "pulse_width_us", mrbc_esp32_ledc_pulse_width_us);
}
