/*! @file
  @brief
  mruby/c SPI class for ESP32
*/

#include "mrbc_esp32_ili934x.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "font.h"

static char* TAG = "SPILCD";

//spi_device_handle_t spidev;
uint16_t gpio_dc_pin = -1;
int spi_cs_pin   = -1;

/*
// ITOC 東さんのソースコードをコピー

  @param vm     Pointer to vm
  @param v      argments
  @param argc   num of arguments
  @param start_idx  Argument parsing start position.
  @param ret_bufsiz allocated buffer size.
  @return       pointer to allocated buffer, or NULL is error.
*/

int toc(int c1, int c2, int c3) {
  return ((c1 & 0xf8) << 8) | ((c2 & 0xfc) << 3) | (c3 >> 3);
}


uint8_t * make_output_buffer3(mrb_vm *vm, mrb_value v[], int argc,
                             int start_idx, int *ret_bufsiz)
{
  uint8_t *ret = 0;

  // calc temporary buffer size.
  int bufsiz = 0;
  for( int i = start_idx; i <= argc; i++ ) {
    switch( v[i].tt ) {
    case MRBC_TT_INTEGER:
      bufsiz += 1;
      break;

    case MRBC_TT_STRING:
      bufsiz += mrbc_string_size(&v[i]);
      break;

    case MRBC_TT_ARRAY:
      bufsiz += mrbc_array_size(&v[i]);
      break;

    default:
      goto ERROR_PARAM;
    }
  }
  *ret_bufsiz = bufsiz;
  if( bufsiz == 0 ) goto ERROR_PARAM;

  // alloc buffer and copy data
  ret = mrbc_alloc(vm, bufsiz);
  uint8_t *pbuf = ret;
  for( int i = start_idx; i <= argc; i++ ) {
    switch( v[i].tt ) {
    case MRBC_TT_INTEGER:
      *pbuf++ = mrbc_integer(v[i]);
      break;

    case MRBC_TT_STRING:
      memcpy( pbuf, mrbc_string_cstr(&v[i]), mrbc_string_size(&v[i]) );
      pbuf += mrbc_string_size(&v[i]);
      break;

    case MRBC_TT_ARRAY: {
      for( int j = 0; j < mrbc_array_size(&v[i]); j++ ) {
        mrbc_value val = mrbc_array_get(&v[i], j);
        if( val.tt != MRBC_TT_INTEGER ) goto ERROR_PARAM;
        *pbuf++ = mrbc_integer(val);
      }
    } break;

    default:
      //
    }
  }
  return ret;

 ERROR_PARAM:
  mrbc_raise(vm, MRBC_CLASS(ArgumentError), "Output parameter error.");
  if( ret != 0 ) {
    mrbc_free( vm, ret );
  }

  return 0;
}


// ILI934X library

static void spi_write_byte(spi_device_handle_t spi_handle, const uint8_t* Data, size_t DataLength)
{
  spi_transaction_t SPITransaction;
  
  //  printf("DataLength: %d\n", DataLength);
  
  if ( DataLength > 0 ) {
    memset( &SPITransaction, 0, sizeof( spi_transaction_t ) );
    SPITransaction.length = DataLength * 8;
    SPITransaction.tx_buffer = Data;
    ESP_ERROR_CHECK( spi_device_transmit( spi_handle, &SPITransaction ) );
  }
}


uint8_t byte[1024];
static void ili934x_write_color(spi_device_handle_t spi_handle, int color, int size)
{
  int index = 0;
  for(int i = 0;i < size; i++) {
    byte[index++] = (color >> 8) & 0xFF;
    byte[index++] = color & 0xFF;
  }
  gpio_set_level(gpio_dc_pin, 1);
  spi_write_byte(spi_handle, byte, size * 2);
}

static void ili934x_write_command(spi_device_handle_t spi_handle, uint8_t command)
{
  gpio_set_level(gpio_dc_pin, 0);
  spi_write_byte(spi_handle, &command, 1);
}

