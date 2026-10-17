#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"
#include "mrubyc.h"

static const char* TAG = "ESPNow";

#define MSG_QUEUE_SIZE 10
#define MSG_MAX_LEN 32
#define ESPNOW_CHANNEL 1

static uint8_t receiver_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
//static uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static char msg_queue[MSG_QUEUE_SIZE][MSG_MAX_LEN];
static int queue_head = 0;
static int queue_tail = 0;

static void add_to_queue(const char* data, int len) {
  int next = (queue_tail + 1) % MSG_QUEUE_SIZE;
  if (next != queue_head) {
    int copy_len = len < (MSG_MAX_LEN - 1) ? len : (MSG_MAX_LEN - 1);
    memcpy(msg_queue[queue_tail], data, copy_len);
    msg_queue[queue_tail][copy_len] = '\0';
    queue_tail = next;
  }
}

static int get_from_queue(char* out_buf) {
  if (queue_head == queue_tail) {
    return 0;
  }
  strcpy(out_buf, msg_queue[queue_head]);
  queue_head = (queue_head + 1) % MSG_QUEUE_SIZE;
  return 1;
}

static void esp_now_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
  if (data && len > 0) {
    add_to_queue((const char*)data, len);
  }
}

static bool parse_mac_address(const char *str, uint8_t mac[ESP_NOW_ETH_ALEN])
{
  unsigned int value[ESP_NOW_ETH_ALEN];

  int count = sscanf(
    str,
    "%02x:%02x:%02x:%02x:%02x:%02x",
    &value[0],
    &value[1],
    &value[2],
    &value[3],
    &value[4],
    &value[5]
  );

  if (count != ESP_NOW_ETH_ALEN) {
    return false;
  }

  for (int i = 0; i < ESP_NOW_ETH_ALEN; i++) {
    if (value[i] > 0xff) {
      return false;
    }
    
    mac[i] = (uint8_t)value[i];
  }
  
  return true;
}

/*
 * constructor
 */
static void mrbc_esp32_espnow_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  v[0] = mrbc_instance_new(vm, v[0].cls, 0);
  mrbc_instance_call_initialize( vm, v, argc );
  vTaskDelay(100 / portTICK_PERIOD_MS);  
}

static void mrbc_esp32_espnow_initialize(mrb_vm* vm, mrb_value* v, int argc) {
  const char* mode = (const char*)GET_STRING_ARG(1);
  const char* mode_send = "send" ;
  const char* mode_recv = "recv" ;

  
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
  
  ESP_ERROR_CHECK(esp_now_init());

  if (strcmp(mode, mode_send) == 0){

    //第 2 引数があればユニキャスト送信
    if (argc >= 2) {
      const char *mac_string = (const char *) GET_STRING_ARG(2);
      if (!parse_mac_address(mac_string, receiver_mac)) {
	ESP_LOGE(TAG, "Invalid MAC address: %s", mac_string);
	return;
      }
    }

    esp_now_peer_info_t peer_info = {};
    //memcpy(peer_info.peer_addr, broadcast_mac, ESP_NOW_ETH_ALEN);
    memcpy(peer_info.peer_addr, receiver_mac, ESP_NOW_ETH_ALEN);    
    peer_info.channel = ESPNOW_CHANNEL;
    peer_info.encrypt = false;
    esp_now_add_peer(&peer_info);
    ESP_LOGI(TAG, "ESP-NOW Senser initialized.\n");

    }else if (strcmp(mode, mode_recv) == 0){
    ESP_ERROR_CHECK(esp_now_register_recv_cb(esp_now_recv_cb));
    ESP_LOGI(TAG, "ESP-NOW Receiver initialized.\n");

  } else{
    ESP_LOGE(TAG, "ESP-NOW unknown mode, stop");
  }
}

static void mrbc_esp32_espnow_send(mrb_vm* vm, mrb_value* v, int argc) {
  const char* msg = (const char*)GET_STRING_ARG(1);

  if (msg == NULL) {
    ESP_LOGE(TAG, "Message is NULL");
    SET_FALSE_RETURN();
    return;
  }

  //esp_err_t ret = esp_now_send( broadcast_mac, (const uint8_t *)msg, strlen(msg) );
  esp_err_t ret = esp_now_send( receiver_mac, (const uint8_t *)msg, strlen(msg) );

  if (ret != ESP_OK) {
    ESP_LOGE( TAG, "esp_now_send failed: %s (0x%x)", esp_err_to_name(ret), ret );
    SET_FALSE_RETURN();
    return;
  }
  
  ESP_LOGI(TAG, "send requested: \"%s\", len=%d", msg, (int)strlen(msg) );
  SET_TRUE_RETURN();  
}

static void mrbc_esp32_espnow_recv(mrb_vm* vm, mrb_value* v, int argc) {
  char buf[MSG_MAX_LEN];
  if (get_from_queue(buf)) {
    mrbc_value str = mrbc_string_new_cstr(vm, buf);
    SET_RETURN(str);
  } else {
    SET_NIL_RETURN();
  }
}

void mrbc_esp32_espnow_gem_init(struct VM* vm) {
  struct RClass* mrbc_class_espnow = mrbc_define_class(vm, "ESPNow", mrbc_class_object);
  mrbc_define_method(vm, mrbc_class_espnow, "new",        mrbc_esp32_espnow_new);
  mrbc_define_method(vm, mrbc_class_espnow, "initialize", mrbc_esp32_espnow_initialize);
  mrbc_define_method(vm, mrbc_class_espnow, "send",       mrbc_esp32_espnow_send);
  mrbc_define_method(vm, mrbc_class_espnow, "recv",       mrbc_esp32_espnow_recv);  
}
