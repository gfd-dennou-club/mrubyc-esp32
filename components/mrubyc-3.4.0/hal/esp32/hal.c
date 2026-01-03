/*! @file
  @brief
  Hardware abstraction layer
        for ESP32 (mruby/c)
        Updated for ESP-IDF v5.x GPTimer
*/

/***** System headers *******************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "esp_types.h"
#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "driver/gptimer.h"

/***** Local headers ********************************************************/
#include "hal.h"

/***** Local variables ******************************************************/
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
static gptimer_handle_t gptimer = NULL;

/***** Local functions ******************************************************/
#ifndef MRBC_NO_TIMER
/**
 * @brief タイマーアラーム発生時のコールバック関数
 * ISR (Interrupt Service Routine) コンテキストで実行されます
 */
static bool IRAM_ATTR on_timer_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    // mruby/c のチック処理を呼び出し
    mrbc_tick();
    
    // 他のタスクをウェイクアップさせる必要がない場合は false を返す
    return false;
}
#endif

/***** Global functions *****************************************************/
#ifndef MRBC_NO_TIMER

/**
 * @brief 初期化
 */
void hal_mrubyc_init(void)
{
    // 1. タイマー設定 (1MHz = 1 tick 1us)
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, 
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    // 2. アラーム設定 (MRBC_TICK_UNIT * 1000 us)
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = MRBC_TICK_UNIT * 1000, 
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

    // 3. コールバック登録 (ここを修正しました)
    gptimer_event_callbacks_t cbs = {
        .on_alarm = on_timer_alarm_cb,
    };
    // gptimer_register_callbacks ではなく gptimer_register_event_callbacks が正解です
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));

    // 4. タイマー有効化と開始
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    ESP_ERROR_CHECK(gptimer_start(gptimer));
}

/**
 * @brief 割り込み許可
 */
void hal_enable_irq(void)
{
    portEXIT_CRITICAL(&mux);
}

/**
 * @brief 割り込み禁止
 */
void hal_disable_irq(void)
{
    portENTER_CRITICAL(&mux);
}

#endif /* ifndef MRBC_NO_TIMER */

int hal_write(int fd, const void *buf, int nbytes)
{
  // stdout ポインタが指す先（UART0 or UART2）に対して、nbytes 分まとめて書き込む
  size_t ret = fwrite(buf, 1, nbytes, stdout);
  
  // バッファリングを無効化していない場合に備えて、即時出力させる
  fflush(stdout); 
  
  return (int)ret;
}

/**
 * @brief プログラム異常終了
 */
void hal_abort(const char *s)
{
  if( s ) {
    //write(1, s, strlen(s));
    hal_write(1, s, strlen(s));
  }
  abort();
}

