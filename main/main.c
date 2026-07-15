#include <stdio.h>
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "mrubyc.h"
#include "sdkconfig.h"
#include "esp_rom_crc.h"  

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
#include "mrbc_esp32_http_camera.h"
#include "mrbc_esp32_sleep.h"
#include "mrbc_esp32_spi.h"
#include "mrbc_esp32_lcdspi.h"
#include "mrbc_esp32_sdspi.h"
#include "mrbc_esp32_stdio.h"
#include "mrbc_esp32_dirent.h"
#include "mrbc_esp32_utils.h"
#include "mrbc_esp32_time.h"

static const char *TAG = "mrubyc-esp32";

#define MRUBYC_VERSION_STRING "mruby/c v4.0.0 RITE0400 MRBW1.2"
#define BUF_SIZE (1024)
#define MEMORY_SIZE (1024*70)
#define RD_BUF_SIZE (BUF_SIZE)

static uint8_t memory_pool[MEMORY_SIZE];

//static FILE *g_spiffs_fp = NULL;  
static char g_current_filename[32] = ""; // 現在書き込み中のファイル名
static uint8_t g_check_crc_enable = 0;   // CRC16チェックが有効かどうかのフラグ
static uint16_t g_expected_crc16 = 0;    // 受信した期待値のCRC16

//UART Number
const uart_port_t uart_num = 0;  
#if defined(CONFIG_ESP_CONSOLE_UART_TX_GPIO)
const uint8_t uart_output_tx = CONFIG_ESP_CONSOLE_UART_TX_GPIO;
const uint8_t uart_output_rx = CONFIG_ESP_CONSOLE_UART_RX_GPIO;
#else
const uint8_t uart_output_tx = 0;
const uint8_t uart_output_rx = 0;
#endif


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

/*!
* @brief ESP32内蔵のROM機能を利用した高速CRC16計算
* @note  初期値 0xFFFF (一般的な CCITT Little Endian 方式)
*/
uint16_t calculateCrc16(const uint8_t *data, const size_t size) {
  return esp_rom_crc16_le(0xFFFF, data, size);
}

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
    .source_clk = UART_SCLK_DEFAULT,
  };

  // UARTドライバのインストール
  ESP_ERROR_CHECK( uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0) );

  // UARTパラメータの設定
  ESP_ERROR_CHECK( uart_param_config(uart_num, &uart_config) );
  
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
* @param prefix ファイル名の接頭辞 ("task" または "lib")
*/
void mrbwrite_cmd_write(struct stat *st, uint8_t *flag_write_mode, const char *prefix) {
  g_current_filename[0] = '\0';
  
  // prefix_01 から prefix_99 までループで空きスロットを探す
  for (int i = 1; i <= 99; i++) {
    snprintf(g_current_filename, sizeof(g_current_filename), "/spiffs/%s_%02d.mrbc", prefix, i);
    if (stat(g_current_filename, st) != 0) { 
      // ファイルが存在しない（＝ここを新規保存先にする）
      break;
    }
  }

  if (g_current_filename[0] == '\0' || stat(g_current_filename, st) == 0) {
    ESP_LOGE(TAG, "No more file slots available for %s.", prefix);
    *flag_write_mode = 0;
    printf("-ERR Too many files (%s)\n\n", prefix);
    return;
  }

  // 新規ファイルを開く
  FILE *fp = fopen(g_current_filename, "wb");
  if (fp == NULL) {
    ESP_LOGE(TAG, "Failed to open file for writing (%s)", g_current_filename);
    *flag_write_mode = 0;
    printf("-ERR File open error\n\n");
    return;
  }
  fclose(fp); // 一旦閉じて SPIFFS に空ファイルを確定させる

  *flag_write_mode = 1;
  printf("+OK Write bytecode (%s)\n\n", g_current_filename);
}

