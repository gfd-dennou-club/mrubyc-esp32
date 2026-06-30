/*! @file
  @brief
  mruby/c WiFi functions for ESP32
*/

#include "mrbc_esp32_wifi.h"

#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_eap_client.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <string.h>

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

static mrbc_error_code hash_set_address(struct VM*, mrbc_value*, const char*, const esp_ip4_addr_t*);
static char* get_auth_mode_name(const wifi_auth_mode_t);

/*! WiFi イベントハンドラ */
static void event_handler(void* ctx, esp_event_base_t event_base, int32_t event_id, void*event_data)
{
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    wifi_event_sta_disconnected_t* disconnected = (wifi_event_sta_disconnected_t*) event_data;
    ESP_LOGE(TAG, "Disconnect reason: %d", disconnected->reason);

    esp_wifi_connect();
    ESP_LOGI(TAG, "retry to connect to the AP");
    ESP_LOGI(TAG,"connect to the AP fail");
    connection_status = DISCONNECTED;
  } else if (event_base && event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    connection_status = CONNECTED;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

bool mrbc_trans_cppbool_value(mrbc_vtype tt)
{
  if(tt==MRBC_TT_TRUE){
    return true;
  }
  return false;
}

/*! constructor */
static void mrbc_esp32_wifi_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  ESP_LOGI(TAG, "WIFI initial");
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(int));
  *((int *)(v[0].instance->data)) = GET_INT_ARG(1);
  mrbc_instance_call_initialize( vm, v, argc );
  vTaskDelay(100 / portTICK_PERIOD_MS);
}

/*! メソッド initialize 本体 */
static void
mrbc_esp32_wifi_initialize(mrb_vm* vm, mrb_value* v, int argc)
{
  esp_err_t nvs_init_result = nvs_flash_init();
  if (nvs_init_result == ESP_ERR_NVS_NO_FREE_PAGES || nvs_init_result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    nvs_init_result = nvs_flash_init();
  }
  ESP_ERROR_CHECK(nvs_init_result);

  s_wifi_event_group = xEventGroupCreate();
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  sta_netif = esp_netif_create_default_wifi_sta();
  assert(sta_netif);
  
  ESP_LOGI(TAG, "WiFi initialization invoked.");
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));
  ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );

  wifi_mode_t mode;
  wifi_mode_t mode_get;
  bool active;

  if(!wifi_started) {
    mode = WIFI_MODE_NULL;
  } else {
    ESP_ERROR_CHECK(esp_wifi_get_mode(&mode));
  }

  ESP_ERROR_CHECK(esp_wifi_get_mode(&mode_get));
  active = mrbc_trans_cppbool_value(GET_TT_ARG(1));
    
  mode = active ? (mode | mode_get) : (mode & ~mode_get);
  if(mode == WIFI_MODE_NULL){
    if(wifi_started){
      ESP_ERROR_CHECK(esp_wifi_stop());
      wifi_started = false;
    }
  } else {
    ESP_ERROR_CHECK(esp_wifi_set_mode(mode));
    if(!wifi_started) {
      ESP_ERROR_CHECK(esp_wifi_start());
      wifi_started = true;
    }
  }
}

/*! メソッド connect(ssid, password) 本体 (WPA Personal) */
static void
mrbc_esp32_wifi_connect(mrb_vm* vm, mrb_value* v, int argc)
{
  char* ssid;
  char* password;

  ssid     = (char*)GET_STRING_ARG(1);
  password = (char*)GET_STRING_ARG(2);

  ESP_LOGI(TAG, "WiFi setting up : WPA2 Personal");

  esp_wifi_sta_enterprise_disable();

  int maxlen;
  memset(&wifi_config, 0, sizeof(wifi_config));

  wifi_config.sta.bssid_set = false;
  wifi_config.sta.channel = 0;
  wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
  wifi_config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
  wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
  //wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA_PSK;
  wifi_config.sta.pmf_cfg.capable = true;
  wifi_config.sta.pmf_cfg.required = false;

  maxlen = sizeof(wifi_config.sta.ssid) - 1;
  if (strlen(ssid) <= maxlen) {
    strcpy((char*)wifi_config.sta.ssid, ssid);
  } else {
    strncpy((char*)wifi_config.sta.ssid, ssid, maxlen);
    wifi_config.sta.ssid[maxlen] = 0;
  }

  maxlen = sizeof(wifi_config.sta.password) - 1;
  if (strlen(password) <= maxlen) {
    strcpy((char*)wifi_config.sta.password, password);
  } else {
    strncpy((char*)wifi_config.sta.password, password, maxlen);
    wifi_config.sta.password[maxlen] = 0;
  }

  ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );

  ESP_LOGI(TAG, "WiFi started.");
  ESP_ERROR_CHECK( esp_wifi_start() );
  wifi_started = true;

  esp_wifi_set_max_tx_power(80);
  esp_wifi_set_ps(WIFI_PS_NONE);

  xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
}

