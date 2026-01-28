/*! @file
  @brief
  mruby/c HTTPClient class for ESP32
  本クラスはインスタンスを生成せず利用する
*/

#include "mrbc_esp32_http_client.h"
#include "mrbc_esp32_http_camera.h"
#include <sys/param.h>
#include "esp_log.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"

#ifdef CONFIG_USE_MRUBYC_HTTP_CAMERA
#include "esp_camera.h"
#include "esp_psram.h"
#endif

static char* TAG = "HTTP_CAMERA";

/*! メソッド post() 本体 

  @param url URL
*/
static void
mrbc_esp32_httpcamera_capture_post(mrb_vm* vm, mrb_value* v, int argc)
{
  char* url        = (char*)GET_STRING_ARG(1);
  char* post_data  = (char*)GET_STRING_ARG(2);
  char* username   = NULL;
  char* password   = NULL;
 
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

#ifdef CONFIG_USE_MRUBYC_HTTP_CAMERA      
  ESP_LOGI(TAG, "Taking picture...");

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    ESP_LOGE(TAG, "Camera capture failed");
    esp_http_client_cleanup(client);
    SET_NIL_RETURN();
    return;
  }
  
  // デバッグログ: 実際にカメラから届いたデータサイズを確認
  ESP_LOGI(TAG, "Captured! Size: %zu bytes, Format: %d", fb->len, fb->format);
  
  // POST
  esp_http_client_set_header(client, "Content-Type", "image/jpeg");
  esp_http_client_set_post_field(client, (const char *)fb->buf, fb->len);

  //実行
  esp_err_t err = esp_http_client_perform(client);
  
  if (err == ESP_OK) {
    ESP_LOGI(TAG, "HTTP POST Success: %d", esp_http_client_get_status_code(client));
  } else {
    ESP_LOGD(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
  }
  
  // クリーンアップ
  esp_http_client_cleanup(client);

  // カメラドライバにバッファを返す
  esp_camera_fb_return(fb);
  
  //戻り値の生成
  size_t length = strlen(local_response_buffer);
  local_response_buffer[length + 1] = '\0'; // 終端文字を追加
  mrbc_value ret = mrbc_string_new(vm, local_response_buffer, length + 1);
  SET_RETURN( ret );
#endif
}


/*! クラス定義処理を記述した関数
  この関数を呼ぶことでクラス HTTP が定義される

  @param vm mruby/c VM
*/
void
mrbc_esp32_httpcamera_gem_init(struct VM* vm)
{
  //mrbc_define_class でクラス名を定義
  mrbc_class *http = mrbc_define_class(0, "HTTP", 0);

  // 各メソッド定義
  mrbc_define_method(0, http, "capture_and_post", mrbc_esp32_httpcamera_capture_post);

#ifdef CONFIG_USE_MRUBYC_HTTP_CAMERA  
  //カメラ初期化
  ESP_LOGI(TAG, "camera initialize...");
  camera_config_t config;
  memset(&config, 0, sizeof(camera_config_t)); // ゼロクリア
  
  config.pin_pwdn = -1;
  config.pin_reset = -1;
  config.pin_xclk = 4;
  config.pin_sscb_sda = 18;
  config.pin_sscb_scl = 23;
  
  config.pin_d7 = 36;
  config.pin_d6 = 37;
  config.pin_d5 = 38;
  config.pin_d4 = 39;
  config.pin_d3 = 35;
  config.pin_d2 = 14;
  config.pin_d1 = 13;
  config.pin_d0 = 34;
  config.pin_vsync = 5;
  config.pin_href = 27;
  config.pin_pclk = 25;

  config.xclk_freq_hz = 20000000;
  config.ledc_timer = LEDC_TIMER_0;
  config.ledc_channel = LEDC_CHANNEL_0;
  
  config.pixel_format = PIXFORMAT_JPEG;
  //config.frame_size = FRAMESIZE_QVGA;
  config.frame_size = FRAMESIZE_VGA;
 
  config.jpeg_quality = 12;
  config.fb_count = 1;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;
  
  ESP_LOGI("DEBUG", "Initializing Camera: Format=%d, Size=%d", config.pixel_format, config.frame_size);

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Camera Init Failed");
      return;
    }
  //  printf("PSRAM size: %d\n", esp_psram_get_size());
  
  sensor_t * s = esp_camera_sensor_get();
  if (s) {
    s->set_vflip(s, 1); 
    s->set_hmirror(s, 1);
  }
#endif
}
