/*! @file
  @brief
  mruby/c HTTPClient class for ESP32
  本クラスはインスタンスを生成せず利用する
  init() にて URL をセットし、以後 invoke() にて URL にアクセスする
  cleanup() にて利用を終了する
*/

#include "mrbc_esp32_http_client.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"

static char* TAG = "HTTP_CLIENT";

/*! HTTP イベントハンドラ
  各種 HTTP イベントが発生した際に呼び出される
*/
static esp_err_t http_event_handler(esp_http_client_event_t* evt)
{
  switch(evt->event_id) {
    case HTTP_EVENT_ERROR:
      ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
      break;

    case HTTP_EVENT_ON_CONNECTED:
      ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
      break;

    case HTTP_EVENT_HEADER_SENT:
      ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
      break;

    case HTTP_EVENT_ON_HEADER:
      ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
      break;

    case HTTP_EVENT_ON_DATA:
      ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
      if (!esp_http_client_is_chunked_response(evt->client)) {
        printf("%.*s", evt->data_len, (char*)evt->data);
      }
      break;

    case HTTP_EVENT_ON_FINISH:
      ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
      break;

    case HTTP_EVENT_DISCONNECTED:
      ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
      break;

    case HTTP_EVENT_REDIRECT:
      ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
      esp_http_client_set_header(evt->client, "From", "user@example.com");
      esp_http_client_set_header(evt->client, "Accept", "text/html");
      esp_http_client_set_redirection(evt->client);
      break;
  }

  return ESP_OK;
}


/*! メソッド invoke() 本体 

  @param url URL
*/
static void
mrbc_esp32_httpclient_invoke(mrb_vm* vm, mrb_value* v, int argc)
{
  char* url = (char*)GET_STRING_ARG(1);
  
  esp_http_client_config_t config = {
    .url = url,
    .query = "esp",
    .event_handler = http_event_handler,
    //    .user_data = local_response_buffer,        // Pass address of local buffer to get response
    .disable_auto_redirect = true,
    .transport_type = HTTP_TRANSPORT_OVER_SSL, // SSL/TLSを通す
    .crt_bundle_attach = esp_crt_bundle_attach, // 組み込みの証明書バンドル
  };
  esp_http_client_handle_t client = esp_http_client_init(&config);
  
  // GET
  esp_err_t err = esp_http_client_perform(client);
  if (err == ESP_OK) {
    ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %"PRId64,
	     esp_http_client_get_status_code(client),
	     esp_http_client_get_content_length(client));
  } else {
    ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
  }
  
  esp_http_client_cleanup(client);
}


/*! クラス定義処理を記述した関数
  この関数を呼ぶことでクラス HTTPClient が定義される

  @param vm mruby/c VM
*/
void
mrbc_esp32_httpclient_gem_init(struct VM* vm)
{
  //mrbc_define_class でクラス名を定義
  mrbc_class *http = mrbc_define_class(0, "HTTP", 0);

  // 各メソッド定義
  mrbc_define_method(0, http, "invoke", mrbc_esp32_httpclient_invoke);
  mrbc_define_method(0, http, "access", mrbc_esp32_httpclient_invoke);
  mrbc_define_method(0, http, "get",    mrbc_esp32_httpclient_invoke);
}