/*! メソッド connect_peap(ssid, username, password) 本体 (WPA2 Enterprise PEAP) */
static void
mrbc_esp32_wifi_connect_peap(mrb_vm* vm, mrb_value* v, int argc)
{
  // 引数の数と型のチェック
  if (argc < 3) {
    ESP_LOGE(TAG, "Argument Error: connect_peap requires 3 arguments (ssid, username, password)");
    return;
  }
  if (v[1].tt != MRBC_TT_STRING || v[2].tt != MRBC_TT_STRING || v[3].tt != MRBC_TT_STRING) {
    ESP_LOGE(TAG, "Argument Error: All arguments must be String");
    return;
  }

  char* ssid;
  char* username;
  char* password;
  const char* anonymous_id = "anonymous";

  ssid     = (char*)GET_STRING_ARG(1);
  username = (char*)GET_STRING_ARG(2);
  password = (char*)GET_STRING_ARG(3);

  // ポインタがNULLになっていないかの最終確認
  if (ssid == NULL || username == NULL || password == NULL) {
    ESP_LOGE(TAG, "Argument Error: Failed to extract string pointers");
    return;
  }

  ESP_LOGI(TAG, "WiFi setting up : WPA2 Enterprise PEAP");
  ESP_LOGI(TAG, "Target SSID: %s, Username: %s", ssid, username);

  esp_wifi_sta_enterprise_disable();

  wifi_config_t ent_wifi_config;
  int maxlen;
  memset(&ent_wifi_config, 0, sizeof(ent_wifi_config));

  ent_wifi_config.sta.bssid_set = false;
  ent_wifi_config.sta.channel = 0;
  ent_wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
  ent_wifi_config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
  
  ent_wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_ENTERPRISE;
  ent_wifi_config.sta.pmf_cfg.capable = true;
  ent_wifi_config.sta.pmf_cfg.required = false;

  maxlen = sizeof(ent_wifi_config.sta.ssid) - 1;
  if (strlen(ssid) <= maxlen) {
    strcpy((char*)ent_wifi_config.sta.ssid, ssid);
  } else {
    strncpy((char*)ent_wifi_config.sta.ssid, ssid, maxlen);
    ent_wifi_config.sta.ssid[maxlen] = 0;
  }

  ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &ent_wifi_config) );

  ESP_ERROR_CHECK( esp_eap_client_set_identity((uint8_t*)anonymous_id, strlen(anonymous_id)) );
  ESP_ERROR_CHECK( esp_eap_client_set_username((uint8_t*)username,     strlen(username)) );
  ESP_ERROR_CHECK( esp_eap_client_set_password((uint8_t*)password,     strlen(password)) );
  ESP_ERROR_CHECK( esp_wifi_sta_enterprise_enable() );

  ESP_LOGI(TAG, "WiFi Enterprise started.");
  ESP_ERROR_CHECK( esp_wifi_start() );
  wifi_started = true;

  esp_wifi_set_max_tx_power(80);
  esp_wifi_set_ps(WIFI_PS_NONE);

  xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
}

/*! メソッド connected? 本体 */
static void
mrbc_esp32_wifi_connected(mrb_vm* vm, mrb_value* v, int argc)
{
  if(connection_status == CONNECTED) {
    SET_TRUE_RETURN();
  }else{
    SET_FALSE_RETURN();
  }
}

