/* master.mrbc と slave.mrbc をどうしよう．今は前者のみ書き込み*/

#include <stdio.h>
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "esp_spiffs.h"
#include "mrubyc.h"

//*********************************************
// ENABLE LIBRARY written by C
//*********************************************
#include "mrbc_esp32_gpio.h"
#include "mrbc_esp32_ledc.h"
#include "mrbc_esp32_adc.h"
#include "mrbc_esp32_uart.h"
#include "mrbc_esp32_i2c.h"
#include "mrbc_esp32_wifi.h"
#include "mrbc_esp32_sntp.h"
#include "mrbc_esp32_http_client.h"
#include "mrbc_esp32_sleep.h"
#include "mrbc_esp32_spi.h"
#include "mrbc_esp32_utils.h"

//*********************************************
// ENABLE MASTER files written by mruby/c
//*********************************************
#include "master.h"
#include "slave.h"

static const char *TAG = "mrubyc-esp32";

#define BUF_SIZE (1024)
#define MEMORY_SIZE (1024*70)
#define RD_BUF_SIZE (BUF_SIZE)
static QueueHandle_t uart0_queue;
static uint8_t memory_pool[MEMORY_SIZE];

// SPIFFS でバイナリデータを書き込み
uint8_t * save_spiffs_file(const char *filename, int len, uint8_t *data)
{
  //  FILE* fp = fopen(filename, "ab");
  FILE* fp = fopen(filename, "ab");
  if (fp == NULL) {
    ESP_LOGE(TAG, "Failed to open file for writing (%s)\n", filename);
    return NULL;
  }
  for (int i = 0; i < len; i++){
    fwrite(&data[i], sizeof(uint8_t), 1, fp);
  }
  fclose(fp);
  return NULL;
}

// SPIFFS でバイナリデータを読み込み
uint8_t * load_spiffs_file(const char *filename)
{
  FILE *fp = fopen(filename, "rb");
  if( fp == NULL ) {
    fprintf(stderr, "File not found (%s)\n", filename);
    return NULL;
  }
  
  /* // get filesize */
  fseek(fp, 0, SEEK_END);
  size_t size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  // allocate memory
  uint8_t *p = malloc(size);
  if( p != NULL ) {
    fread(p, sizeof(uint8_t), size, fp);
  } else {
    fprintf(stderr, "Memory allocate error.\n");
  }
  fclose(fp);
  ESP_LOG_BUFFER_HEXDUMP(TAG, p, size, ESP_LOG_INFO);

  return p;
}

//SPIFFS 初期化
uint8_t init_spiffs(){
  esp_vfs_spiffs_conf_t conf = {
    .base_path = "/spiffs",
    .partition_label = NULL,
    .max_files = 2,
    .format_if_mount_failed = true
  };
  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount or format filesystem");
    } else if (ret == ESP_ERR_NOT_FOUND) {
      ESP_LOGE(TAG, "Failed to find SPIFFS partition");
    } else {
      ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
    }
    return 1;
  }
  size_t total = 0, used = 0;
  ret = esp_spiffs_info(conf.partition_label, &total, &used);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
  } else {
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
  }
  return 1;
}

//UART 初期化
uint8_t init_uart(){
  uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_APB,
  };
  //uart_driver_install(UART_NUM_0, 2*1024, 0, 0, NULL, 0);
  //uart_param_config(UART_NUM_0, &uart_config);
  uart_driver_install(UART_NUM_0, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0);
  uart_param_config(UART_NUM_0, &uart_config);
  return 1;
}  



//*******************************************
// メインプログラム
//
//*******************************************

