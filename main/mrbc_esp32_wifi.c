/*! @file
  @brief
  mruby/c WiFi class for ESP32
  本クラスはインスタンスを生成せず利用する
  init() を最初に呼び出し、setup_psk() もしくは setup_ent_peap() で各種設定の後、start() で接続が開始される
*/

#include "mrbc_esp32_wifi.h"

#include <string.h>
#include <stdlib.h>

#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"

typedef enum {
  DISCONNECTED = 0,
  CONNECTED
} WIFI_CONNECTION_STATUS;


static struct RClass* mrbc_class_esp32_wifi;
static char* tag = "main";
static WIFI_CONNECTION_STATUS connection_status = DISCONNECTED;

/*! WiFi イベントハンドラ
  各種 WiFi イベントが発生した際に呼び出される
*/
static void wifi_event_handler(void* ctx, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
  switch (event_id) {
    // IP Address が取得できた
    case SYSTEM_EVENT_STA_GOT_IP:
      ESP_LOGI(tag, "Got an IP ... ready to go!");
      // この状態のみを CONNECTED とする
      connection_status = CONNECTED;
      tcpip_adapter_ip_info_t ip;
      memset(&ip, 0, sizeof(tcpip_adapter_ip_info_t));
      if (tcpip_adapter_get_ip_info(ESP_IF_WIFI_STA, &ip) == 0) {
        ESP_LOGI(tag, "~~~~~~~~~~~");
        ESP_LOGI(tag, "IP:"IPSTR, IP2STR(&ip.ip));
        ESP_LOGI(tag, "MASK:"IPSTR, IP2STR(&ip.netmask));
        ESP_LOGI(tag, "GW:"IPSTR, IP2STR(&ip.gw));
        ESP_LOGI(tag, "~~~~~~~~~~~");
      }
      break;

    case SYSTEM_EVENT_STA_START:
      ESP_LOGI(tag, "Waiting for IP ..");
      connection_status = DISCONNECTED;
      esp_wifi_connect();
      break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
      ESP_LOGI(tag, "Retrying for IP ..");
      connection_status = DISCONNECTED;
      esp_wifi_connect();
      break;

    default:
      ESP_LOGI(tag, "Trying to get IP ..");
      connection_status = DISCONNECTED;
      esp_wifi_connect();
      break;
  }
}


/*! メソッド init() 本体 : wrapper for esp_wifi_init
  引数なし
*/
static void
mrbc_esp32_wifi_init(mrb_vm* vm, mrb_value* v, int argc)
{

  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();
  
  ESP_LOGI(tag, "WiFi initialization invoked.");
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK( esp_wifi_init(&cfg) );

  ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
  ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
}


/*! メソッド start() 本体 : wrapper for esp_wifi_start
  引数なし
*/
static void
mrbc_esp32_wifi_start(mrb_vm* vm, mrb_value* v, int argc)
{
  ESP_LOGI(tag, "WiFi started.");
  //  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
  ESP_ERROR_CHECK( esp_wifi_start() );
}

/*! メソッド setup_psk(ssid, password) 本体 : wrapper for esp_wifi_set_config
  WPA Personal モードで WiFi をセットアップする

  @param ssid     ssid
  @param password password
*/
static void
mrbc_esp32_wifi_setup_psk(mrb_vm* vm, mrb_value* v, int argc)
{
  char* ssid;
  char* password;

  ssid     = (char*)GET_STRING_ARG(1);
  password = (char*)GET_STRING_ARG(2);

  ESP_LOGI(tag, "WiFi setting up : WPA2 Personal");

  wifi_config_t wifi_config;
  int maxlen;
  memset(&wifi_config, 0, sizeof(wifi_config));

  maxlen = sizeof(wifi_config.sta.ssid) - 1;
  if (strlen(ssid) <= maxlen) {
    strcpy((char*)wifi_config.sta.ssid, ssid);
  }
  else {
    strncpy((char*)wifi_config.sta.ssid, ssid, maxlen);
    wifi_config.sta.ssid[maxlen] = 0;
  }

  maxlen = sizeof(wifi_config.sta.password) - 1;
  if (strlen(password) <= maxlen) {
    strcpy((char*)wifi_config.sta.password, password);
  }
  else {
    strncpy((char*)wifi_config.sta.password, password, maxlen);
    wifi_config.sta.password[maxlen] = 0;
  }

  ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
}


/*! メソッド setup_ent_peap(id, ssid, username, password) 本体 : wrapper for esp_wifi_set_config
  WPA Enterprise (PEAP) モードで WiFi をセットアップする

  @param id       id
  @param ssid     ssid
  @param username username
  @param password password
*/
static void
mrbc_esp32_wifi_setup_ent_peap(mrb_vm* vm, mrb_value* v, int argc)
{
  char* id;
  char* ssid;
  char* username;
  char* password;

  id       = (char*)GET_STRING_ARG(1);
  ssid     = (char*)GET_STRING_ARG(2);
  username = (char*)GET_STRING_ARG(3);
  password = (char*)GET_STRING_ARG(4);

  ESP_LOGI(tag, "WiFi setting up : WPA2 Enterprise PEAP");

  ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_enable() );

  wifi_config_t wifi_config;
  int maxlen;
  memset(&wifi_config, 0, sizeof(wifi_config));

  maxlen = sizeof(wifi_config.sta.ssid) - 1;
  if (strlen(ssid) <= maxlen) {
    strcpy((char*)wifi_config.sta.ssid, ssid);
  }
  else {
    strncpy((char*)wifi_config.sta.ssid, ssid, maxlen);
    wifi_config.sta.ssid[maxlen] = 0;
  }

  ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );

  ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_set_identity((uint8_t*)id,       strlen(id)) );
  ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_set_username((uint8_t*)username, strlen(username)) );
  ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_set_password((uint8_t*)password, strlen(password)) );
}


/*! メソッド is_connected() 本体
  引数なし

  @return true : CONNECTED / false : CONNECTED 以外
*/
static void
mrbc_esp32_wifi_is_connected(mrb_vm* vm, mrb_value* v, int argc)
{
  if (CONNECTED == connection_status) {
    SET_TRUE_RETURN();
  }
  else {
    SET_FALSE_RETURN();
  }
}


/*! クラス定義処理を記述した関数
  この関数を呼ぶことでクラス WiFi が定義される

  @param vm mruby/c VM
*/
void
mrbc_mruby_esp32_wifi_gem_init(struct VM* vm)
{
/*
WiFi.init()
WiFi.setup_psk(ssid: ssid, password: password)
WiFi.setup_ent_peap(id: id, ssid: ssid, username: username, password: password)
WiFi.start()
*/

  // クラス WiFi 定義
  mrbc_class_esp32_wifi = mrbc_define_class(vm, "WiFi", mrbc_class_object);
  // 各メソッド定義
  mrbc_define_method(vm, mrbc_class_esp32_wifi, "init",           mrbc_esp32_wifi_init);
  mrbc_define_method(vm, mrbc_class_esp32_wifi, "start",          mrbc_esp32_wifi_start);
  mrbc_define_method(vm, mrbc_class_esp32_wifi, "setup_psk",      mrbc_esp32_wifi_setup_psk);
  mrbc_define_method(vm, mrbc_class_esp32_wifi, "setup_ent_peap", mrbc_esp32_wifi_setup_ent_peap);
  mrbc_define_method(vm, mrbc_class_esp32_wifi, "is_connected?",  mrbc_esp32_wifi_is_connected);
}
