/* master.mrbc と slave.mrbc をどうしよう．今は前者のみ書き込み*/

#include <stdio.h>
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
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
#include "mrbc_esp32_lcdspi.h"
#include "mrbc_esp32_sdspi.h"
#include "mrbc_esp32_stdio.h"
#include "mrbc_esp32_dirent.h"
#include "mrbc_esp32_utils.h"

static const char *TAG = "mrubyc-esp32";

#define MRUBYC_VERSION_STRING "mruby/c v3.3.1 RITE0300 MRBW1.2"
#define BUF_SIZE (1024)
#define MEMORY_SIZE (1024*70)
#define RD_BUF_SIZE (BUF_SIZE)
//static QueueHandle_t uart0_queue;
static uint8_t memory_pool[MEMORY_SIZE];

//UART Number
const uart_port_t uart_num = CONFIG_UART_NUM;  // make menuconfig 

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
uint8_t * load_spiffs_file(const char *filename, uint8_t flag)
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

  if (flag == 1){
    ESP_LOG_BUFFER_HEXDUMP(TAG, p, size, ESP_LOG_INFO);  //バイトコード出力
  }
  
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

/*!
 * UART バッファクリア
 */
uint8_t clear_uart_buffer(){
  //バッファークリア
  ESP_ERROR_CHECK( uart_flush( uart_num ) );
  ESP_ERROR_CHECK( uart_flush_input( uart_num ) );
  return 1;  
}

/*
 * UART 初期化
 */
uint8_t init_uart(){
  uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_APB,
  };

  // UARTパラメータの設定
  ESP_ERROR_CHECK( uart_param_config(uart_num, &uart_config) );
  
  // UART pin 設定
  if (uart_num == 2){
    ESP_ERROR_CHECK( uart_set_pin(uart_num, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) );
  }

  // UARTドライバのインストール
  ESP_ERROR_CHECK( uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0) );
  //ESP_ERROR_CHECK( uart_driver_install(uart_num, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0) );
  
  if (uart_num == 2){  
    // 標準入出力をリダイレクト
    FILE* uart_output = fopen("/dev/uart/2", "w");
    if (uart_output != NULL) {
      setvbuf(uart_output, NULL, _IONBF, 0);
      stdout = uart_output;
      stderr = uart_output;
    }
  }
  
  clear_uart_buffer(); //clear
  return 1;
}  


/*!
* @brief リセットコマンド
*/
void mrbwrite_cmd_reset() {
  printf("+OK \n\n");
  vTaskDelay(2000 / portTICK_PERIOD_MS);
  esp_restart();
}

/*!
* @brief 書き込まれたコードを実行
*/
void mrbwrite_cmd_execute() {
  printf("+OK \n\n");
  vTaskDelay(2000 / portTICK_PERIOD_MS);
}

/*!
* @brief バイトコードの書き込み(書き込みモードの開始)
*/
void mrbwrite_cmd_write(struct stat *st, uint8_t *flag_write_mode, uint8_t *ifile) {
  printf("+OK Write bytecode \n\n");
  *flag_write_mode = 1;
  // 書き込み先ファイルの決定.
  if (stat("/spiffs/master.mrbc", st) != 0) {
    *ifile = 1;
  } else if (stat("/spiffs/slave.mrbc", st) != 0) {
    *ifile = 2;
  } else {
    ESP_LOGE(TAG, "Failed to determine file to write");
    *ifile = 0;
    *flag_write_mode = 0; //書き込みモード終了
  }
}

/*!
* @brief 書き込まれたバイトコードの消去
*/
void mrbwrite_cmd_clear(struct stat *st) {
  //ファイルを消す
  if (stat("/spiffs/master.mrbc", st) == 0) {
    unlink("/spiffs/master.mrbc");
  }
  if (stat("/spiffs/slave.mrbc", st) == 0) {
    unlink("/spiffs/slave.mrbc");
  }
  printf("+OK \n\n");
}

/*!
* @brief ヘルプ(コマンド一覧)の表示
*/
void mrbwrite_cmd_help() {
  printf("  version  \n");
  printf("  write    \n");
  printf("  showprog \n");
  printf("  clear    \n");
  printf("  reset    \n");
  printf("  execute  \n");
  printf("+OK \n\n");
}

/*!
* @brief マイコン/mrubycのバージョン表示
*/
void mrbwrite_cmd_version() {
  printf("+OK %s\n\n", MRUBYC_VERSION_STRING);
}

/*!
* @brief プログラムの表示
*/
void mrbwrite_cmd_showprog(struct stat *st) {
  //読み込み
  if (stat("/spiffs/master.mrbc", st) == 0) {
    printf("**** master.mrbc **** \n\n");
    load_spiffs_file("/spiffs/master.mrbc", 1);
  }
  if (stat("/spiffs/slave.mrbc", st) == 0) {
    printf("**** slave.mrbc **** \n\n");
    load_spiffs_file("/spiffs/slave.mrbc", 1);
  }
  printf("+DONE\n\n");
}