void app_main(void) {

  //************************************
  // 初期化
  //************************************

  //変数初期化
  uint8_t* data = (uint8_t*) malloc(BUF_SIZE);
  uint8_t wait = 0;
  uint8_t flag_cmd_mode = 0;
  uint8_t flag_write_mode = 1;
  uint8_t ifile = 0;
  char buffer[BUF_SIZE];
  struct stat st;
      
  // SPIFFS 初期化
  init_spiffs();

  // UART0 初期化
  init_uart();

  //************************************
  // mrbcwrite モード開始
  //************************************
  ESP_LOGI(TAG, "");  
  ESP_LOGI(TAG, "Please push Enter key x 2 to mrbwite mode");  
  printf("\n\n");

  while (1) {
    //バイト数の取得
    int len = uart_read_bytes(UART_NUM_0, data, BUF_SIZE, 1000 / portTICK_PERIOD_MS);
    
    //取得したバイト数が正か否かで場合分け
    if (len > 0) {
      
      //表示
      //ESP_LOGI(TAG, "Read %d bytes", len);
      //ESP_LOG_BUFFER_HEXDUMP(TAG, data, len, ESP_LOG_INFO);
      
      wait = 0;  //waiting の変数のクリア
      
      //文字型に変換
      for (int i = 0; i < len; i++){
	buffer[i] = data[i];
      }
      buffer[len] = '\0'; //末尾に終了記号

      //コマンドモードに入っていない場合
      if (flag_cmd_mode == 0){

	//コマンドモードに入っていない状態で Enter が 2 度打鍵された場合は
	//コマンドモードに入るためのフラグを立てる
	if ( (data[0] == 0x0d && data[1] == 0x0d) ||
	     (data[0] == 0x0d && data[2] == 0x0d && data[1] == 0x0a && data[3] == 0x0a ) ) {
	  //ESP_LOGI(TAG, "Entering Command Mode ...");
	  printf("+OK mruby/c \n\n");
	  flag_cmd_mode = 1;
	}

      //コマンドモードに入っている場合
      }else{

	//reset
	if (strncmp(buffer, "reset", 5) == 0) {
	  printf("+OK reset \n\n");
	  vTaskDelay(2000 / portTICK_PERIOD_MS);
	  esp_restart();

	//execute	  
	} else if (strncmp(buffer, "execute", 7) == 0) {
	  printf("+OK execute \n\n");
	  vTaskDelay(2000 / portTICK_PERIOD_MS);
	  break; //ループから抜ける
	  
	//write	  
	} else if (strncmp(buffer, "write", 5) == 0) {
	  printf("+OK write bytecode \n\n");	  
	  flag_write_mode = 1;  //書き込みフラグを立てる

	  // 書き込み先ファイルの決定.
	  if (stat("/spiffs/master.mrbc", &st) != 0) {
	    printf("+OK writting to master.mrbc \n\n");
	    ifile = 1;
	  }else if (stat("/spiffs/slave.mrbc", &st) != 0) {
	    printf("+OK writting to slave.mrbc \n\n");
	    ifile = 2;
	  } else {
	    ESP_LOGE(TAG, "Failed to determine file to write");
	    ifile = 0;
	    flag_write_mode = 0; //書き込みモード終了	      
	  }

	//clear	  
	} else if (strncmp(buffer, "clear", 5) == 0) {
	  printf("+OK clear \n\n");

	  //ファイルを消す
	  if (stat("/spiffs/master.mrbc", &st) == 0) {
	    unlink("/spiffs/master.mrbc");
	    printf("+OK Delete master.mrbc \n\n");
	  }
	  if (stat("/spiffs/slave.mrbc", &st) == 0) {
	    unlink("/spiffs/slave.mrbc");
	    printf("+OK Delete slave.mrbc \n\n");
	  }
	  
        //help	  
	} else if (strncmp(buffer, "help", 4) == 0) {
	  printf("+OK help   \n");
	  printf("  version  \n");
	  printf("  write    \n");
	  printf("  showprog \n");
	  printf("  clear    \n");
	  printf("  reset    \n");
	  printf("  execute  \n");

        //version
	} else if (strncmp(buffer, "version", 6) == 0) {
	  printf("+OK mruby/c 3.1 \n\n");

	//showprog
	} else if (strncmp(buffer, "showprog", 8) == 0) {
	  printf("+OK show program \n\n");

	  //読み込み
	  if (stat("/spiffs/master.mrbc", &st) == 0) {
	    printf("**** master.mrbc **** \n\n");
	    load_spiffs_file("/spiffs/master.mrbc");
	  }
	  if (stat("/spiffs/slave.mrbc", &st) == 0) {	  
	    printf("**** slave.mrbc **** \n\n");
	    load_spiffs_file("/spiffs/slave.mrbc");
	  }
	  
	// 書き込み
	} else if (flag_write_mode == 1){

	  //ファイル書き込み
	  if (ifile == 1) {
	    save_spiffs_file("/spiffs/master.mrbc", len, data);
	  } else if (ifile == 2){
	    save_spiffs_file("/spiffs/slave.mrbc", len, data);
	  }
	  
	  //書き込みを継続するか否か．バッファーサイズと読み込みバイト数で判断．
	  if (len < BUF_SIZE ) { 
	    printf("+DONE write bytecode \n\n");
	    flag_write_mode = 0; //書き込みモード終了
	  }
	}	 	
      }
    }else{
      //入力バイト数がゼロの場合

      //コマンドモードでなければカウントアップ
      if (flag_cmd_mode == 0){
	wait += 1; 
      }
      //閾値を越えたらタイムアウト
      if (wait > 2){
	break; 
      }      
    }
  }
  //書き込みモード終了
  ESP_LOGI(TAG, "End mrbwrite mode");

  
  
  //***************************************
  // Ruby 
  //***************************************
  mrbc_init(memory_pool, MEMORY_SIZE);

  printf("start GPIO (C)\n");
  mrbc_esp32_gpio_gem_init(0);
  printf("start PWM (C)\n");
  mrbc_esp32_ledc_gem_init(0);
  printf("start ADC (C)\n");
  mrbc_esp32_adc_gem_init(0);
  printf("start I2C (C)\n");
  mrbc_esp32_i2c_gem_init(0);
  printf("start UART (C)\n");
  mrbc_esp32_uart_gem_init(0);
  printf("start WiFi (C) \n");
  mrbc_esp32_wifi_gem_init(0);
  mrbc_esp32_sntp_gem_init(0);
  mrbc_esp32_httpclient_gem_init(0);
  printf("start SLEEP (C) \n");
  mrbc_esp32_sleep_gem_init(0);
  printf("start SPI (C) \n");
  mrbc_esp32_spi_gem_init(0);
  printf("start Utils (C) \n");
  mrbc_esp32_utils_gem_init(0);  
    
  // Ruby 側のクラス・メソッド定義
  extern const uint8_t myclass_bytecode[];
  mrbc_run_mrblib(myclass_bytecode);
  
  // tasks
#ifdef CONFIG_USE_ESP32_FIRMWAREFLASH
  printf("FIRMWAREFLASH mode\n");
  mrbc_create_task(master, 0);
  mrbc_create_task( slave, 0 );
#else
  printf("SPIFFS mode\n");

  uint8_t *master = load_spiffs_file("/spiffs/master.mrbc");
  mrbc_create_task(master, 0);

  if (stat("/spiffs/slave.mrbc", &st) == 0) {
    uint8_t *slave = load_spiffs_file("/spiffs/slave.mrbc");
    mrbc_create_task( slave, 0 );
  }
#endif

  mrbc_run();

}

