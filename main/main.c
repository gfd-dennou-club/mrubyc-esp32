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

/*!
* @brief SPIFFS でバイナリデータを読み込み
* @param *filename ファイルのパス
*/
size_t get_file_size(const char *filename)
{
  FILE *fp= fopen(filename, "rb");
  if( fp == NULL ) {
    fprintf(stderr, "File not found (%s)\n", filename);
    return 0;
  }
  
  /* // get filesize */
  fseek(fp, 0, SEEK_END);
  size_t size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  fclose(fp);
  
  return size;
}
/*!
* @brief SPIFFS でバイナリデータを書き込み
* @param *filename ファイルのパス
* @param len バイナリのサイズ
* @param *data 書き込むバイナリ
*/
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


/*!
* @brief SPIFFS でバイナリデータを読み込み
* @param *filename ファイルのパス
*/
uint8_t * load_spiffs_file(const char *filename)
{
  FILE *fp = fopen(filename, "rb");
  if( fp == NULL ) {
    fprintf(stderr, "File not found (%s)\n", filename);
    return NULL;
  }
  
  size_t size = get_file_size(filename);
  // allocate memory
  uint8_t *p = malloc(size);
  if( p != NULL ) {
    fread(p, sizeof(uint8_t), size, fp);
  } else {
    fprintf(stderr, "Memory allocate error.\n");
  }
  fclose(fp);

  return p;
}


/**
 * 与えられたバイナリデータのCRC8ハッシュ値を計算する
 * - CRCレジスタ初期値: `0xff`
 * - 生成多項式: `x^8 + x^5 + x^4 + 1`(`0x31`)
 * - RefIn: なし(`false`)
 * - RefOut: なし(`false`)
 * - XorOut: なし(`0x00`)
 *
 * @see https://www.sunshine2k.de/articles/coding/crc/understanding_crc.html
 * @brief filenameに書き込まれているバイトコードのハッシュ値を計算する
 * @param *filename 確認したいファイルのパス
 */
uint8_t calculateCrc8(const uint8_t *data,const size_t size) {
uint8_t crc = 0xFF;
const uint8_t poly = 0x31;
for (size_t i = 0; i < size; i++) {
    crc ^= data[i];

    for (int j = 8; j > 0; --j) {
        if (crc & 0x80) {
            crc = (crc << 1) ^ poly;
        } else {
            crc <<= 1;
        }
    }
}
return crc;
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

/*
 * UART 初期化
 */
uint8_t init_uart(){
  ESP_ERROR_CHECK( uart_driver_delete(uart_num) ); // 既存ドライバの削除
  
  uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_APB,
  };

  // UARTドライバのインストール
  ESP_ERROR_CHECK( uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0) );
  //ESP_ERROR_CHECK( uart_driver_install(uart_num, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0) );

  // UARTパラメータの設定
  ESP_ERROR_CHECK( uart_param_config(uart_num, &uart_config) );
  
  // UART pin 設定
  if (uart_num == 2){
    ESP_ERROR_CHECK( uart_set_pin(uart_num, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) );
  }

  if (uart_num == 2){  
    // 標準入出力をリダイレクト
    FILE* uart_output = fopen("/dev/uart/2", "w");
    if (uart_output != NULL) {
      setvbuf(uart_output, NULL, _IONBF, 0);
      stdout = uart_output;
      stderr = uart_output;
    }
  }
  
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
  /*
  //ファイルを消す
  if (stat("/spiffs/master.mrbc", st) == 0) {
    unlink("/spiffs/master.mrbc");
  }
  if (stat("/spiffs/slave.mrbc", st) == 0) {
    unlink("/spiffs/slave.mrbc");
  }
  printf("+OK \n\n");
  */
  
  // format spiffs region
  esp_err_t ret = esp_spiffs_format(NULL);
  vTaskDelay(pdMS_TO_TICKS(50)); //wait
  
  if (ret == ESP_OK) {
    printf("+OK\n\n");
  } else {
    printf("-ERR: Format failed (0x%x)\n\n", ret);
  }
}

/*!
* @brief ヘルプ(コマンド一覧)の表示
*/
void mrbwrite_cmd_help() {
  printf("+OK \n\n");
  printf("Commands:\n");
  printf("  version  \n");
  printf("  write    \n");
  printf("  showprog \n");
  printf("  clear    \n");
  printf("  reset    \n");
  printf("  execute  \n");
  printf("  verify  \n");
  printf("+DONE \n\n");
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
    uint8_t *data = load_spiffs_file("/spiffs/master.mrbc");
    if(data!=NULL)
    {      
      size_t size = get_file_size("/spiffs/master.mrbc");
      ESP_LOG_BUFFER_HEXDUMP(TAG, data, size, ESP_LOG_ERROR);  //バイトコード出力
    }
  if (stat("/spiffs/slave.mrbc", st) == 0) {
    printf("**** slave.mrbc **** \n\n");
    uint8_t *data = load_spiffs_file("/spiffs/slave.mrbc");
    if(data!=NULL)
    {      
      size_t size = get_file_size("/spiffs/slave.mrbc");
      ESP_LOG_BUFFER_HEXDUMP(TAG, data, size, ESP_LOG_ERROR);  //バイトコード出力
    }
  }
  printf("+DONE\n\n");
  }
}

