/*! @file
  @brief
  mruby/c HTTPClient class for ESP32
  本クラスはインスタンスを生成せず利用する
  init() にて URL をセットし、以後 invoke() にて URL にアクセスする
  cleanup() にて利用を終了する
*/

#include "mrbc_esp32_http_client.h"

#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_http_client.h"


static struct RClass* mrbc_class_esp32_httpclient;
static char* tag = "main";
static esp_http_client_handle_t client;


/*! HTTP イベントハンドラ
  各種 HTTP イベントが発生した際に呼び出される
*/
static esp_err_t http_event_handler(esp_http_client_event_t* evt)
{
  switch(evt->event_id) {
    case HTTP_EVENT_ERROR:
      ESP_LOGD(tag, "HTTP_EVENT_ERROR");
      break;

    case HTTP_EVENT_ON_CONNECTED:
      ESP_LOGD(tag, "HTTP_EVENT_ON_CONNECTED");
      break;

    case HTTP_EVENT_HEADER_SENT:
      ESP_LOGD(tag, "HTTP_EVENT_HEADER_SENT");
      break;

    case HTTP_EVENT_ON_HEADER:
      ESP_LOGD(tag, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
      break;

    case HTTP_EVENT_ON_DATA:
      ESP_LOGD(tag, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
      if (!esp_http_client_is_chunked_response(evt->client)) {
        printf("%.*s", evt->data_len, (char*)evt->data);
      }
      break;

    case HTTP_EVENT_ON_FINISH:
      ESP_LOGD(tag, "HTTP_EVENT_ON_FINISH");
      break;

    case HTTP_EVENT_DISCONNECTED:
      ESP_LOGD(tag, "HTTP_EVENT_DISCONNECTED");
      break;
  }

  return ESP_OK;
}


/*! メソッド init() 本体 : wrapper for esp_http_client_init

  @param url URL
*/
static void
mrbc_esp32_httpclient_init(mrb_vm* vm, mrb_value* v, int argc)
{
  char* url = (char*)GET_STRING_ARG(1);
  esp_http_client_config_t config = {
    .url = url,
    .event_handler = http_event_handler
  };
  client = esp_http_client_init(&config);
}


/*! メソッド invoke() 本体 : wrapper for esp_http_client_perform
  引数なし
*/
static void
mrbc_esp32_httpclient_invoke(mrb_vm* vm, mrb_value* v, int argc)
{
  esp_err_t err = esp_http_client_perform(client);
  if (err != ESP_OK) {
    ESP_LOGE(tag, "HTTP GET request failed: %s", esp_err_to_name(err));
  }else{
  }
}


/*! メソッド cleanup() 本体 : wrapper for esp_http_client_cleanup
  引数なし
*/
static void
mrbc_esp32_httpclient_cleanup(mrb_vm* vm, mrb_value* v, int argc)
{
  esp_http_client_cleanup(client);
}


/*! クラス定義処理を記述した関数
  この関数を呼ぶことでクラス HTTPClient が定義される

  @param vm mruby/c VM
*/
void
mrbc_esp32_httpclient_gem_init(struct VM* vm)
{
/*
HTTPClient.init("http://foo.bar/)
HTTPClient.invoke()
HTTPClient.cleanup()
*/

  // クラス HTTPClient 定義
  mrbc_class_esp32_httpclient = mrbc_define_class(vm, "HTTPClient", mrbc_class_object);
  // 各メソッド定義
  mrbc_define_method(vm, mrbc_class_esp32_httpclient, "init",    mrbc_esp32_httpclient_init);
  mrbc_define_method(vm, mrbc_class_esp32_httpclient, "invoke",  mrbc_esp32_httpclient_invoke);
  mrbc_define_method(vm, mrbc_class_esp32_httpclient, "cleanup", mrbc_esp32_httpclient_cleanup);
}