/*! メソッド scan 本体 (未起動時の一時起動ロジックを統合) */
static void mrbc_esp32_wifi_scan(mrb_vm* vm, mrb_value* v, int argc)
{
  wifi_mode_t mode;
  ESP_ERROR_CHECK(esp_wifi_get_mode(&mode));
  if((mode & WIFI_MODE_STA) == 0){
    ESP_LOGD(TAG, "STA is not active. scan is not allowed");
    SET_FALSE_RETURN();
    return;
  }
  
  uint16_t scan_size = 10;
  uint16_t number = scan_size;
  wifi_ap_record_t ap_info[scan_size];
  uint16_t ap_count = 0;

  mrb_value result = mrbc_array_new(vm, 0);
  memset(ap_info, 0, sizeof(ap_info));

  bool local_started = false;
  esp_err_t ret = esp_wifi_scan_start(NULL, true);
  
  // ★ もし「Wi-Fiが起動していない」というエラーが返ってきたら、その場だけ一時的に起動する
  if (ret == ESP_ERR_WIFI_NOT_STARTED) {
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    local_started = true;
    ret = esp_wifi_scan_start(NULL, true); // 再チャレンジ
  }
  
  if (ret != ESP_OK) {
    if (local_started) esp_wifi_stop();
    SET_FALSE_RETURN();
    return;
  }
  
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
  if (ap_count > number) ap_count = number;
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));

  for(uint16_t i = 0; i < ap_count; i++){
    mrbc_value mrbc_ap_records;
    mrbc_value key;
    mrbc_value value;
    char bssid_buf[20];
    mrbc_ap_records = mrbc_hash_new(vm, 0);

    key = mrbc_string_new_cstr(vm, "ssid");
    value = mrbc_string_new_cstr(vm, (char *)ap_info[i].ssid);
    mrbc_hash_set(&mrbc_ap_records, &key, &value);

    key = mrbc_string_new_cstr(vm, "bssid");
    sprintf(bssid_buf, "%02X:%02X:%02X:%02X:%02X:%02X",
            ap_info[i].bssid[0], ap_info[i].bssid[1], ap_info[i].bssid[2],
            ap_info[i].bssid[3], ap_info[i].bssid[4], ap_info[i].bssid[5]);
    value = mrbc_string_new_cstr(vm, bssid_buf);
    mrbc_hash_set(&mrbc_ap_records, &key, &value);

    key = mrbc_string_new_cstr(vm, "channel");
    value = mrbc_integer_value(ap_info[i].primary);
    mrbc_hash_set(&mrbc_ap_records, &key, &value);

    key = mrbc_string_new_cstr(vm, "rssi");
    value = mrbc_integer_value(ap_info[i].rssi);
    mrbc_hash_set(&mrbc_ap_records, &key, &value);
    
    key = mrbc_string_new_cstr(vm, "authmode");
    value = mrbc_string_new_cstr(vm, get_auth_mode_name(ap_info[i].authmode));
    mrbc_hash_set(&mrbc_ap_records, &key, &value);

    key = mrbc_string_new_cstr(vm, "hidden");
    value = mrbc_false_value();
    mrbc_hash_set(&mrbc_ap_records, &key, &value);
    
    mrbc_array_set(&result, i, &mrbc_ap_records);
  }

  // ★ 一時的に起動しただけだった場合は、stopして元の状態（未接続状態）に戻す
  if (local_started) {
    esp_wifi_stop();
  }

  SET_RETURN(result);
}

static char* get_auth_mode_name(const wifi_auth_mode_t auth_mode)
{
  char *auth_mode_name;
  switch(auth_mode){
  case WIFI_AUTH_OPEN: auth_mode_name = "OPEN"; break;
  case WIFI_AUTH_WEP: auth_mode_name = "WEP"; break;
  case WIFI_AUTH_WPA_PSK: auth_mode_name = "WPA PSK"; break;
  case WIFI_AUTH_WPA2_PSK: auth_mode_name = "WPA2 PSK"; break;
  case WIFI_AUTH_WPA_WPA2_PSK: auth_mode_name = "WPA/WPA2 PSK"; break;
  default: auth_mode_name = "Unknown"; break;
  }
  return auth_mode_name;
}

static char* get_mac_address(int argc){
  char* buf = (char *)malloc(sizeof(char) * 20);
  uint8_t mac[6];
  ESP_ERROR_CHECK(esp_read_mac(mac, argc));
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return buf;
}

