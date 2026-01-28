#include <stdio.h>
#include "mrbc_esp32_utils.h"
#include "mrubyc.h"
#include <time.h>
#include "esp_rom_sys.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "esp_log.h"

#ifdef CONFIG_USE_MRUBYC_WS2813
#include "led_strip.h"
#endif

#define DELAY_US 5
static const char *TAG = "Utils";


//================================================================
/*! cast
  ["01000101001101001110000100111100"].pack('B*').unpack('g') ができないために用意
*/
static void c_floatCast(struct VM *vm, mrbc_value *v, int argc)
{

  float Value;
  uint32_t val = 0;
  uint32_t arg1 = GET_INT_ARG(1);
  uint32_t arg2 = GET_INT_ARG(2);
  uint32_t arg3 = GET_INT_ARG(3);
  uint32_t arg4 = GET_INT_ARG(4);
  mrbc_value result;
  result = mrbc_array_new(vm, 0);

  val |= arg1;
  val <<= 8;
  val |= arg2;
  val <<= 8;
  val |= arg3;
  val <<= 8;
  val |= arg4;
  memcpy(&Value, &val, sizeof(Value));

  mrbc_array_set(&result, 0, &mrbc_fixnum_value(Value * 100.0));
  SET_RETURN(result);
  //   result = *(float*) &tempU32;
}

static void c_millis(mrb_vm *vm, mrb_value *v, int argc)
{
  struct timespec tp;
  int n;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  n = (int)(tp.tv_sec * 1000ul + tp.tv_nsec / 1000000);
  SET_INT_RETURN(n);
}

//////////////////////////////////////////////////
/// 円形 LED (MY9221)
/// 時間にシビアな部分は C 側で作成
///

static void inline pulse(int clk) {
    gpio_set_level(clk, 0);
    ets_delay_us(DELAY_US);
    gpio_set_level(clk, 1);
    ets_delay_us(DELAY_US);
}

static void inline send_8bit(int data, int clk, uint8_t byte) {
    for (int i = 7; i >= 0; i--) {
        gpio_set_level(data, (byte >> i) & 0x01);
        pulse(clk);
    }
}

static void c_my9221_transmit(mrbc_vm *vm, mrbc_value v[], int argc) {
    int data_pin = GET_INT_ARG(1);
    int clk_pin  = GET_INT_ARG(2);
    mrbc_value array = GET_ARG(3);

    if (array.tt != MRBC_TT_ARRAY) return;

    // 1. 24個のLEDに8bitずつ送る (計192bit)
    // 配列の 23番(奥) から 0番(手前) へ
    for (int i = 23; i >= 0; i--) {
        mrbc_value val_obj = mrbc_array_get(&array, i);
        uint8_t val = (val_obj.tt == MRBC_TT_INTEGER) ? val_obj.i : 0;
        send_8bit(data_pin, clk_pin, val);
    }

    // これで合計200bit。物理的な配線の「最後の1つ」にデータを届ける
    send_8bit(data_pin, clk_pin, 0x00);

    // 3. 確定信号 (Latch)
    gpio_set_level(clk_pin, 0);
    gpio_set_level(data_pin, 0);
    ets_delay_us(50);

    // 8bitモードでの安全なラッチパルス数は 8〜16回。
    // 32回だとコマンドモードと誤認されるため 16回に固定。
    for (int i = 0; i < 16; i++) {
        gpio_set_level(data_pin, 1);
        ets_delay_us(DELAY_US);
        gpio_set_level(data_pin, 0);
        ets_delay_us(DELAY_US);
    }
    gpio_set_level(clk_pin, 1);
}


////////////////////////////////
///  ロードセル用 HX711
///

// 読み取り: hx711_read_raw_c(dat_pin, clk_pin)
static void mrbc_esp32_hx711_read_raw(mrb_vm* vm, mrb_value* v, int argc)
{
    int dat = GET_INT_ARG(1);
    int clk = GET_INT_ARG(2);

    // データが準備できていない (DATがHIGH) 場合は、待たずに nil を返す
    if (gpio_get_level(dat)) {
        SET_NIL_RETURN();
        return;
    }

    int32_t data = 0;
    // 24ビット読み取り (ここからは非常に高速なので WDT は問題ありません)
    for(int i = 0; i < 24; i++){
        gpio_set_level(clk, 1);
        ets_delay_us(1);
        data = (data << 1) | gpio_get_level(dat);
        gpio_set_level(clk, 0);
        ets_delay_us(1);
    }

    // 25番目のパルス
    gpio_set_level(clk, 1);
    ets_delay_us(1);
    gpio_set_level(clk, 0);
    ets_delay_us(1);

    if (data & 0x800000) {
        data |= 0xFF000000;
    }

    SET_INT_RETURN(data);
}

