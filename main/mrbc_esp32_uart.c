/*! @file
  @brief
  mruby/c UART class for ESP32
*/

#include "mrbc_esp32_uart.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"

#define BUF_SIZE (1024)

static char* TAG = "UART";

//プロトタイプ宣言
uint8_t * make_output_buffer(mrb_vm *vm, mrb_value v[], int argc,
                             int start_idx, int *ret_bufsiz);

/*! constructor

  uart = I2C.new( 2 )		# id
  uart = I2C.new( 2, baurate:9600 )
  uart = I2C.new( 2, baurate:9600, rts_pin:18 )  #RS485
*/
static void mrbc_esp32_uart_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  //インスタンス作成
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(uart_port_t));

  //initialize を call
  mrbc_instance_call_initialize( vm, v, argc );
  
  vTaskDelay(100 / portTICK_PERIOD_MS);  //wait
}

/*! initializer

*/
static void mrbc_esp32_uart_initialize(mrbc_vm *vm, mrbc_value v[], int argc)
{
  int bps = 15100;  //ボーレート (通信速度)
  uart_port_t uart_num = 2;
  int rtsPin = -1; // RTS for RS485 Half-Duplex Mode manages DE/~RE
  
  // ID が与えられた場合の設定
  if ( GET_INT_ARG(1) >= 0){
    uart_num = GET_INT_ARG(1);
  }

  //オプション解析   
  MRBC_KW_ARG(baudrate, baud, rts_pin);
  if( MRBC_ISNUMERIC(baudrate) ) {
    bps = MRBC_TO_INT(baudrate);
  }
  if( MRBC_ISNUMERIC(baud) ) {
    bps = MRBC_TO_INT(baud);
  }
  if( MRBC_ISNUMERIC(rts_pin) ) {
    rtsPin = MRBC_TO_INT(rts_pin);
  } 

  // instance->data を構造体へのポインタとみなして、値を代入する。
  *((uart_port_t *)(v[0].instance->data)) = uart_num;
  
  ESP_LOGI(TAG, "UART initial");
  ESP_LOGI(TAG, "id:      %d", uart_num);
  ESP_LOGI(TAG, "baurate: %d", bps);

  uart_config_t uart_config = {
    .baud_rate  = bps,
    .data_bits  = UART_DATA_8_BITS,
    .parity     = UART_PARITY_DISABLE,
    .stop_bits  = UART_STOP_BITS_1,
    .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_APB,
  }; 

  // ピン番号を決める
  int txPin = 1, rxPin = 3; 
  if(uart_num == 0){
    txPin = 1;
    rxPin = 3; 
  }
  if(uart_num == 1){
    txPin = 10;
    rxPin = 9;
  }
  if(uart_num == 2){
    txPin = 17;
    rxPin = 16;
  }

  // Install UART driver
  ESP_ERROR_CHECK( uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0) );

  // Configure UART parameters
  ESP_ERROR_CHECK( uart_param_config(uart_num, &uart_config) );

  //RS232C or RS485
  if ( rtsPin > 0 ){
    ESP_LOGI(TAG, "UART MODE: RS485");
    ESP_LOGI(TAG, "rtsPin: %d", rtsPin);

    // Set UART pins (RS485)
    ESP_ERROR_CHECK( uart_set_pin(uart_num, txPin, rxPin, rtsPin, UART_PIN_NO_CHANGE) );

    // Set RS485 half duplex mode
    ESP_ERROR_CHECK( uart_set_mode(uart_num, UART_MODE_RS485_HALF_DUPLEX) );

    // Set read timeout of UART TOUT feature
    ESP_ERROR_CHECK( uart_set_rx_timeout(uart_num, 100 / portTICK_PERIOD_MS) );    

  }else{
    ESP_LOGI(TAG, "UART MODE: RS232C");

    // Set UART pins (RS232C)
    ESP_ERROR_CHECK( uart_set_pin(uart_num, txPin, rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) );
  }
}


