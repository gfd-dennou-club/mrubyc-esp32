/*! @file
  @brief
  mruby/c WiFi class for ESP32
  本クラスはインスタンスを生成せず利用する
  init() を最初に呼び出し、setup_psk() もしくは setup_ent_peap() で各種設定の後、start() で接続が開始される
*/

#include "mrbc_esp32_wifi.h"

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sys.h"

static struct RClass* mrbc_class_esp32_wifi;
static char* TAG = "WiFi";

typedef enum {
  DISCONNECTED = 0,
  CONNECTED
} WIFI_CONNECTION_STATUS;

#define MAXIMUM_RETRY 5
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t s_wifi_event_group;
static WIFI_CONNECTION_STATUS connection_status = DISCONNECTED;
static bool wifi_started = false;

static esp_event_handler_instance_t instance_any_id;
static esp_event_handler_instance_t instance_got_ip;
static esp_netif_t *sta_netif = NULL;
static int s_retry_num = 0;
static wifi_config_t wifi_config;

/*! WiFi イベントハンドラ
  各種 WiFi イベントが発生した際に呼び出される
*/
static void event_handler(void* ctx, esp_event_base_t event_base, int32_t event_id, void*event_data)
{
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    esp_wifi_connect();
    ESP_LOGI(TAG, "retry to connect to the AP");
    ESP_LOGI(TAG,"connect to the AP fail");
    connection_status = DISCONNECTED;
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    connection_status = CONNECTED;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}


/*! メソッド init() 本体 : wrapper for esp_wifi_init
  引数なし
*/
static void
mrbc_esp32_wifi_init(mrb_vm* vm, mrb_value* v, int argc)
{
  s_wifi_event_group = xEventGroupCreate();
  
  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  sta_netif = esp_netif_create_default_wifi_sta();
  assert(sta_netif);
  
  ESP_LOGI(TAG, "WiFi initialization invoked.");
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      &event_handler,
                                                      NULL,
                                                      &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                      IP_EVENT_STA_GOT_IP,
                                                      &event_handler,
                                                      NULL,
                                                      &instance_got_ip));
  ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
}
/*
static void wpa2_enterprise_example_task(void *pvParameters)
{
    esp_netif_ip_info_t ip;
    memset(&ip, 0, sizeof(esp_netif_ip_info_t));
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    while (1) {
        vTaskDelay(2000 / portTICK_PERIOD_MS);

        if (esp_netif_get_ip_info(sta_netif, &ip) == 0) {
            ESP_LOGI(TAG, "~~~~~~~~~~~");
            ESP_LOGI(TAG, "IP:"IPSTR, IP2STR(&ip.ip));
            ESP_LOGI(TAG, "MASK:"IPSTR, IP2STR(&ip.netmask));
            ESP_LOGI(TAG, "GW:"IPSTR, IP2STR(&ip.gw));
            ESP_LOGI(TAG, "~~~~~~~~~~~");
        }
    }
}
*/
/*! メソッド start() 本体 : wrapper for esp_wifi_start
  引数なし
*/
static void
mrbc_esp32_wifi_start(mrb_vm* vm, mrb_value* v, int argc)
{
  
  ESP_LOGI(TAG, "WiFi started.");
  ESP_ERROR_CHECK( esp_wifi_start() );
  xEventGroupWaitBits(s_wifi_event_group,
                      WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                      pdFALSE,
                      pdFALSE,
                      portMAX_DELAY);
  /*EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE,
                                         pdFALSE,
                                         portMAX_DELAY);
  */
  //xTaskCreate(&wpa2_enterprise_example_task, "wpa2_enterprise_example_task", 4096, NULL, 5, NULL);
  /* The event will not be processed after unregister */
  //ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
  //ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
  //vEventGroupDelete(s_wifi_event_group);
  
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

  ESP_LOGI(TAG, "WiFi setting up : WPA2 Personal");

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

  ESP_LOGI(TAG, "WiFi setting up : WPA2 Enterprise PEAP");

  ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_enable() );

  wifi_config_t wifi_config;
  int maxlen;
  memset(&wifi_config, 0, sizeof(wifi_config));

  maxlen = sizeof(wifi_config.sta.ssid) - 1;
  if (strlen(ssid) <= maxlen) {
    strcpy((char*)wifi_config.sta.ssid, ssid);
  } else {
    strncpy((char*)wifi_config.sta.ssid, ssid, maxlen);
    wifi_config.sta.ssid[maxlen] = 0;
  }
  ESP_LOGI(TAG, "ID: %s", id);
  ESP_LOGI(TAG, "SSIDID: %s", ssid);
  ESP_LOGI(TAG, "USERNAME: %s", username);
  ESP_LOGI(TAG, "Password: %s", password);
  ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );

  ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_set_identity((uint8_t*)id,       strlen(id)) );
  ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_set_username((uint8_t*)username, strlen(username)) );
  ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_set_password((uint8_t*)password, strlen(password)) );
  ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_enable() );
}