////////////////////////////////////////////
// ビジーウェイト
// <1ms の精密待ちは esp_rom_delay_us を使うことに
// >=1ms の待ちは vTaskDelay()／esp_timer／タイマ割り込みで処理
//
static void mrbc_esp32_sleep_us(mrbc_vm *vm, mrbc_value v[], int argc) {
  int us = (argc >= 1) ? GET_INT_ARG(1) : 0;
  if (us < 0) us = 0;
  esp_rom_delay_us((uint32_t)us);  
}


///////////////////////////////////////////
//
// LED strip (WS2813)
//
#ifdef CONFIG_USE_MRUBYC_WS2813
// 初期化: c_ws2813_init(gpio, count)
static void c_ws2813_init(mrbc_vm *vm, mrbc_value v[], int argc) {
    int gpio = GET_INT_ARG(1);
    int max_leds = GET_INT_ARG(2);

    // LEDストリップの基本設定 (マクロを使用して互換性を確保)
    led_strip_config_t strip_config = {
        .strip_gpio_num = gpio,
        .max_leds = max_leds,
        .led_model = LED_MODEL_WS2812, // WS2813はWS2812として制御可能
        .flags.invert_out = false,
    };

    // RMTバックエンドの設定
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };

    led_strip_handle_t led_strip;
    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "LED Strip initialized (GPIO:%d, LEDs:%d)", gpio, max_leds);
        SET_INT_RETURN((intptr_t)led_strip);
    } else {
        ESP_LOGE(TAG, "Failed to initialize LED strip");
        SET_NIL_RETURN();
    }
}

// 色設定: c_ws2813_set_pixel(handle, index, r, g, b)
static void c_ws2813_set_pixel(mrbc_vm *vm, mrbc_value v[], int argc) {
    led_strip_handle_t handle = (led_strip_handle_t)GET_INT_ARG(1);
    int index = GET_INT_ARG(2);
    int r = GET_INT_ARG(3);
    int g = GET_INT_ARG(4);
    int b = GET_INT_ARG(5);

    if (handle) {
        led_strip_set_pixel(handle, index, r, g, b);
    }
}

// 反映: c_ws2813_show(handle)
static void c_ws2813_show(mrbc_vm *vm, mrbc_value v[], int argc) {
    led_strip_handle_t handle = (led_strip_handle_t)GET_INT_ARG(1);
    if (handle) {
        led_strip_refresh(handle);
    }
}

static void c_ws2813_clear(mrbc_vm *vm, mrbc_value v[], int argc) {
    led_strip_handle_t handle = (led_strip_handle_t)GET_INT_ARG(1);
    led_strip_clear(handle);
}
#endif

void mrbc_esp32_utils_gem_init(struct VM* vm)
{
  mrbc_define_method(0, mrbc_class_object, "floatCast",        c_floatCast);
  mrbc_define_method(0, mrbc_class_object, "millis",           c_millis);
  mrbc_define_method(0, mrbc_class_object, "my9221_transmit",  c_my9221_transmit);  
  mrbc_define_method(0, mrbc_class_object, "hx711_read_raw",   mrbc_esp32_hx711_read_raw);  
  mrbc_define_method(0, mrbc_class_object, "sleep_us",         mrbc_esp32_sleep_us);
#ifdef CONFIG_USE_MRUBYC_WS2813
  mrbc_define_method(0, mrbc_class_object, "ws2813_init",      c_ws2813_init);
  mrbc_define_method(0, mrbc_class_object, "ws2813_set_pixel", c_ws2813_set_pixel);
  mrbc_define_method(0, mrbc_class_object, "ws2813_show",      c_ws2813_show);
  mrbc_define_method(0, mrbc_class_object, "ws2813_clear",     c_ws2813_clear);
#endif  
}