static void ili934x_write_data(spi_device_handle_t spi_handle, uint8_t data)
{
  gpio_set_level(gpio_dc_pin, 1);
  spi_write_byte(spi_handle, &data, 1);
}

static void ili934x_write_addr(spi_device_handle_t spi_handle, int addr1, int addr2)
{
  byte[0] = (addr1 >> 8) & 0xFF;
  byte[1] = addr1 & 0xFF;
  byte[2] = (addr2 >> 8) & 0xFF;
  byte[3] = addr2 & 0xFF;
  gpio_set_level(gpio_dc_pin, 1);
  spi_write_byte(spi_handle, byte, 4);
}

static void ili934x_draw_pixel( spi_device_handle_t spi_handle, int x, int y, int color)
{  
  ili934x_write_command(spi_handle, 0x2a);
  ili934x_write_addr(spi_handle, x, x);
  ili934x_write_command(spi_handle, 0x2b);
  ili934x_write_addr(spi_handle, y, y);
  ili934x_write_command(spi_handle, 0x2c);
  byte[0] = (color >> 8) & 0xFF;
  byte[1] = color & 0xFF;
  gpio_set_level(gpio_dc_pin, 1);
  spi_write_byte(spi_handle, byte, 2);
}


/*! constructor
  
  spi = SPI.new( )	
  spi = SPI.new( mosi:23, miso:18, clk:14, cs:27 )
  
  @param   mosi_pin MOSI Pin Number
           miso MISO Pin Number
           clk SPI Clock Pin Number
           cs   CS Pin Number
*/
static void mrbc_esp32_ili934x_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  //オプション解析
  MRBC_KW_ARG(cs_pin, dc_pin);
  if( MRBC_ISNUMERIC(cs_pin) ) {
    spi_cs_pin = MRBC_TO_INT(cs_pin);
  }
  if( MRBC_ISNUMERIC(dc_pin) ) {
    gpio_dc_pin = MRBC_TO_INT(dc_pin);
  }

  //インスタンス作成
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(spi_device_handle_t));

  //initialize をcall
  mrbc_instance_call_initialize( vm, v, argc );
  
  vTaskDelay(100 / portTICK_PERIOD_MS);  //wait
}


/*! initializer

*/
static void mrbc_esp32_ili934x_initialize(mrbc_vm *vm, mrbc_value v[], int argc)
{
  //引数処理．第一引数は spi オブジェクト
  spi_host_device_t spi_unit = *((spi_host_device_t *)(v[1].instance->data));
  
  spi_device_interface_config_t dev_cfg = {
    .command_bits = 0,
    .mode = 0,                        //SPI mode 0
    .clock_speed_hz = SPI_MASTER_FREQ_40M,
    .spics_io_num = spi_cs_pin,
    .queue_size = 7,
    .flags = SPI_DEVICE_NO_DUMMY,
  };
  
  //デバイスの追加
  spi_device_handle_t spi_handle;
  ESP_ERROR_CHECK(spi_bus_add_device(spi_unit, &dev_cfg, &spi_handle));
  
  // instance->data を構造体へのポインタとみなして、値を代入する。
  *((spi_device_handle_t *)(v[0].instance->data)) = spi_handle;
  //  spidev = spi_handle;
  
  //dc_pin の初期化
  ESP_ERROR_CHECK( gpio_reset_pin(gpio_dc_pin) );
  ESP_ERROR_CHECK( gpio_set_direction(gpio_dc_pin, GPIO_MODE_OUTPUT) );

  ESP_LOGI(TAG, "ILI934X initial");
  ESP_LOGI(TAG, "unit:%d", spi_unit);
  ESP_LOGI(TAG, "dc:  %d", gpio_dc_pin);
  ESP_LOGI(TAG, "cs:  %d", spi_cs_pin);

  ESP_LOGI(TAG, "ILI934X initial setup end");
}

/*! Method write_byte(data)
    @param data
 */
