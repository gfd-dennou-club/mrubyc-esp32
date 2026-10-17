#include <stdio.h>

#include "esp_log.h"
#include "esp_mac.h"
#include "mqtt_client.h"
#include "mrubyc.h"

static const char* TAG = "MQTT";

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, "topic/qos0", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "topic/qos1", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_unsubscribe(client, "topic/qos1");
        ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d, return code=0x%02x ", event->msg_id, (uint8_t)*event->data);
        msg_id = esp_mqtt_client_publish(client, "topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            ESP_LOGI(TAG, "Last captured errno : %d (%s)", event->error_handle->esp_transport_sock_errno,
                     strerror(event->error_handle->esp_transport_sock_errno));
        } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
            ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
        } else {
            ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

/*
 * constructor
 */
static void mrbc_esp32_mqtt_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(esp_mqtt_client_handle_t)); 
  mrbc_instance_call_initialize( vm, v, argc );
  vTaskDelay(100 / portTICK_PERIOD_MS);  
}

/*
 * Ruby: MQTT.new("mqtt://10.100.111.110", client_id:"hogehoge", port:1883)
 */
static void mrbc_esp32_mqtt_initialize(mrb_vm *vm, mrb_value v[], int argc)
{

  if(argc < 1){
    ESP_LOGE(TAG, "broker uri required");
    SET_FALSE_RETURN();
    return;
  }
 
  const char* broker_uri_ini = (const char*)GET_STRING_ARG(1);
  const char* client_id_ini = NULL;
  int   port_ini = 1883;
  esp_mqtt_client_config_t mqtt_cfg = {0};
  
  // オプション解析
  MRBC_KW_ARG(client_id, port);
  if( MRBC_ISNUMERIC(port) )  port_ini = MRBC_TO_INT(port);
  if( client_id.tt == MRBC_TT_STRING ) {
    client_id_ini = (const char *)client_id.string->data;
  }
  
  mqtt_cfg.broker.address.uri = broker_uri_ini;
  mqtt_cfg.broker.address.port = port_ini;
  if(client_id_ini){
    mqtt_cfg.credentials.client_id = client_id_ini;
  }

  esp_mqtt_client_handle_t mqtt_client =
    esp_mqtt_client_init(&mqtt_cfg);
  
  if (mqtt_client == NULL) {
    ESP_LOGE(TAG, "Failed to initialize MQTT client");
    SET_FALSE_RETURN();
    return;
  }
  
  *((esp_mqtt_client_handle_t *)(v[0].instance->data)) = mqtt_client;
  ESP_LOGI(TAG, "MQTT initialized with URI: %s", broker_uri_ini);
}

/*
 * Ruby: mqtt.connect
 */
static void mrbc_esp32_mqtt_connect(mrb_vm* vm, mrb_value* v, int argc)
{
  esp_mqtt_client_handle_t
    mqtt_client = *((esp_mqtt_client_handle_t *)(v[0].instance->data));

  if (mqtt_client == NULL) {
    ESP_LOGE(TAG, "MQTT client is not initialized");
    SET_FALSE_RETURN();
    return;
  }

  esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);  
  esp_mqtt_client_start(mqtt_client);
  ESP_LOGI(TAG, "MQTT client start");
}

/*
 * Ruby: MQTT.publish("topic", "payload")
 * 戻り値:
 *   >= 0 : 成功 (esp_mqtt_client_publish が返した msg_id)
 *    -1  : 送信失敗 (MQTTブローカー未接続・キュー溢れ等)
 *    -2  : 未初期化エラー (MQTT.init が呼ばれていない)
 *    -3  : 引数エラー (引数が不足している)
 */
static void mrbc_esp32_mqtt_publish(mrb_vm* vm, mrb_value* v, int argc) {

  esp_mqtt_client_handle_t
    mqtt_client = *((esp_mqtt_client_handle_t *)(v[0].instance->data));
  
  if (argc < 2) {
    ESP_LOGE(TAG, "Argument error: topic and payload are required");
    SET_INT_RETURN(-3);
    return;
  }

  if (mqtt_client == NULL) {
    ESP_LOGE(TAG, "MQTT client is not initialized. Call MQTT.init first.");
    SET_INT_RETURN(-2);
    return;
  }

  const char* topic = (const char*)GET_STRING_ARG(1);
  const char* payload = (const char*)GET_STRING_ARG(2);

  // QoS=0, retain=0 でパブリッシュ
  int msg_id = esp_mqtt_client_publish(mqtt_client, topic, payload, 0, 0, 0);

  if (msg_id != -1) {
    SET_INT_RETURN(msg_id);
  } else {
    ESP_LOGE(TAG, "Failed to publish message to topic: %s", topic);
    SET_INT_RETURN(-1);
  }
}


/*
static void mrbc_esp32_mqtt_subscribe(mrb_vm *vm, mrb_value* v, int argc)
{
  esp_mqtt_client_handle_t
    mqtt_client = *((esp_mqtt_client_handle_t *)(v[0].instance->data));
    
  const char* topic = (const char*)GET_STRING_ARG(1);
  
  if (argc < 1 || v[1].tt != MRBC_TT_STRING) {
    ESP_LOGE(TAG, "Topic must be specified");
    SET_INT_RETURN(-3);
    return;
  }

  if (mqtt_client == NULL) {
    ESP_LOGE(TAG, "MQTT client is not initialized");
    SET_INT_RETURN(-2);
    return;
  }

  int msg_id = esp_mqtt_client_subscribe(
      mqtt_client,
      topic,
      0);

  if (msg_id < 0) {
    ESP_LOGE( TAG, "Failed to subscribe: %s", topic );
    SET_INT_RETURN(-1);
    return;
  }

  ESP_LOGI(TAG, "Subscribe requested: topic=%s, msg_id=%d", topic, msg_id);
  SET_INT_RETURN(msg_id);
}
*/


/*
 * mrubyc-esp32 の命名規則に合わせた gem 初期化関数
 */
void mrbc_esp32_mqtt_gem_init(mrbc_vm* vm) {
  mrb_class* mqtt = mrbc_define_class(0, "MQTT", mrbc_class_object);
  
  mrbc_define_method(0, mqtt, "new",        mrbc_esp32_mqtt_new);
  mrbc_define_method(0, mqtt, "initialize", mrbc_esp32_mqtt_initialize);
  mrbc_define_method(0, mqtt, "connect",    mrbc_esp32_mqtt_connect);
  mrbc_define_method(0, mqtt, "publish",    mrbc_esp32_mqtt_publish);
  //  mrbc_define_method(0, mqtt, "subscribe",  mrbc_esp32_mqtt_subscribe);
}