/*! メソッド read( bytes )  本体:wrapper for uart_read_bytes

  @param bytes          読み込むデータのバイト数
*/
static void mrbc_esp32_uart_read(mrb_vm* vm, mrb_value* v, int argc)
{
  uart_port_t uart_num = *((uart_port_t *)(v[0].instance->data));  
  int read_bytes = GET_INT_ARG(1);
  
  //データ読み出し
  mrbc_value ret = mrbc_string_new(vm, 0, read_bytes);
  char *buf = mrbc_string_cstr(&ret);
  
  int len = uart_read_bytes( uart_num, buf, read_bytes, 100 / portTICK_PERIOD_MS );
  if (len != read_bytes){
    ESP_LOGE(TAG, "ERROR: Received %u bytes", len);
  }
  buf[read_bytes] = '\0';
  
  SET_RETURN(ret);
  
  /* 確認 */  
  if (len > 0) {
    ESP_LOGI(TAG, "Received %u bytes:", len);
    printf("[ ");
    for (int i = 0; i < len; i++) {
      printf("0x%.2X ", (uint8_t)buf[i]);
    }
    printf("] \n");
  } else {
    ESP_LOGE(TAG, "Read data critical failure.");
  }
	
  /* 配列で値を返す場合
     
  uint8_t* data = (uint8_t*) malloc(BUF_SIZE);
  int len = uart_read_bytes( uart_num, data, read_bytes, 100 / portTICK_PERIOD_MS );
  if (len != read_bytes){
    ESP_LOGE(TAG, "ERROR: Received %u bytes", len);
  }  
  mrbc_value result = mrbc_array_new(vm, len);
  
  // Array インスタンス result に Fixnum インスタンスとして read データをセット
  for ( int x = 0; x < len; ++x ) {
    mrbc_array_set(&result, x, &mrbc_fixnum_value(data[x]));
  }
  
  // Array インスタンス result を本メソッドの返り値としてセット
  SET_RETURN(result);
    
  if (len > 0) {
    ESP_LOGI(TAG, "Received %u bytes:", len);
    printf("[ ");
    for (int i = 0; i < len; i++) {
      printf("0x%.2X ", (uint8_t)data[i]);
    }
    printf("] \n");
  } else {
    ESP_LOGE(TAG, "Read data critical failure.");
  }
  */
}


/*! メソッド gets( break_r:1)

  @param identify_r_with_break  \r単体を改行と判定するかどうか (1: true)
                                trueにすると\r\nでバグが出るので注意
*/
static void mrbc_esp32_uart_gets(mrb_vm* vm, mrb_value* v, int argc)
{
  uart_port_t uart_num = *((uart_port_t *)(v[0].instance->data));  
  int identify_r_with_break = 0;

  //オプション解析   
  MRBC_KW_ARG( break_r );
  if( MRBC_ISNUMERIC( break_r ) ) {
    identify_r_with_break = MRBC_TO_INT( break_r );
  }

  //  ESP_LOGI(TAG, "UART read");
  //  ESP_LOGI(TAG, "id:      %d", uart_num);
  //  ESP_LOGI(TAG, "break_r: %d", identify_r_with_break);
  
  size_t length;
  uint8_t *data;
  int i;
  mrb_value moji;

  uart_get_buffered_data_len(uart_num, &length);
  if(length == 0){
    // printf("no data\n");
    moji = mrbc_string_new_cstr(vm,"");
    SET_RETURN(moji);
    return;
  }
  data = (uint8_t *)malloc(length);
  for (i = 0; i < length - 1; i++){
    uart_read_bytes(uart_num, data + i, 1, 20 / portTICK_PERIOD_MS);
    if (data[i] == '\n' || (identify_r_with_break == 1 && data[i] == '\r'))
      break;
  }
  if(i == length - 1){
    SET_NIL_RETURN();
    return;
  }
  // 改行が'\r\n’のとき、'\r'を消す
  if(data[i - 1] == '\r')
    data[i - 1] = '\0';
  else
    data[i] = '\0';

  //ruby用の文字列を作成
  moji = mrbc_string_new_cstr(vm,(const char *)data);

  //ruby用の文字列を返す
  SET_RETURN(moji);
}