static void
mrbc_esp32_ili934x_write(mrb_vm* vm, mrb_value* v, int argc)
{
  uint8_t *buf = 0;
  int bufsiz = 0;
  uint8_t Data[1024];
  
  //第一引数は書き込みデータ
  buf = make_output_buffer3( vm, v, argc, 1, &bufsiz );
  if (!buf){
    SET_RETURN( mrbc_integer_value(bufsiz) );
  }

  for (int i = 0; i < bufsiz ; i++){
    Data[0] = (uint8_t) buf[i];
  }

  // start SPI communication
  spi_device_handle_t spi_handle = *((spi_device_handle_t *)(v[0].instance->data));
  spi_write_byte( spi_handle, Data, bufsiz);

    /*
  spi_transaction_t transaction;
  memset(&transaction, 0, sizeof( spi_transaction_t ));
  transaction.length = bufsiz * 8;
  transaction.tx_buffer = Data;
  ESP_ERROR_CHECK( spi_device_transmit(spi_handle, &transaction) );
    */
    
  //動的に確保したメモリの解放
  mrbc_free( vm, buf );
  
  //バッファのサイズを戻す
  SET_RETURN( mrbc_integer_value(bufsiz) );
}


/*! Method __draw_rectangle(x1, y1, x2, y2, color)
    @param x1 point 1 x
           y1 point 1 y
           x2 point 2 x
           y2 point 2 y
           color 16bit color 
 */
static void
mrbc_esp32_ili934x_draw_fillrectangle(mrb_vm* vm, mrb_value* v, int argc)
{
  spi_device_handle_t spi_handle = *((spi_device_handle_t *)(v[0].instance->data));
  
  int x1 = GET_INT_ARG(1);
  int y1 = GET_INT_ARG(2);
  int x2 = GET_INT_ARG(3);
  int y2 = GET_INT_ARG(4);
  int color = GET_INT_ARG(5);

  ili934x_write_command(spi_handle, 0x2a);
  ili934x_write_addr(spi_handle, x1, x2);
  ili934x_write_command(spi_handle, 0x2b);
  ili934x_write_addr(spi_handle, y1, y2);
  ili934x_write_command(spi_handle, 0x2c);
  int size = y2 - y1 + 1;
  for (int i = x1; i <= x2; i++)
    ili934x_write_color(spi_handle, color, size);
}

/*! Method __draw_line(x1, y1, x2, y2, color)
    @param x1 point 1 x
           y1 point 1 y
           x2 point 2 x
           y2 point 2 y
           color 16bit color 
 */
static void
mrbc_esp32_ili934x_draw_line(mrb_vm* vm, mrb_value* v, int argc)
{
  spi_device_handle_t spi_handle = *((spi_device_handle_t *)(v[0].instance->data));
  int x1 = GET_INT_ARG(1);
  int y1 = GET_INT_ARG(2);
  int x2 = GET_INT_ARG(3);
  int y2 = GET_INT_ARG(4);
  int color = GET_INT_ARG(5);
  
  int i, dx, dy ,sx, sy, E;

  dx = (x2 > x1) ? x2 - x1 : x1 - x2;
  dy = (y2 > y1) ? y2 - y1 : y1 - y2;
  
  sx = (x2 > x1) ? 1 : -1;
  sy = (y2 > y1) ? 1 : -1;
  
  if (dx > dy)
    {
      E = -dx;
      for (i = 0; i <= dx; i++)
        {
	  ili934x_draw_pixel(spi_handle, x1, y1, color);
	  x1 += sx;
	  E += 2 * dy;
	  if (E >= 0)
            {
	      y1 += sy;
	      E -= 2 * dx;
            }
        }
    }
  else
    {
      E = -dy;
      for (i = 0; i <= dy; i++)
        {
	  ili934x_draw_pixel(spi_handle, x1, y1, color);
	  y1 += sy;
	  E += 2 * dx;
	  if (E >= 0)
            {
	      x1 += sx;
	      E -= 2 * dy;
            }
        }
    }  
}

