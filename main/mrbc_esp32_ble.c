#include <stdint.h>
#include <string.h>

#include "mrbc_esp32_ble.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "mrubyc.h"

#define esp_err_to_name(ret) #ret

#define SCANNING_DURATION 5
#define SCAN_PAUSE_PERIOD 10

#define SCAN_DEBUG 1 /* for debugging */

static struct RClass* mrbc_class_esp32_ble;

int iotexRSSI = -100 ;
int iotexRSSI_flag = 0 ;

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval = 0x50,
    .scan_window = 0x30,
    .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE};

static const char *GAP_TAG = "BLE-scan";

void restartScanning()
{

  //    uint32_t ticks = SCAN_PAUSE_PERIOD * 1000 / portTICK_RATE_MS;
  //    ESP_LOGI(GAP_TAG, "delay %d ticks", ticks);
  //    vTaskDelay(ticks);

    uint32_t duration = SCANNING_DURATION;
    ESP_ERROR_CHECK(esp_ble_gap_start_scanning(duration));
}

static void gap_cb_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
    {
      restartScanning();
      break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        //scan start complete event to indicate scan start successfully or failed
        if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(GAP_TAG, "scan start failed, error status = %x", param->scan_start_cmpl.status);
            break;
        }
        ESP_LOGI(GAP_TAG, "scan start success");

        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT:
    {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt)
        {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
	  // output
	  {
	    uint8_t *p = scan_result->scan_rst.bda;
	    char macAddress[2 + (2 * 6) + 1];
	    uint8_t *p_adv_name, adv_name_len ;

	    p_adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv, ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
	    
	    //	    sprintf(macAddress, "0x%02x%02x%02x%02x%02x%02x", p[0], p[1], p[2], p[3], p[4], p[5]);
	    //	    ESP_LOGI(GAP_TAG, "MAC: %s, RSSI: %d, len: %d", macAddress, scan_result->scan_rst.rssi, adv_name_len); 

	    
	    if (adv_name_len == 5 && scan_result->scan_rst.rssi < 0 && *p_adv_name == 73){
	      iotexRSSI = scan_result->scan_rst.rssi;
	      printf("IN iotexNAME: %d\n", *p_adv_name);
	      printf("IN iotexRSSI: %d\n", iotexRSSI);
	      iotexRSSI_flag = 1;
	    }

	  }
	  break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
            ESP_LOGI(GAP_TAG, "ESP_GAP_SEARCH_INQ_CMPL_EVT");

	    if ( iotexRSSI_flag == 0 ) {
	      iotexRSSI = -100;
	    }
	    iotexRSSI_flag = 0;
            restartScanning();

            break;
        default:
            ESP_LOGI(GAP_TAG, "scan_result->scan_rst.search_evt = %d", scan_result->scan_rst.search_evt);
            break;
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(GAP_TAG, "scan stop failed, error status = %x", param->scan_stop_cmpl.status);
            break;
        }
        ESP_LOGI(GAP_TAG, "stop scan successfully");
        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(GAP_TAG, "adv stop failed, error status = %x", param->adv_stop_cmpl.status);
            break;
        }
        ESP_LOGI(GAP_TAG, "stop adv successfully");
        break;

    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        ESP_LOGI(GAP_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                 param->update_conn_params.status,
                 param->update_conn_params.min_int,
                 param->update_conn_params.max_int,
                 param->update_conn_params.conn_int,
                 param->update_conn_params.latency,
                 param->update_conn_params.timeout);
        break;
    default:
        ESP_LOGI(GAP_TAG, "ESP_GAP_BLE event = %d", event);
        break;
    }
}


void setupBLEScanning()
{
    //register the  callback function to the gap module
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_cb_event_handler));
    ESP_ERROR_CHECK(esp_ble_gap_set_scan_params(&ble_scan_params));
}

static void
mrbc_esp32_ble_init(mrb_vm* vm, mrb_value* v, int argc)
{
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    setupBLEScanning();

}

static void
mrbc_esp32_ble_get_rssi(mrb_vm* vm, mrb_value* v, int argc)
{
  int val;
  val = iotexRSSI;
  printf("------------ rssi = %d ------------ \n", val);      
  //  return val;    
  SET_INT_RETURN( val );
}


void
mrbc_mruby_esp32_ble_gem_init(struct VM* vm)
{
  // クラス BLE 定義
  mrbc_class_esp32_ble = mrbc_define_class(vm, "BLE", mrbc_class_object);

  // 各メソッド定義
  mrbc_define_method(vm, mrbc_class_esp32_ble, "init",     mrbc_esp32_ble_init);
  mrbc_define_method(vm, mrbc_class_esp32_ble, "get_rssi", mrbc_esp32_ble_get_rssi);
}


  
