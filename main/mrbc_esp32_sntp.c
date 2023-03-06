/*! @file
  @brief
  mruby/c SNTP class for ESP32
*/

#include "mrbc_esp32_sntp.h"
#include "esp_sntp.h"
#include "esp_log.h"

static char* tag = "sntp";

time_t now = 0;
struct tm timeinfo = { 0 };

static void
mrbc_esp32_sntp_init(mrb_vm* vm, mrb_value* v, int argc)
{
  // STNP 初期化
  ESP_LOGI(tag, "Initializing SNTP");
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "ntp.nict.jp");
  //  sntp_init();
}

static void
mrbc_esp32_sntp_set(mrb_vm* vm, mrb_value* v, int argc)
{
  mrbc_value result;
  int retry  = 0;
  const int retry_count = 10;
  const int len = 7;    //時刻の要素は 7
  uint8_t buf[len];
 
  // Array インスタンスを生成
  result = mrbc_array_new(vm, len);

  // set time
  sntp_init();
  
  // wait for time to be set
  while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
    ESP_LOGI(tag, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }

  //UNIXTIME の取得
  time(&now);

  //JST に設定
  setenv("TZ", "JST-9", 1);
  tzset();

  //tm 構造体に格納
  localtime_r(&now, &timeinfo);

  //配列 buf に登録
  //  buf[0] = timeinfo.tm_year + 1900;
  buf[0] = timeinfo.tm_year;
  buf[1] = timeinfo.tm_mon + 1;
  buf[2] = timeinfo.tm_mday;
  buf[3] = timeinfo.tm_wday;
  buf[4] = timeinfo.tm_hour;
  buf[5] = timeinfo.tm_min;
  buf[6] = timeinfo.tm_sec;

  // Array インスタンス result に Fixnum インスタンスとして read データをセット
  for ( int x = 0; x < len; ++x ) {
    mrbc_array_set(&result, x, &mrbc_fixnum_value(buf[x]));
  }

  // Array インスタンス result を本メソッドの返り値としてセット
  SET_RETURN( result );
}
/*
static void
mrbc_esp32_sntp_year(mrb_vm* vm, mrb_value* v, int argc)
{
   SET_INT_RETURN( timeinfo.tm_year + 1900);
}
static void
mrbc_esp32_sntp_mon(mrb_vm* vm, mrb_value* v, int argc)
{
   SET_INT_RETURN( timeinfo.tm_mon + 1);
}
static void
mrbc_esp32_sntp_mday(mrb_vm* vm, mrb_value* v, int argc)
{
   SET_INT_RETURN( timeinfo.tm_mday);
}
static void
mrbc_esp32_sntp_wday(mrb_vm* vm, mrb_value* v, int argc)
{
   SET_INT_RETURN( timeinfo.tm_wday);
}
static void
mrbc_esp32_sntp_hour(mrb_vm* vm, mrb_value* v, int argc)
{
   SET_INT_RETURN( timeinfo.tm_hour);
}
static void
mrbc_esp32_sntp_min(mrb_vm* vm, mrb_value* v, int argc)
{
   SET_INT_RETURN( timeinfo.tm_min);
}
static void
mrbc_esp32_sntp_sec(mrb_vm* vm, mrb_value* v, int argc)
{
   SET_INT_RETURN( timeinfo.tm_sec);
}
*/
/*! クラス定義処理を記述した関数

  @param vm mruby/c VM
*/
void
mrbc_esp32_sntp_gem_init(struct VM* vm)
{
  mrbc_define_method(0, mrbc_class_object, "sntp_init",  mrbc_esp32_sntp_init);
  mrbc_define_method(0, mrbc_class_object, "sntp_set",  mrbc_esp32_sntp_set);
  /*
  mrbc_define_method(0, mrbc_class_object, "sntp_year",  mrbc_esp32_sntp_year);
  mrbc_define_method(0, mrbc_class_object, "sntp_mon",   mrbc_esp32_sntp_mon);
  mrbc_define_method(0, mrbc_class_object, "sntp_mday",  mrbc_esp32_sntp_mday);
  mrbc_define_method(0, mrbc_class_object, "sntp_wday",  mrbc_esp32_sntp_wday);
  mrbc_define_method(0, mrbc_class_object, "sntp_hour",  mrbc_esp32_sntp_hour);
  mrbc_define_method(0, mrbc_class_object, "sntp_min",   mrbc_esp32_sntp_min);
  mrbc_define_method(0, mrbc_class_object, "sntp_sec",   mrbc_esp32_sntp_sec);
  */
}