/*! メソッド mac 本体 */
static void mrbc_esp32_wifi_mac(mrb_vm* vm, mrb_value* v, int argc)
{
  wifi_mode_t mode;
  mrb_value mrbc_mac;
  ESP_ERROR_CHECK(esp_wifi_get_mode(&mode));
  assert(mode == WIFI_MODE_STA || mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA);

  if(mode == WIFI_MODE_STA) {
    mrbc_mac = mrbc_string_new_cstr(vm, get_mac_address(ESP_MAC_WIFI_STA));
  } else if(mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA) {
    mrbc_mac = mrbc_string_new_cstr(vm, get_mac_address(ESP_MAC_WIFI_SOFTAP));
  }
  SET_RETURN(mrbc_mac);
}

/*! メソッド ip 本体 */
static void mrbc_esp32_wifi_ip(mrb_vm* vm, mrb_value* v, int argc)
{
  esp_netif_ip_info_t ip_info;
  mrb_value mrbc_ip;
  ESP_ERROR_CHECK(esp_netif_get_ip_info(sta_netif, &ip_info));
  const int addr_buf_size = 16;
  char addr_buf[addr_buf_size];
  assert(esp_ip4addr_ntoa(&ip_info.ip, addr_buf, addr_buf_size) != NULL);
  mrbc_ip = mrbc_string_new_cstr(vm, addr_buf);
  SET_RETURN(mrbc_ip);
}

/*! メソッド ifconfig 本体 */
static void mrbc_esp32_wifi_ifconfig(mrb_vm* vm, mrb_value* v, int argc)
{
  esp_netif_ip_info_t ip_info;
  esp_netif_dns_info_t dns_info;
  mrbc_value mrbc_ifconfig;
  
  ESP_ERROR_CHECK(esp_netif_get_ip_info(sta_netif, &ip_info));
  ESP_ERROR_CHECK(esp_netif_get_dns_info(sta_netif, ESP_NETIF_DNS_MAIN, &dns_info));

  mrbc_ifconfig = mrbc_hash_new(vm, 0);
  hash_set_address(vm, &mrbc_ifconfig, "ip", &ip_info.ip);
  hash_set_address(vm, &mrbc_ifconfig, "netmask", &ip_info.netmask);
  hash_set_address(vm, &mrbc_ifconfig, "gw", &ip_info.gw);
  hash_set_address(vm, &mrbc_ifconfig, "dns", (esp_ip4_addr_t*)&dns_info.ip);

  SET_RETURN(mrbc_ifconfig);
}

static mrbc_error_code hash_set_address(struct VM* vm, mrbc_value* hash, const char* key, const esp_ip4_addr_t* address) {
  const int addr_buf_size = 16;
  char addr_buf[addr_buf_size];
  mrbc_value mrbc_key = mrbc_string_new_cstr(vm, key);
  assert(esp_ip4addr_ntoa(address, addr_buf, addr_buf_size) != NULL);
  mrbc_value mrbc_value = mrbc_string_new_cstr(vm, addr_buf);
  return mrbc_hash_set(hash, &mrbc_key, &mrbc_value);
}

/*! クラス定義処理 */
void mrbc_esp32_wifi_gem_init(struct VM* vm)
{
  mrbc_class *wlan = mrbc_define_class(0, "WLAN", 0);

  mrbc_define_method(0, wlan, "new",           mrbc_esp32_wifi_new);
  mrbc_define_method(0, wlan, "initialize",    mrbc_esp32_wifi_initialize);
  mrbc_define_method(0, wlan, "connect",       mrbc_esp32_wifi_connect);
  mrbc_define_method(0, wlan, "connected?",    mrbc_esp32_wifi_connected);
  mrbc_define_method(0, wlan, "scan",          mrbc_esp32_wifi_scan);
  mrbc_define_method(0, wlan, "ifconfig",      mrbc_esp32_wifi_ifconfig);
  mrbc_define_method(0, wlan, "mac",           mrbc_esp32_wifi_mac);
  mrbc_define_method(0, wlan, "ip",            mrbc_esp32_wifi_ip);
  mrbc_define_method(0, wlan, "connect_peap",   mrbc_esp32_wifi_connect_peap);
}