/*! メソッド write(uart_num,data)  本体:wrapper for uart_write_bytes

  @param uart_num UARTポート番号
  @param data	  出力データバッファアドレス
*/
static void mrbc_esp32_uart_write(mrb_vm* vm, mrb_value* v, int argc)
{
  uint8_t *buf = 0;
  int bufsiz = 0;

  uart_port_t uart_num = *((uart_port_t *)(v[0].instance->data));  

  //第一引数は書き込みデータ
  buf = make_output_buffer( vm, v, argc, 1, &bufsiz );
  if (!buf){
    SET_RETURN( mrbc_integer_value( bufsiz ) );
  }

  // 確認
  printf("In C: [ ");
  for (int i = 0; i < bufsiz; i++) {
    printf("0x%.2X ", buf[i]);
  }
  printf("], len:%d \n", bufsiz);
  
  // 書き込み
  if ( uart_write_bytes( uart_num, (const char *) buf, bufsiz ) != bufsiz ){
    ESP_LOGE(TAG, "Send data critical failure.");
  }

  //  uint8_t *data = (uint8_t *)malloc(BUF_SIZE);
  //  data = GET_STRING_ARG(1);
  //  uint8_t length = strlen((const char *)data);
    
  //  if ( uart_write_bytes( uart_num, (const char *)data, strlen((const char *)data) ) != length){
  //    ESP_LOGE(TAG, "Send data critical failure.");
  //  }
}


/*! メソッド flush(uart_num)  本体:wrapper for uart_flush

  @param uart_num UARTポート番号
*/
static void mrbc_esp32_uart_flush(mrb_vm* vm, mrb_value* v, int argc)
{
  uart_port_t uart_num = *((uart_port_t *)(v[0].instance->data));  

  uart_flush( uart_num );
}

/*! メソッド flush_input(uart_num)  本体:wrapper for uart_flush_input

  @param uart_num UARTポート番号
*/
static void mrbc_esp32_uart_flush_input(mrb_vm* vm, mrb_value* v, int argc)
{
  uart_port_t uart_num = *((uart_port_t *)(v[0].instance->data));  
  uart_flush_input(uart_num);
}


static void mrbc_esp32_uart_get_buffered_data_len(mrb_vm* vm, mrb_value* v, int argc)
{
  uart_port_t uart_num = *((uart_port_t *)(v[0].instance->data));  
  size_t length;

  uart_get_buffered_data_len(uart_num, &length);
  SET_INT_RETURN( length );
}


void mrbc_esp32_uart_gem_init(struct VM* vm)
{
  //クラスUART定義
  mrbc_class *uart = mrbc_define_class(vm,"UART",mrbc_class_object);

  //メソッド定義
  mrbc_define_method(vm, uart, "new",             mrbc_esp32_uart_new);
  mrbc_define_method(vm, uart, "initialize",      mrbc_esp32_uart_initialize);
  mrbc_define_method(vm, uart, "read",            mrbc_esp32_uart_read);
  mrbc_define_method(vm, uart, "gets",            mrbc_esp32_uart_gets);
  mrbc_define_method(vm, uart, "write",           mrbc_esp32_uart_write);
  mrbc_define_method(vm, uart, "clear_rx_buffer", mrbc_esp32_uart_flush);
  mrbc_define_method(vm, uart, "clear_tx_buffer", mrbc_esp32_uart_flush_input);
  mrbc_define_method(vm, uart, "clear_tx_buffer", mrbc_esp32_uart_flush_input);
  mrbc_define_method(vm, uart, "bytes_available", mrbc_esp32_uart_get_buffered_data_len);
}