/*! メソッド is_connected() 本体
  引数なし
  @return true : CONNECTED / false : CONNECTED 以外
*/
static void
mrbc_esp32_wifi_is_connected(mrb_vm* vm, mrb_value* v, int argc)
{
  if(connection_status == CONNECTED) {
    SET_TRUE_RETURN();
  }else{
    SET_FALSE_RETURN();
  }
}
/*! メソッド scan() 本体
  引数なし
  @return hash in array : [{ssid: "SSID", bssid: "BSSID", channel: channnel, rssi: "RSSI", authmode: "AUTHMODE", hidden: false} ]

*/

static void
mrbc_esp32_wifi_scan(mrb_vm* vm, mrb_value* v, int argc)
{
  wifi_mode_t mode;
  ESP_ERROR_CHECK(esp_wifi_get_mode(&mode));
  if((mode & WIFI_MODE_STA) == 0){
    ESP_LOGD(TAG, "STA is connecting. scan are not allowd");
    SET_FALSE_RETURN();
  }
  wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&config));
  
  uint16_t scan_size = 10;
  uint16_t number = scan_size;
  wifi_ap_record_t ap_info[scan_size];
  uint16_t ap_count = 0;

  mrb_value result = mrbc_array_new(vm, 0);
  
  memset(ap_info, 0, sizeof(ap_info));

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
  for(uint16_t i = 0; i < ap_count; i++){
    mrbc_value mrbc_ap_records;
    mrbc_value key;
    mrbc_value value;
    char buf[20];
    mrbc_ap_records = mrbc_hash_new(vm, 0);

    key = mrbc_string_new_cstr(vm, "ssid");
    value = mrbc_string_new_cstr(vm, (char *)ap_info[i].ssid);
    mrbc_hash_set(&mrbc_ap_records, &key, &value);

    key = mrbc_string_new_cstr(vm, "bssid");
    // TODO: macアドレスの生成に使えるので後で関数化するかもしれない
    sprintf(buf,
            "%02X:%02X:%02X:%02X:%02X:%02X",
            ap_info[i].bssid[0],
            ap_info[i].bssid[1],
            ap_info[i].bssid[2],
            ap_info[i].bssid[3],
            ap_info[i].bssid[4],
            ap_info[i].bssid[5]);
    value = mrbc_string_new_cstr(vm, buf);
    mrbc_hash_set(&mrbc_ap_records, &key, &value);

    key = mrbc_string_new_cstr(vm, "channel");
    value = mrbc_fixnum_value(ap_info[i].primary);
    mrbc_hash_set(&mrbc_ap_records, &key, &value);

    key = mrbc_string_new_cstr(vm, "rssi");
    value = mrbc_fixnum_value(ap_info[i].rssi);
    mrbc_hash_set(&mrbc_ap_records, &key, &value);

    // 別関数にした方がいいかもしれない
    char *authmode;
    switch(ap_info[i].authmode){
    case WIFI_AUTH_OPEN:
      authmode = "OPEN";
      break;
    case WIFI_AUTH_WEP:
      authmode = "WEP";
      break;
    case WIFI_AUTH_WPA_PSK:
      authmode = "WPA PSK";
      break;
    case WIFI_AUTH_WPA2_PSK:
      authmode = "WPA2 PSK";
      break;
    case WIFI_AUTH_WPA_WPA2_PSK:
      authmode = "WPA/WPA2 PSK";
      break;
    default:
      authmode = "Unknown";
      break;
    }
    key = mrbc_string_new_cstr(vm, "authmode");
    value = mrbc_string_new_cstr(vm, authmode);
    mrbc_hash_set(&mrbc_ap_records, &key, &value);

    // TODO: micropythonの仕様に合わせているが必ずfalseになるので要らない気がする
    key = mrbc_string_new_cstr(vm, "hidden");
    value = mrbc_false_value();
    mrbc_hash_set(&mrbc_ap_records, &key, &value);
    
    mrbc_array_set(&result, i, &mrbc_ap_records);
  }
  ESP_ERROR_CHECK( esp_wifi_stop() );
  SET_RETURN(result);
}