/*!
* @brief 最後に書き込まれたバイトコードのCRC8ハッシュ値を計算して返す
*/
void mrbwrite_cmd_verify()
{
  //Memo:複数ファイルの書き込みを行うようにした場合はファイル名の取得して行う
  uint8_t *data = load_spiffs_file("/spiffs/master.mrbc");
  size_t size = get_file_size("/spiffs/master.mrbc");
  uint8_t hash = calculateCrc8(data,size);
  printf("+OK %2x\n",hash);
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
  uint8_t *data,
  size_t *totallen
) {
  char copybuffer[BUF_SIZE];
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
    //write [バイト数]という風にコマンドが来るため後ろのバイト数を取得する
    char * ret;
    strcpy(copybuffer, buffer);
    strtok(copybuffer," ");
    ret = strtok(NULL, " ");
    //char型から変換
    for(int i = strlen(ret)-1,base = 1; 0 <= i;i--)
    {
      int a = ret[i] - '0';
      if( 0<=a && a<=9)
      {
        *totallen += a * base;
        base*=10;
      }
    }
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
  }else if(strncmp(buffer, "verify", 6) == 0){
    //verify
    mrbwrite_cmd_verify();
  }else if (*flag_write_mode == 1) {
    //ファイル書き込み
    if (*ifile == 1) {
      save_spiffs_file("/spiffs/master.mrbc", len, data);
    } else if (*ifile == 2){
      save_spiffs_file("/spiffs/slave.mrbc", len, data);
    }
    *totallen -= len;
    //書き込みを継続するか否か．バッファーサイズと読み込みバイト数で判断．
     if (*totallen == 0 ){ 
      printf("+DONE \n\n");
      *flag_write_mode = 0; //書き込みモード終了
    }
  }else{
    if(buffer[0] == 0x0d && buffer[1] == 0x0a)
      printf("+OK mruby/c \n\n");
    else
      printf("-ERR Illegal command.\n\n");
  }
  return 0;
}


//*******************************************
// メインプログラム
//
//*******************************************

void app_main(void) {

  //setvbuf(stdout, NULL, _IONBF, 0); // disable buffering
  vTaskDelay(500 / portTICK_PERIOD_MS); // 若干待ってから送信開始

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
  size_t totallen = 0;
  int write_time = 0;

  // SPIFFS 初期化
  init_spiffs();
  vTaskDelay(100 / portTICK_PERIOD_MS);
  
  // UART0 初期化
  init_uart();
  vTaskDelay(100 / portTICK_PERIOD_MS);
  
  //************************************
  // mrbcwrite モード開始
  //************************************
  printf("Kani-Board, Please push Enter key to mrbwite mode\n\n");

  // clear buffer
  uart_wait_tx_done(uart_num, pdMS_TO_TICKS(100));
  uart_flush_input(uart_num);

  while (wait < 2) {
    
    //バイト数の取得
    int len = uart_read_bytes(uart_num, data, BUF_SIZE, 1000 / portTICK_PERIOD_MS);
    
    //取得したバイト数が正か否かで場合分け
    if (len > 0) {
      wait = 0;  //waiting の変数のクリア

      int start_pos = 0;
      
      // 先頭にある改行(0x0d, 0x0a)をスキップ
      while (start_pos < len && (data[start_pos] == 0x0d || data[start_pos] == 0x0a)) {
	start_pos++;
      }

      //文字型に変換 
      int idx = 0;
      for (int i = start_pos; i < len; i++){
          buffer[idx++] = data[i];
      }
      buffer[len] = '\0'; //末尾に終了記号
      
      if (flag_cmd_mode == 0){
        // Enter (CR+LF) が打鍵された場合はフラグを立てる
        if ( data[0] == 0x0d && data[1] == 0x0a ) {         
          printf("+OK mruby/c \n\n");

	  // clear buffer
          uart_wait_tx_done(uart_num, pdMS_TO_TICKS(100));
          uart_flush_input(uart_num);
	  
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
          data,
          &totallen
        );

	// clear buffer
	uart_wait_tx_done(uart_num, pdMS_TO_TICKS(100));
	uart_flush_input(uart_num);
	
        if (cmd_state == 1) break;
      }
      
    } else {

      //コマンドモードでなければカウントアップ
      if ( flag_cmd_mode == 0) wait += 1; 

      //書き込みモードでバイトコード受信中の時にタイムアウトさせる
      if (flag_write_mode == 1 && totallen != 0 && write_time <= 2){
	ESP_LOGE(TAG,"-ERR Not the specified number of bytes.\n");
	totallen = 0;
	flag_write_mode = 0;
	write_time = 0;
      }else{
	write_time +=1;
      }
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
  //書き込みモード終了
  printf("Kani-Board, End mrbwrite mode\n");
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

  uint8_t *master = load_spiffs_file("/spiffs/master.mrbc");
  mrbc_create_task(master, 0);

  if (stat("/spiffs/slave.mrbc", &st) == 0) {
    uint8_t *slave = load_spiffs_file("/spiffs/slave.mrbc");
    mrbc_create_task( slave, 0 );
  }

  mrbc_run();
}
