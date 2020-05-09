/*! @file
  @brief
  mruby/c SNTP class for ESP32
  本クラスはインスタンスを生成せず利用する
*/

#include "mrbc_esp32_sntp.h"

#include "esp_sntp.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"

static struct RClass* mrbc_class_esp32_sntp;
static char* tag = "sntp";

time_t now = 0;
struct tm timeinfo = { 0 };

static void
mrbc_esp32_sntp_init(mrb_vm* vm, mrb_value* v, int argc)
{
  int retry  = 0;
  const int retry_count = 10;
 
  // STNP 初期化
  ESP_LOGI(tag, "Initializing SNTP");
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "ntp.nict.jp");
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
}

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

/*! クラス定義処理を記述した関数
  この関数を呼ぶことでクラス SNTP が定義される

  @param vm mruby/c VM
*/
void
mrbc_mruby_esp32_sntp_gem_init(struct VM* vm)
{
/*
SNTP.init()
SNTP.get_time()
*/
  // クラス SNTP 定義
  mrbc_class_esp32_sntp = mrbc_define_class(vm, "SNTP", mrbc_class_object);

  // 各メソッド定義（mruby/c ではインスタンスメソッドをクラスメソッドとしても呼び出し可能）
  mrbc_define_method(vm, mrbc_class_esp32_sntp, "init",  mrbc_esp32_sntp_init);
  mrbc_define_method(vm, mrbc_class_esp32_sntp, "year",  mrbc_esp32_sntp_year);
  mrbc_define_method(vm, mrbc_class_esp32_sntp, "mon",   mrbc_esp32_sntp_mon);
  mrbc_define_method(vm, mrbc_class_esp32_sntp, "mday",  mrbc_esp32_sntp_mday);
  mrbc_define_method(vm, mrbc_class_esp32_sntp, "wday",  mrbc_esp32_sntp_wday);
  mrbc_define_method(vm, mrbc_class_esp32_sntp, "hour",  mrbc_esp32_sntp_hour);
  mrbc_define_method(vm, mrbc_class_esp32_sntp, "min",   mrbc_esp32_sntp_min);
  mrbc_define_method(vm, mrbc_class_esp32_sntp, "sec",   mrbc_esp32_sntp_sec);
}