/*!
* @return 0: 何もしない / 1: コマンドモードを抜けた
*/
int mrbwrite_cmd_mode(
  struct stat *st,
  uint8_t *ifile,
  uint8_t *flag_write_mode,
  const char buffer[BUF_SIZE],
  int len,
  uint8_t *data
) {
  //コマンドモードに入っている場合
  if (strncmp(buffer, "reset", 5) == 0) {
    //reset
    mrbwrite_cmd_reset();
  } else if (strncmp(buffer, "execute", 7) == 0) {
    //execute
    mrbwrite_cmd_execute();
    return 1;
  } else if (strncmp(buffer, "write", 5) == 0) {
    //write
    mrbwrite_cmd_write(st, flag_write_mode, ifile);
  } else if (strncmp(buffer, "clear", 5) == 0) {
    //clear
    mrbwrite_cmd_clear(st);
  } else if (strncmp(buffer, "help", 4) == 0) {
    //help
    mrbwrite_cmd_help();
  } else if (strncmp(buffer, "version", 6) == 0) {
    //version
    mrbwrite_cmd_version();
  } else if (strncmp(buffer, "showprog", 8) == 0) {
    //showprog
    mrbwrite_cmd_showprog(st);
  } else if (*flag_write_mode == 1) {
    // 書き込み
    
    //ファイル書き込み
    if (*ifile == 1) {
      save_spiffs_file("/spiffs/master.mrbc", len, data);
    } else if (*ifile == 2){
      save_spiffs_file("/spiffs/slave.mrbc", len, data);
    }

    //書き込みを継続するか否か．バッファーサイズと読み込みバイト数で判断．
    if (len < BUF_SIZE ) { 
      printf("+DONE\n\n");
      flag_write_mode = 0; //書き込みモード終了
    }
  }
  return 0;
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
  uint8_t flag_write_mode = 0;
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
  printf("mrubyc-esp32: Please push Enter key x 2 to mrbwite mode\n\n");
  
  while (wait < 2) {
    //バイト数の取得
    int len = uart_read_bytes(uart_num, data, BUF_SIZE, 1000 / portTICK_PERIOD_MS);
    
    //取得したバイト数が正か否かで場合分け
    if (len > 0) {
      //表示
      ESP_LOGI(TAG, "Read %d bytes", len);
      ESP_LOG_BUFFER_HEXDUMP(TAG, data, len, ESP_LOG_INFO);
      
      wait = 0;  //waiting の変数のクリア
      
      //文字型に変換
      for (int i = 0; i < len; i++){
	      buffer[i] = data[i];
      }
      buffer[len] = '\0'; //末尾に終了記号
      
      if (flag_cmd_mode == 0){
	//コマンドモードに入っていない状態で Enter (CR+LF) が打鍵された場合は
	//コマンドモードに入るためのフラグを立てる
	if ( data[0] == 0x0d && data[1] == 0x0a ) {
	  //ESP_LOGI(TAG, "Entering Command Mode ...");
	  printf("+OK mruby/c \n\n");
	  flag_cmd_mode = 1;
	}
      } else {
	//コマンドモードに入っている場合
        int cmd_state = mrbwrite_cmd_mode(
          &st,
          &ifile,
          &flag_write_mode,
          buffer,
          len,
          data
        );
        if (cmd_state == 1) break;
      }
    } else {
      //コマンドモードでなければカウントアップ
      if (flag_cmd_mode == 0){
	wait += 1; 
      }
    }
    clear_uart_buffer();
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  //書き込みモード終了
  ESP_LOGI(TAG, "End mrbwrite mode");
  printf("Kani-Board, mruby/c v3.3.1 start\n");
  
  //***************************************
  // Ruby 
  //***************************************
  mrbc_init(memory_pool, MEMORY_SIZE);

  ESP_LOGI(TAG, "start GPIO (C)\n");
  mrbc_esp32_gpio_gem_init(0);
  ESP_LOGI(TAG, "start PWM (C)\n");
  mrbc_esp32_ledc_gem_init(0);
  ESP_LOGI(TAG, "start ADC (C)\n");
  mrbc_esp32_adc_gem_init(0);
  ESP_LOGI(TAG, "start I2C (C)\n");
  mrbc_esp32_i2c_gem_init(0);
  if (uart_num < 2){
    ESP_LOGI(TAG, "start UART (C)\n");
    mrbc_esp32_uart_gem_init(0);
  }
  ESP_LOGI(TAG, "start WiFi (C) \n");
  mrbc_esp32_wifi_gem_init(0);
  mrbc_esp32_sntp_gem_init(0);
  mrbc_esp32_httpclient_gem_init(0);
  ESP_LOGI(TAG, "start SLEEP (C) \n");
  mrbc_esp32_sleep_gem_init(0);
  ESP_LOGI(TAG, "start SPI (C) \n");
  mrbc_esp32_spi_gem_init(0);
  mrbc_esp32_lcdspi_gem_init(0);
  mrbc_esp32_sdspi_gem_init(0);  
  mrbc_esp32_stdio_gem_init(0);
  mrbc_esp32_dirent_gem_init(0);
  ESP_LOGI(TAG, "start Utils (C) \n");
  mrbc_esp32_utils_gem_init(0);
  
  // Ruby 側のクラス・メソッド定義
  extern const uint8_t myclass_bytecode[];
  mrbc_run_mrblib(myclass_bytecode);
  
  // tasks
  ESP_LOGI(TAG, "SPIFFS mode\n");

  uint8_t *master = load_spiffs_file("/spiffs/master.mrbc", 0);
  mrbc_create_task(master, 0);

  if (stat("/spiffs/slave.mrbc", &st) == 0) {
    uint8_t *slave = load_spiffs_file("/spiffs/slave.mrbc", 0);
    mrbc_create_task( slave, 0 );
  }

  mrbc_run();
}
