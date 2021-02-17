/*! @file
  @brief
  mruby/c UART class for ESP32
  本クラスはインスタンスを生成せず利用する
*/

#include "mrbc_esp32_uart.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include <string.h>
#include <stdio.h>

#define BUF_SIZE (1024)

static struct RClass* mrbc_class_esp32_uart;
static int unreferenced;

/*! メソッド nop(count) 本体 : nop (no operation)
  @param count nop の長さ、ダミー処理ループの回数
*/
static void
mrbc_nop(mrb_vm* vm, mrb_value* v, int argc)
{
  // NO OPERATION
  int max = GET_INT_ARG(1);
  for ( int i = 0 ; i < max ; ++i ) {
    unreferenced += 1;
  }
}
/*! メソッド set_config(uart_num,bps)  本体:wrapper for uart_param_config,
  @param uart_num UARTポート番号 
  @param bps ボーレート(通信速度) 
*/
static void mrbc_esp32_uart_config(mrb_vm* vm, mrb_value* v, int argc)
{
  uart_port_t uart_num = GET_INT_ARG(1);
  int bps = GET_INT_ARG(2);
  
  uart_config_t uart_config = {
	.baud_rate  = bps,
	.data_bits  = UART_DATA_8_BITS,
	.parity     = UART_PARITY_DISABLE,
	.stop_bits  = UART_STOP_BITS_1,
	.flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
	.source_clk = UART_SCLK_APB,
  };
  uart_param_config(uart_num,&uart_config);
}

/*! メソッド driver_install(uart_num)  本体:wrapper for uart_driver_install
  @param uart_num UARTポート番号

*/
static void mrbc_esp32_uart_driver_install(mrb_vm* vm,mrb_value* v, int argc)
{

  uart_port_t uart_num = GET_INT_ARG(1); 
  uart_driver_install(uart_num,BUF_SIZE * 2, 0, 0, NULL, 0);
  if(uart_is_driver_installed(uart_num) == 1) printf("driver ok\n");

}

/*! メソッド set_pin(uart_num)  本体:wrapper for urat_set_pin
  @param uart_num UARTポート番号
*/
static void mrbc_esp32_uart_set_pin(mrb_vm* vm, mrb_value* v, int argc)
{
  uart_port_t uart_num = GET_INT_ARG(1);
  int txPin=1,rxPin=3; 

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
  uart_set_pin(uart_num,txPin,rxPin,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE);
}

/*! メソッド read(uart_num)  本体:wrapper for uart_read_bytes
  @param uart_num UARTポート番号

*/
static void mrbc_esp32_uart_read_bytes(mrb_vm* vm, mrb_value* v, int argc)
{
  uart_port_t uart_num = GET_INT_ARG(1);
  uint8_t *data = (uint8_t *)malloc(BUF_SIZE);
  uart_read_bytes(uart_num,data,BUF_SIZE,20 / portTICK_RATE_MS);
 
  
  mrb_value moji;

  if(strstr( (const char *)data,"Rc") != 0){
    
    //ruby用の文字列を作成
    moji = mrbc_string_new_cstr(vm,(const char *)data);

    //ruby用の文字列を返す
    SET_RETURN(moji);

	
  }else{
   // printf("no data\n");
    moji = mrbc_string_new_cstr(vm,"");
    SET_RETURN(moji);
  }
 
}

/*! メソッド write(uart_num,data)  本体:wrapper for uart_write_bytes
  @param uart_num UARTポート番号
  @param data	  出力データバッファアドレス
*/
static void mrbc_esp32_uart_write_bytes(mrb_vm* vm, mrb_value* v, int argc)
{
  uart_port_t uart_num = GET_INT_ARG(1);

  uint8_t *data = (uint8_t *)malloc(BUF_SIZE);
  data = GET_STRING_ARG(2);

  uart_write_bytes( uart_num,(const char *)data, strlen((const char *)data) );
}


void mrbc_mruby_esp32_uart_gem_init(struct VM* vm)
{
/*
UART.config(uart_num,bps)
UART.driver_install(uart_num)
UART.set_pin(uart_num)
UART.read(uart_num)
UART.write(uart_num,data,)
*/
  //クラスUART定義
  mrbc_class_esp32_uart = mrbc_define_class(vm,"UART",mrbc_class_object);

  // 各メソッド定義（mruby/c ではインスタンスメソッドをクラスメソッドとしても呼び出し可能）
  mrbc_define_method(vm,mrbc_class_esp32_uart,"config",mrbc_esp32_uart_config);
  mrbc_define_method(vm,mrbc_class_esp32_uart,"driver_install",mrbc_esp32_uart_driver_install);
  mrbc_define_method(vm,mrbc_class_esp32_uart,"set_pin",mrbc_esp32_uart_set_pin);
  mrbc_define_method(vm,mrbc_class_esp32_uart,"read_bytes",mrbc_esp32_uart_read_bytes);
  mrbc_define_method(vm,mrbc_class_esp32_uart,"write_bytes",mrbc_esp32_uart_write_bytes);
  mrbc_define_method(vm,mrbc_class_esp32_uart,"nop",mrbc_nop);

}