/*! Method __draw_circle(x, y, r, color)
    @param x0 center x
           y0 center y
           r radius
           color 16bit color 
 */
static void
mrbc_esp32_ili934x_draw_circle(mrb_vm* vm, mrb_value* v, int argc)
{
  spi_device_handle_t spi_handle = *((spi_device_handle_t *)(v[0].instance->data));
  int x0 = GET_INT_ARG(1);
  int y0 = GET_INT_ARG(2);
  int r = GET_INT_ARG(3);
  int color = GET_INT_ARG(4);
  int x = 0, y = -r, err = 2 - 2 * r, old_err;
  do{
    ili934x_draw_pixel(spi_handle, x0 - x, y0 + y, color);
    ili934x_draw_pixel(spi_handle, x0 - y, y0 - x, color);
    ili934x_draw_pixel(spi_handle, x0 + x, y0 - y, color);
    ili934x_draw_pixel(spi_handle, x0 + y, y0 + x, color);
    if ((old_err = err) <= x)
      err += ++x * 2 + 1;
    if (old_err > y || err > x)
      err += ++y * 2 + 1;
  } while (y < 0);
}


/*! Method __draw_char(x, y, color)
    @param x point x
           y point y
           c character
           color 16bit color 
           height height pixel size of drawing font
 */
static void
mrbc_esp32_ili934x_draw_char(mrb_vm* vm, mrb_value* v, int argc)
{
    spi_device_handle_t spi_handle = *((spi_device_handle_t *)(v[0].instance->data));
    int x = GET_INT_ARG(1);
    int y = GET_INT_ARG(2);
    int c = GET_INT_ARG(3);
    int color = GET_INT_ARG(4);
    int height = GET_INT_ARG(5);

    int index = get(c);
    int ptr = pointers[index];
    if (index == -1) {
        SET_INT_RETURN(HEIGHT);
        return;
    }
    int Width = bitmaps[ptr] >> 12;
    int width = Width * height / HEIGHT;
    int bit = 15 - 4;
    int pre_pos = 0;
    int n = !!(bitmaps[ptr] & 1 << bit);
    for (int dy = 0; dy < height; dy++)
    {
        int i = dy * HEIGHT / height;
        for (int dx = 0; dx < width; dx++)
        {
            int j = dx * Width / width;
            int pos = i * Width + j;
            if (pos != pre_pos)
            {
                bit -= (pos - pre_pos);
                if(bit < 0) {
                    bit += 16;
                    ptr++;
                }
                if(bit >= 16) {
                    bit -= 16;
                    ptr--;
                }
                n = !!(bitmaps[ptr] & 1 << bit);
            }
            if(n)
	      ili934x_draw_pixel(spi_handle, x + dx, y + dy, color);
            pre_pos = pos;
        }
    }
    SET_INT_RETURN(width);
}

/*! Register SPI Class.p
 */
void
mrbc_esp32_ili934x_gem_init(struct VM* vm)
{
  //クラスUART定義
  mrbc_class *spi = mrbc_define_class(vm, "LCDSPI", mrbc_class_object);

  // 各メソッド定義
  mrbc_define_method(vm, spi, "new",               mrbc_esp32_ili934x_new);
  mrbc_define_method(vm, spi, "initialize",        mrbc_esp32_ili934x_initialize);
  mrbc_define_method(vm, spi, "write",             mrbc_esp32_ili934x_write);
  mrbc_define_method(vm, spi, "fillrectangle",mrbc_esp32_ili934x_draw_fillrectangle);
  mrbc_define_method(vm, spi, "circle",       mrbc_esp32_ili934x_draw_circle);
  mrbc_define_method(vm, spi, "line",              mrbc_esp32_ili934x_draw_line);
  mrbc_define_method(vm, spi, "char",         mrbc_esp32_ili934x_draw_char);
}