/*!
* @brief 開いているファイルへバイナリデータを追記書き込みする（CRC16オプション検証付き）
*/
/*!
* @brief 分割されたバイナリデータを受信し、追記書き込みと終了判定を行う
*/
void write_binary_chunk(int len, uint8_t *data, size_t *totallen, uint8_t *flag_write_mode) {
  
  // 受信データ(len)が残りサイズ(*totallen)を上回った場合、
  // ゴミデータが含まれているため、本当に必要な分だけを書き込む！
  size_t write_len = (len > *totallen) ? *totallen : len;

  // "ab" (追記) モードで毎回ファイルを開く
  FILE *fp = fopen(g_current_filename, "ab");
  if (fp == NULL) {
    ESP_LOGE(TAG, "Failed to open file for appending");
    return;
  }

  // 一括で書き込み、すぐに閉じてフラッシュ（これで長大ファイルも失敗しません）
  size_t written = fwrite(data, 1, write_len, fp);
  fclose(fp); 

  if (written != write_len) {
    ESP_LOGE(TAG, "Failed to write all data. Written: %d/%d", written, write_len);
  }

  // 残りバイト数を減らす（ここで継続判定の準備）
  *totallen -= written;

  // 【継続・終了判定】すべてのバイト数を受信し終わった時の処理
  if (*totallen == 0) {
    
    // --- CRC16 の自動検証ロジック ---
    if (g_check_crc_enable == 1) {
      uint8_t *file_data = load_spiffs_file(g_current_filename);
      if (file_data != NULL) {
        size_t size = get_file_size(g_current_filename);
        uint16_t actual_crc16 = calculateCrc16(file_data, size);
        free(file_data);

        if (actual_crc16 != g_expected_crc16) {
          unlink(g_current_filename); // 破損ファイルなので消去
          printf("-ERR CRC mismatch. Expected: 0x%04X, Actual: 0x%04X\n\n", g_expected_crc16, actual_crc16);
          *flag_write_mode = 0;
          return;
        }
      }
    }

    // 問題なく成功した場合は +DONE を出力してモード終了
    printf("+DONE \n\n");
    *flag_write_mode = 0;
  }
}

