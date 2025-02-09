/*! @file
  @brief
  mruby/c HTTPClient class for ESP32
  本クラスはインスタンスを生成せず利用する
*/

#include "mrbc_esp32_http_client.h"
#include <sys/param.h>
#include "esp_log.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"

static char* TAG = "HTTP_CLIENT";

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048

char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER + 1] = {0};

/*! HTTP イベントハンドラ
  各種 HTTP イベントが発生した際に呼び出される
  esp-idf/examples/protocols/esp_http_client/main/esp_http_client_example.c より
*/
esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
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
            // Clean the buffer in case of a new request
            if (output_len == 0 && evt->user_data) {
                // we are just starting to copy the output data into the use
                memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
            }
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                int copy_len = 0;
                if (evt->user_data) {
                    // The last byte in evt->user_data is kept for the NULL character in case of out-of-bound access.
                    copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                    if (copy_len) {
                        memcpy(evt->user_data + output_len, evt->data, copy_len);
                    }
                } else {
                    int content_len = esp_http_client_get_content_length(evt->client);
                    if (output_buffer == NULL) {
                        // We initialize output_buffer with 0 because it is used by strlen() and similar functions therefore should be null terminated.
                        output_buffer = (char *) calloc(content_len + 1, sizeof(char));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    copy_len = MIN(evt->data_len, (content_len - output_len));
                    if (copy_len) {
                        memcpy(output_buffer + output_len, evt->data, copy_len);
                    }
                }
                output_len += copy_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGD(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGD(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
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

/*! メソッド get() 本体 

  @param url URL
*/
static void
mrbc_esp32_httpclient_get(mrb_vm* vm, mrb_value* v, int argc)
{
  char* url = (char*)GET_STRING_ARG(1);
  char* username = NULL;
  char* password = NULL;

  //オプション解析
  MRBC_KW_ARG(user, passwd);
  if( MRBC_KW_ISVALID(user) ) {
    username = mrbc_string_cstr(&user);
    ESP_LOGD(TAG, "username :%s\n", username);
  }
  if( MRBC_KW_ISVALID(passwd) ) {
    password = mrbc_string_cstr(&passwd);
    ESP_LOGD(TAG, "password :%s\n", password);
  }
  
  esp_http_client_config_t config = {
    .url = url,
    .event_handler = http_event_handler,
    .user_data = local_response_buffer,   // Pass address of local buffer to get response
    .disable_auto_redirect = true,
    .transport_type = HTTP_TRANSPORT_OVER_SSL, // SSL/TLSを通す
    .crt_bundle_attach = esp_crt_bundle_attach, // 組み込みの証明書バンドル
    .method = HTTP_METHOD_GET,
    .auth_type = HTTP_AUTH_TYPE_NONE,
  };
  esp_http_client_handle_t client = esp_http_client_init(&config);
  
  if (username != NULL && password != NULL ) {
    esp_http_client_set_username(client, username);
    esp_http_client_set_password(client, password);
    esp_http_client_set_authtype(client, HTTP_AUTH_TYPE_BASIC);
  }
    
  // GET
  esp_err_t err = esp_http_client_perform(client);

  if (err == ESP_OK) {
    ESP_LOGD(TAG, "HTTP GET Status = %d, content_length = %"PRId64,
	     esp_http_client_get_status_code(client),
	     esp_http_client_get_content_length(client));
  } else {
    ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
  }

  esp_http_client_cleanup(client);

  mrbc_value ret = mrbc_string_new(vm, local_response_buffer, MAX_HTTP_OUTPUT_BUFFER + 1);
  SET_RETURN( ret );
}


/*! メソッド post() 本体 

  @param url URL
*/
static void
mrbc_esp32_httpclient_post(mrb_vm* vm, mrb_value* v, int argc)
{
  char* url        = (char*)GET_STRING_ARG(1);
  char* post_data  = (char*)GET_STRING_ARG(2);
  char* username   = NULL;
  char* password   = NULL;
  
  //オプション解析
  //オプション解析
  MRBC_KW_ARG(user, passwd);
  if( MRBC_KW_ISVALID(user) ) {
    username = mrbc_string_cstr(&user);
    ESP_LOGD(TAG, "username :%s\n", username);
  }
  if( MRBC_KW_ISVALID(passwd) ) {
    password = mrbc_string_cstr(&passwd);
    ESP_LOGD(TAG, "password :%s\n", password);
  }
    
  esp_http_client_config_t config = {
    .url = url,
    .event_handler = http_event_handler,
    .user_data = local_response_buffer,   // Pass address of local buffer to get response
    .disable_auto_redirect = true,
    .transport_type = HTTP_TRANSPORT_OVER_SSL,  // SSL/TLSを通す
    .crt_bundle_attach = esp_crt_bundle_attach, // 組み込みの証明書バンドル
    .method = HTTP_METHOD_POST,
    .auth_type = HTTP_AUTH_TYPE_NONE
  };
  esp_http_client_handle_t client = esp_http_client_init(&config);

  if (username != NULL && password != NULL ) {
    esp_http_client_set_username(client, username);
    esp_http_client_set_password(client, password);
    esp_http_client_set_authtype(client, HTTP_AUTH_TYPE_BASIC);
  }
  
  // POST
  esp_http_client_set_header(client, "Content-Type", "application/json");
  esp_http_client_set_post_field(client, post_data, strlen(post_data));
  esp_err_t err = esp_http_client_perform(client);
  
  if (err == ESP_OK) {
    ESP_LOGD(TAG, "HTTP POST Status = %d, content_length = %"PRId64,
	     esp_http_client_get_status_code(client),
	     esp_http_client_get_content_length(client));
  } else {
    ESP_LOGD(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
  }

  esp_http_client_cleanup(client);
  
  mrbc_value ret = mrbc_string_new(vm, local_response_buffer, MAX_HTTP_OUTPUT_BUFFER + 1);
  SET_RETURN( ret );
}


/*! クラス定義処理を記述した関数
  この関数を呼ぶことでクラス HTTP が定義される

  @param vm mruby/c VM
*/
void
mrbc_esp32_httpclient_gem_init(struct VM* vm)
{
  //mrbc_define_class でクラス名を定義
  mrbc_class *http = mrbc_define_class(0, "HTTP", 0);

  // 各メソッド定義
  mrbc_define_method(0, http, "get",    mrbc_esp32_httpclient_get);
  mrbc_define_method(0, http, "post",   mrbc_esp32_httpclient_post);
  mrbc_define_method(0, http, "invoke", mrbc_esp32_httpclient_get); //obsolete
  mrbc_define_method(0, http, "access", mrbc_esp32_httpclient_get); //obsolete
}