static char* get_mac_address(int argc){
  char* buf = (char *)malloc(sizeof(char) * 20);
  uint8_t mac[6];
  ESP_ERROR_CHECK(esp_read_mac(mac, argc));
  sprintf(buf,
          "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0],
          mac[1],
          mac[2],
          mac[3],
          mac[4],
          mac[5]
          );
  return buf;
}
/*! メソッド mac() 本体
  @param 'STA' or 'sta' STAモードのMAC ADDRESS取得
        'AP' or 'ap' or 'SOFTAP' or 'softap' APモードのMAC ADDRESS取得
        'BLE' or 'ble' or 'BT' or 'bt' BluetoothのMAC ADDRESS取得
        'ETH' or 'eth' EthernetのMAC ADDRESS取得
*/
static void
mrbc_esp32_wifi_config_mac(mrb_vm* vm, mrb_value* v, int argc)
{
  if(GET_TT_ARG(1) == MRBC_TT_STRING){
    mrb_value value;
    const char *args = (const char *)GET_STRING_ARG(1);
    if(strcmp(args,"STA") == 0 || strcmp(args,"sta") == 0){
      value = mrbc_string_new_cstr(vm, get_mac_address(ESP_MAC_WIFI_STA));
      SET_RETURN(value);
    } else if(strcmp(args, "AP") || strcmp(args, "ap") || strcmp(args, "SOFTAP") || strcmp(args, "softap")) {
      value = mrbc_string_new_cstr(vm, get_mac_address(ESP_MAC_WIFI_SOFTAP));
      SET_RETURN(value);
    } else if(strcmp(args, "BLE") || strcmp(args, "ble") || strcmp(args, "BT") || strcmp(args,"bt")) {
      value = mrbc_string_new_cstr(vm, get_mac_address(ESP_MAC_BT));
      SET_RETURN(value);
    } else if(strcmp(args, "ETH") || strcmp(args, "eth")) {
      value = mrbc_string_new_cstr(vm, get_mac_address(ESP_MAC_ETH));
      SET_RETURN(value);
    } else {
      SET_FALSE_RETURN();
    }
  } else {
    SET_NIL_RETURN();
  }
}
/*! メソッド ip() 本体
  @param 'STA' or 'sta' STAモードのIP ADDRESS取得
         'AP' or 'ap' or 'SOFTAP' or 'softap' APモードのIP ADDRESS取得
*/
static void
mrbc_esp32_wifi_config_ip(mrb_vm* vm, mrb_value* v, int argc)
{
  if(GET_TT_ARG(1) == MRBC_TT_STRING){
    tcpip_adapter_ip_info_t ip;
    mrb_value value;
    const char *args = (const char *)GET_STRING_ARG(1);
    memset(&ip, 0, sizeof(tcpip_adapter_ip_info_t));
     if(strcmp(args, "STA") || strcmp(args, "sta")){
      if(tcpip_adapter_get_ip_info(ESP_IF_WIFI_STA, &ip) == 0){
        tcpip_adapter_ip_info_t ip;
        tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);
        value = mrbc_string_new_cstr(vm,ip4addr_ntoa(&ip.ip));
        SET_RETURN(value);
      }
    } else if(strcmp(args, "AP") || strcmp(args, "ap") || strcmp(args, "SOFTAP") || strcmp(args, "softap")) {
      if(tcpip_adapter_get_ip_info(ESP_IF_WIFI_AP, &ip) == 0){
        tcpip_adapter_ip_info_t ip;
        tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ip);
        value = mrbc_string_new_cstr(vm,ip4addr_ntoa(&ip.ip));
        SET_RETURN(value);
      }    
    } else {
      SET_FALSE_RETURN();
    }
  } else {
    SET_NIL_RETURN();
  }
}
// TODO: どこかに共通処理として設置した方が良いかもしれない  
bool mrbc_trans_cppbool_value(mrbc_vtype tt)
{
  if(tt==MRBC_TT_TRUE){
    return true;
  }
  return false;
}