/*!
* @brief 書き込まれたバイトコードの消去
*/
void mrbwrite_cmd_clear(struct stat *st) {
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
  printf("  write_lib\n");
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
* @brief 保存されているすべてのプログラム（lib, task）を表示
*/
void mrbwrite_cmd_showprog(struct stat *st) {
  char filename[32];
  int found = 0;
  const char *prefixes[] = {"lib", "task"}; // 両方を検索対象にする
  
  for (int p = 0; p < 2; p++) {
    for (int i = 1; i <= 99; i++) {
      snprintf(filename, sizeof(filename), "/spiffs/%s_%02d.mrbc", prefixes[p], i);
      if (stat(filename, st) == 0) {
        printf("**** %s **** \n\n", filename);
        uint8_t *data = load_spiffs_file(filename);
        if (data != NULL) {
          size_t size = get_file_size(filename);
          ESP_LOG_BUFFER_HEXDUMP(TAG, data, size, ESP_LOG_ERROR);
          free(data);
          found++;
        }
      }
    }
  }
  if (found == 0) printf("-ERR No program files found\n");
  printf("+DONE\n\n");
}

/*!
* @brief 最後に書き込まれたバイトコードのCRC8ハッシュ値を計算して返す
*/
void mrbwrite_cmd_verify()
{
  //Memo:複数ファイルの書き込みを行うようにした場合はファイル名の取得して行う
  uint8_t *data = load_spiffs_file("/spiffs/task_01.mrbc");
  size_t size = get_file_size("/spiffs/task_01.mrbc");
  uint8_t hash = calculateCrc8(data,size);
  printf("+OK %2x\n",hash);
  free(data);
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
  } else if (strncmp(buffer, "write_lib", 9) == 0) {
    //write_lib 
    strcpy(copybuffer, buffer);
    strtok(copybuffer, " ");
    char *ret_bytes = strtok(NULL, " ");
    char *ret_crc16 = strtok(NULL, " ");
    
    if (ret_bytes != NULL) {
      *totallen += atoi(ret_bytes);
      if (ret_crc16 != NULL) {
	g_check_crc_enable = 1;
	g_expected_crc16 = (uint16_t)strtol(ret_crc16, NULL, 16);
      } else {
	g_check_crc_enable = 0;
      }
      mrbwrite_cmd_write(st, flag_write_mode, "lib"); // プレフィックスに "lib" を指定
    } else {
      printf("-ERR Syntax error. Usage: write_lib [bytes] [crc16(hex, opt)]\n\n");
    }    
  } else if (strncmp(buffer, "write", 5) == 0) {
    // write
    strcpy(copybuffer, buffer);
    strtok(copybuffer, " ");
    char *ret_bytes = strtok(NULL, " ");
    char *ret_crc16 = strtok(NULL, " ");

    if (ret_bytes != NULL) {
      *totallen += atoi(ret_bytes);
      if (ret_crc16 != NULL) {
	g_check_crc_enable = 1;
	g_expected_crc16 = (uint16_t)strtol(ret_crc16, NULL, 16);
      } else {
	g_check_crc_enable = 0;
      }
      mrbwrite_cmd_write(st, flag_write_mode, "task"); // プレフィックスに "task" を指定
    } else {
      printf("-ERR Syntax error. Usage: write [bytes] [crc16(hex, opt)]\n\n");
    }
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
  }else if(buffer[0] == 0x0d && buffer[1] == 0x0a){
    printf("+OK mruby/c \n\n");
//  }else{
//    printf("-ERR Illegal command.\n\n");
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
  uint8_t data[BUF_SIZE];
  uint8_t wait = 0;
  uint32_t write_timeout = 0;
  uint8_t flag_cmd_mode = 0;
  uint8_t flag_write_mode = 0;
  uint8_t ifile = 0;
  char buffer[BUF_SIZE];
  struct stat st;
  size_t totallen = 0;

  // SPIFFS 初期化
  init_spiffs();
  vTaskDelay(100 / portTICK_PERIOD_MS);
  
  // UART0 初期化
  init_uart();
  vTaskDelay(100 / portTICK_PERIOD_MS);
  
  //************************************
  // mrbcwrite モード開始
  //************************************
  printf("\nKani-Board, Please push Enter key to mrbwrite mode\n\n");

  // clear buffer
  uart_wait_tx_done(uart_num, pdMS_TO_TICKS(100));
  uart_flush_input(uart_num);

  while (wait < 20) {
    
    // バイト数の取得. 100ms の待ち
    int len = uart_read_bytes(uart_num, data, BUF_SIZE, 100 / portTICK_PERIOD_MS);
    
    // 取得したバイト数が正か否かで場合分け
    if (len > 0) {
      wait = 0;  // waiting の変数のクリア
      write_timeout = 0;
      
      if (flag_write_mode == 1) {
	// バイナリ書き込みモード
        write_binary_chunk(len, data, &totallen, &flag_write_mode);
        
      } else {
        // コマンドモード（通常時：文字・コマンド受信の処理）
        int start_pos = 0;
        
        // 先頭にある改行(0x0d, 0x0a)をスキップ
        while (start_pos < len && (data[start_pos] == 0x0d || data[start_pos] == 0x0a)) {
          start_pos++;
        }

        // 文字型に変換 
        int idx = 0;
        for (int i = start_pos; i < len && idx < (BUF_SIZE - 1); i++) {
          buffer[idx++] = data[i];
        }
        buffer[idx] = '\0'; // 末尾にヌル文字を入れる
        
        if (flag_cmd_mode == 0) {
          // Enter (CR+LF) が打鍵された場合はフラグを立てる
          if (data[0] == 0x0d && data[1] == 0x0a) {         
            printf("+OK mruby/c \n\n");
	    
            uart_wait_tx_done(uart_num, pdMS_TO_TICKS(100));
            uart_flush_input(uart_num);
            flag_cmd_mode = 1;
          }
        } else {
          // コマンドモードに入っている場合
          int cmd_state = mrbwrite_cmd_mode(
            &st,
            &ifile,
            &flag_write_mode,
            buffer,
            len,
            data,
            &totallen
          );

          uart_wait_tx_done(uart_num, pdMS_TO_TICKS(100));
          uart_flush_input(uart_num);
          
          if (cmd_state == 1) break;
        }
      } // else (コマンドモード処理の終わり)
      
    } else {
      // コマンドモードでなければカウントアップ (タイムアウト監視)
      if (flag_cmd_mode == 0) {
        wait += 1; 
      }

      // 書き込みモード中なのに 1秒待ってもデータが来ない(= len <= 0)なら中断
      if (flag_write_mode == 1 && totallen != 0) {
	write_timeout += 1;

	// 30回連続 (100ms * 30回 = 3秒間) データが途切れたらエラーとする
        if (write_timeout >= 30) {
          ESP_LOGE(TAG, "-ERR Timeout: Data transmission interrupted.\n");
    
          totallen = 0;
          flag_write_mode = 0;
          write_timeout = 0;
	}
      }      
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
  // 書き込みモード終了
  printf("\nKani-Board, End mrbwrite mode\n");
  printf("Kani-Board, mruby/c v4.0.0 start\n");
  
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
  if (!(uart_output_tx == 17 && uart_output_rx == 16)){
    ESP_LOGI(TAG, "start UART (C)\n");
    mrbc_esp32_uart_gem_init(0);
  }
  ESP_LOGI(TAG, "start WiFi (C) \n");
  mrbc_esp32_wifi_gem_init(0);
  mrbc_esp32_sntp_gem_init(0);
  mrbc_esp32_httpclient_gem_init(0);
  ESP_LOGI(TAG, "start Time (C) \n");  
  mrbc_esp32_time_gem_init(0);
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
  ESP_LOGI(TAG, "start HTTP_CAMERA (C) \n");
  mrbc_esp32_httpcamera_gem_init(0);

  // Ruby 側のクラス・メソッド定義
  extern const uint8_t myclass_bytecode[];
  mrbc_run_mrblib(myclass_bytecode);

  // SPIFFS 上の lib_%02d.mrbc を自動検索して mrblib に登録
  ESP_LOGI(TAG, "SPIFFS mode: Loading mruby/c libraries...");
  char lib_filename[32];
  int lib_count = 0;
  
  for (int i = 1; i <= 99; i++) {
    snprintf(lib_filename, sizeof(lib_filename), "/spiffs/lib_%02d.mrbc", i);
    if (stat(lib_filename, &st) == 0) {
      uint8_t *lib_bytecode = load_spiffs_file(lib_filename);
      if (lib_bytecode != NULL) {
        mrbc_run_mrblib(lib_bytecode);
        lib_count++;
        ESP_LOGI(TAG, "Loaded mrblib from file: %s", lib_filename);
      }
    }
  }
  if (lib_count > 0) {
    ESP_LOGI(TAG, "Total %d libraries registered successfully!", lib_count);
  }
  
  // SPIFFS 上の task_%02d.mrbc を自動検索して task に登録
  ESP_LOGI(TAG, "SPIFFS mode: Loading tasks...");
  char filename[32];
  int task_count = 0;
  
  for (int i = 1; i <= 99; i++) {
    snprintf(filename, sizeof(filename), "/spiffs/task_%02d.mrbc", i);
    if (stat(filename, &st) == 0) {
      uint8_t *bytecode = load_spiffs_file(filename);
      if (bytecode != NULL) {
        mrbc_create_task(bytecode, 0);
        task_count++;
        ESP_LOGI(TAG, "Created task from file: %s", filename);
      }
    }
  }
  
  if (task_count == 0) {
    ESP_LOGW(TAG, "No bytecode files found in SPIFFS. Only built-in tasks will run.");
  } else {
    ESP_LOGI(TAG, "Total %d tasks started successfully!", task_count);
  }
  
  mrbc_run();
  
}