/*! メソッド active() 本体
  @param true or false 
*/
static void
mrbc_esp32_wifi_active(mrb_vm* vm, mrb_value* v, int argc)
{
  wifi_mode_t mode;
  wifi_mode_t mode_get;
  bool active;

  if(!wifi_started) {
    mode = WIFI_MODE_NULL;
  } else {
    ESP_ERROR_CHECK(esp_wifi_get_mode(&mode));
  }
  if(GET_TT_ARG(1) == MRBC_TT_TRUE || GET_TT_ARG(1) == MRBC_TT_FALSE) {
    ESP_ERROR_CHECK(esp_wifi_get_mode(&mode_get));
    active = mrbc_trans_cppbool_value(GET_TT_ARG(1));
    mode = active ? (mode | mode_get) : (mode & ~mode_get);
    if(mode == WIFI_MODE_NULL){
      if(wifi_started){
        ESP_ERROR_CHECK(esp_wifi_stop());
        wifi_started = false;
        SET_FALSE_RETURN();
      }
    } else {
      ESP_ERROR_CHECK(esp_wifi_set_mode(mode));
      if(!wifi_started) {
        ESP_ERROR_CHECK(esp_wifi_start());
        wifi_started = true;
        SET_TRUE_RETURN();
      }
    }
  }
}

/*! メソッド ifconfig() 本体
  @param 
*/
static void
mrbc_esp32_wifi_ifconfig(mrb_vm* vm, mrb_value* v, int argc)
{
  tcpip_adapter_ip_info_t info;
  esp_netif_dns_info_t dns_info;
  mrbc_value mrbc_ifconfig ;
  mrbc_value key;
  mrbc_value value;
  
  const char *args = (const char *)GET_STRING_ARG(1);

  mrbc_ifconfig = mrbc_hash_new(vm, 0);
  
  if(strcmp(args, "STA") || strcmp(args, "sta")){
    ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &info));
    ESP_ERROR_CHECK(tcpip_adapter_get_dns_info(TCPIP_ADAPTER_IF_STA, 0, &dns_info));
  } else if(strcmp(args, "AP") || strcmp(args, "ap") || strcmp(args, "SOFTAP") || strcmp(args, "softap")) {
    ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &info));
    ESP_ERROR_CHECK(tcpip_adapter_get_dns_info(TCPIP_ADAPTER_IF_AP, 0, &dns_info));
  }
  key = mrbc_string_new_cstr(vm, "ip");
  value = mrbc_string_new_cstr(vm, ip4addr_ntoa(&info.ip));
  mrbc_hash_set(&mrbc_ifconfig, &key, &value);

  key = mrbc_string_new_cstr(vm, "netmask");
  value = mrbc_string_new_cstr(vm, ip4addr_ntoa(&info.netmask));
  mrbc_hash_set(&mrbc_ifconfig, &key, &value);

  key = mrbc_string_new_cstr(vm, "gw");
  value = mrbc_string_new_cstr(vm, ip4addr_ntoa(&info.gw));
  mrbc_hash_set(&mrbc_ifconfig, &key, &value);
  char dns[16];
  uint8_t *dns_ip = (uint8_t *)&dns_info.ip;
  sprintf(dns, "%u.%u.%u.%u", dns_ip[0],dns_ip[1],dns_ip[2],dns_ip[3]);
  key = mrbc_string_new_cstr(vm, "dns");
  value = mrbc_string_new_cstr(vm,dns);
  mrbc_hash_set(&mrbc_ifconfig, &key, &value);

  SET_RETURN(mrbc_ifconfig);
}

/*! クラス定義処理を記述した関数
  この関数を呼ぶことでクラス WiFi が定義される
  @param vm mruby/c VM
*/
void
mrbc_esp32_wifi_gem_init(struct VM* vm)
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
  mrbc_define_method(vm, mrbc_class_esp32_wifi, "scan",  mrbc_esp32_wifi_scan);
  mrbc_define_method(vm, mrbc_class_esp32_wifi, "mac",  mrbc_esp32_wifi_config_mac);
  mrbc_define_method(vm, mrbc_class_esp32_wifi, "ip",  mrbc_esp32_wifi_config_ip);
  mrbc_define_method(vm, mrbc_class_esp32_wifi, "ifconfig",  mrbc_esp32_wifi_ifconfig);
  mrbc_define_method(vm, mrbc_class_esp32_wifi, "active",  mrbc_esp32_wifi_active);

}
