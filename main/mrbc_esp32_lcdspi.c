/*! @file
  @brief
  mruby/c SPI class for ESP32
*/

#include "mrbc_esp32_lcdspi.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "mrbc_esp32_lcdspi_font.h"

#define XMIN 0
#define YMIN 0
#define XMAX 320
#define YMAX 240

static char* TAG = "SPILCD";

//spi_device_handle_t spidev;
uint16_t gpio_dc_pin = -1;


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

static void ili934x_draw_line(spi_device_handle_t spi_handle, int x1, int y1, int x2, int y2, int color)
{
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


static int ili934x_draw_char(spi_device_handle_t spi_handle, int x, int y, char c, int color, int height)
{
  int index = get(c);
  int ptr = pointers[index];
  if (index == -1) {
    ESP_LOGE(TAG, "char is not defined");
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
  return width;
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
  int spi_cs_pin   = -1;
  uint16_t gpio_bl_pin  = -1;
  uint16_t gpio_rst_pin = -1;

  //オプション解析
  MRBC_KW_ARG(cs_pin, dc_pin, rst_pin, bl_pin);
  if( MRBC_ISNUMERIC(cs_pin) ) {
    spi_cs_pin = MRBC_TO_INT(cs_pin);
  }
  if( MRBC_ISNUMERIC(dc_pin) ) {
    gpio_dc_pin = MRBC_TO_INT(dc_pin);
  }
  if( MRBC_ISNUMERIC(rst_pin) ) {
    gpio_rst_pin = MRBC_TO_INT(rst_pin);
  }
  if( MRBC_ISNUMERIC(bl_pin) ) {
    gpio_bl_pin = MRBC_TO_INT(bl_pin);
  }

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

  ESP_LOGD(TAG, "ILI934X initial");
  ESP_LOGD(TAG, "unit:%d", spi_unit);
  ESP_LOGD(TAG, "dc:  %d", gpio_dc_pin);
  ESP_LOGD(TAG, "cs:  %d", spi_cs_pin);
  ESP_LOGD(TAG, "rst: %d", gpio_rst_pin);
  ESP_LOGD(TAG, "bl:  %d", gpio_bl_pin);
  
  //dc_pin の初期化
  ESP_ERROR_CHECK( gpio_reset_pin(gpio_dc_pin) );
  ESP_ERROR_CHECK( gpio_set_direction(gpio_dc_pin, GPIO_MODE_OUTPUT) );

  //rst_pin の初期化
  ESP_ERROR_CHECK( gpio_reset_pin(gpio_rst_pin) );
  ESP_ERROR_CHECK( gpio_set_direction(gpio_rst_pin, GPIO_MODE_OUTPUT) );
  gpio_set_level(gpio_rst_pin, 1);
  
  //bl_pin の初期化
  ESP_ERROR_CHECK( gpio_reset_pin(gpio_bl_pin) );
  ESP_ERROR_CHECK( gpio_set_direction(gpio_bl_pin, GPIO_MODE_OUTPUT) );
  gpio_set_level(gpio_bl_pin, 1);

  //初期化
  ili934x_write_command(spi_handle, 0xc0);
  ili934x_write_data(spi_handle, 0x23);

  ili934x_write_command(spi_handle, 0xc1);
  ili934x_write_data(spi_handle, 0x10);
    
  ili934x_write_command(spi_handle, 0xc5);
  ili934x_write_data(spi_handle, 0x3e);
  ili934x_write_data(spi_handle, 0x28);
    
  ili934x_write_command(spi_handle, 0xc7);
  ili934x_write_data(spi_handle, 0x86);
    
  ili934x_write_command(spi_handle, 0x36);
  ili934x_write_data(spi_handle, 0x08);  //Memory Access Control
    
  ili934x_write_command(spi_handle, 0x3a);
  ili934x_write_data(spi_handle, 0x55); //Pixel Format
    
  ili934x_write_command(spi_handle, 0x21);
    
  ili934x_write_command(spi_handle, 0xb1);
  ili934x_write_data(spi_handle, 0x00);
  ili934x_write_data(spi_handle, 0x18);
    
  ili934x_write_command(spi_handle, 0xb6);//Display Function Control
  ili934x_write_data(spi_handle, 0x08); 
  ili934x_write_data(spi_handle, 0xa2); 
  ili934x_write_data(spi_handle, 0x27); 
  ili934x_write_data(spi_handle, 0x00); 
    
  ili934x_write_command(spi_handle, 0x26);
  ili934x_write_data(spi_handle, 0x01);
    
  ili934x_write_command(spi_handle, 0xE0); //Positive Gamma Correction
  ili934x_write_data(spi_handle, 0x0F);
  ili934x_write_data(spi_handle, 0x31);
  ili934x_write_data(spi_handle, 0x2B);
  ili934x_write_data(spi_handle, 0x0C);
  ili934x_write_data(spi_handle, 0x0E);
  ili934x_write_data(spi_handle, 0x08);
  ili934x_write_data(spi_handle, 0x4E);
  ili934x_write_data(spi_handle, 0xF1);
  ili934x_write_data(spi_handle, 0x37);
  ili934x_write_data(spi_handle, 0x07);
  ili934x_write_data(spi_handle, 0x10);
  ili934x_write_data(spi_handle, 0x03);
  ili934x_write_data(spi_handle, 0x0E);
  ili934x_write_data(spi_handle, 0x09);
  ili934x_write_data(spi_handle, 0x00);
    
  ili934x_write_command(spi_handle, 0xE1); //Negative Gamma Correction
  ili934x_write_data(spi_handle, 0x00);
  ili934x_write_data(spi_handle, 0x0E);
  ili934x_write_data(spi_handle, 0x14);
  ili934x_write_data(spi_handle, 0x03);
  ili934x_write_data(spi_handle, 0x11);
  ili934x_write_data(spi_handle, 0x07);
  ili934x_write_data(spi_handle, 0x31);
  ili934x_write_data(spi_handle, 0xC1);
  ili934x_write_data(spi_handle, 0x48);
  ili934x_write_data(spi_handle, 0x08);
  ili934x_write_data(spi_handle, 0x0F);
  ili934x_write_data(spi_handle, 0x0C);
  ili934x_write_data(spi_handle, 0x31);
  ili934x_write_data(spi_handle, 0x36);
  ili934x_write_data(spi_handle, 0x0F);
  
  ili934x_write_command(spi_handle, 0x11); //Sleep Out

  vTaskDelay(120 / portTICK_PERIOD_MS);  //wait

  ili934x_write_command(spi_handle, 0x29); //Display ON
  
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

  int _x1 = XMIN;
  int _y1 = XMIN;
  int _x2 = XMAX;
  int _y2 = YMAX;
  int _color = 0;
  
  //オプション解析
  MRBC_KW_ARG(x1, y1, x2, y2, color);
  if( MRBC_ISNUMERIC(x1) ) {
    _x1 = MRBC_TO_INT(x1);
  }
  if( MRBC_ISNUMERIC(y1) ) {
    _y1 = MRBC_TO_INT(y1);
  }
  if( MRBC_ISNUMERIC(x2) ) {
    _x2 = MRBC_TO_INT(x2);
  }
  if( MRBC_ISNUMERIC(y2) ) {
    _y2 = MRBC_TO_INT(y2);
  }
  if( MRBC_ISNUMERIC(color) ) {
    _color = MRBC_TO_INT(color);
  }

  ili934x_write_command(spi_handle, 0x2a);
  ili934x_write_addr(spi_handle, _x1, _x2);
  ili934x_write_command(spi_handle, 0x2b);
  ili934x_write_addr(spi_handle, _y1, _y2);
  ili934x_write_command(spi_handle, 0x2c);
  int size = _y2 - _y1 + 1;
  for (int i = _x1; i <= _x2; i++)
    ili934x_write_color(spi_handle, _color, size);
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

  int _x1 = XMIN;
  int _y1 = XMIN;
  int _x2 = XMAX;
  int _y2 = YMAX;
  int _color = 0;
  
  //オプション解析
  MRBC_KW_ARG(x1, y1, x2, y2, color);
  if( MRBC_ISNUMERIC(x1) ) {
    _x1 = MRBC_TO_INT(x1);
  }
  if( MRBC_ISNUMERIC(y1) ) {
    _y1 = MRBC_TO_INT(y1);
  }
  if( MRBC_ISNUMERIC(x2) ) {
    _x2 = MRBC_TO_INT(x2);
  }
  if( MRBC_ISNUMERIC(y2) ) {
    _y2 = MRBC_TO_INT(y2);
  }
  if( MRBC_ISNUMERIC(color) ) {
    _color = MRBC_TO_INT(color);
  }

  int i, dx, dy ,sx, sy, E;

  dx = (_x2 > _x1) ? _x2 - _x1 : _x1 - _x2;
  dy = (_y2 > _y1) ? _y2 - _y1 : _y1 - _y2;
  
  sx = (_x2 > _x1) ? 1 : -1;
  sy = (_y2 > _y1) ? 1 : -1;
  
  if (dx > dy)
    {
      E = -dx;
      for (i = 0; i <= dx; i++)
        {
	  ili934x_draw_pixel(spi_handle, _x1, _y1, _color);
	  _x1 += sx;
	  E += 2 * dy;
	  if (E >= 0)
            {
	      _y1 += sy;
	      E -= 2 * dx;
            }
        }
    }
  else
    {
      E = -dy;
      for (i = 0; i <= dy; i++)
        {
	  ili934x_draw_pixel(spi_handle, _x1, _y1, _color);
	  _y1 += sy;
	  E += 2 * dx;
	  if (E >= 0)
            {
	      _x1 += sx;
	      E -= 2 * dy;
            }
        }
    }  
}


/*! Method __draw_fillcircle(x, y, r, color)
    @param x0 center x
           y0 center y
           r radius
           color 16bit color 
 */
static void
mrbc_esp32_ili934x_draw_fillcircle(mrb_vm* vm, mrb_value* v, int argc)
{
  spi_device_handle_t spi_handle = *((spi_device_handle_t *)(v[0].instance->data));

  int x0 = 160;
  int y0 = 120;
  int _r = 40;
  int _color = 0;
  
  //オプション解析
  MRBC_KW_ARG(x, y, r, color);
  if( MRBC_ISNUMERIC(x) ) {
    x0 = MRBC_TO_INT(x);
  }
  if( MRBC_ISNUMERIC(y) ) {
    y0 = MRBC_TO_INT(y);
  }
  if( MRBC_ISNUMERIC(r) ) {
    _r = MRBC_TO_INT(r);
  }
  if( MRBC_ISNUMERIC(color) ) {
    _color = MRBC_TO_INT(color);
  }
  
  int x1 = 0, y1 = -1 * _r, err = 2 - 2 * _r, old_err, ChangeX = 1;
  
  do
    {
      if (ChangeX)
        {
	  ili934x_draw_line(spi_handle, x0 - x1, y0 - y1, x0 - x1, y0 + y1, _color);
	  ili934x_draw_line(spi_handle, x0 + x1, y0 - y1, x0 + x1, y0 + y1, _color);
        } // endif
      ChangeX = (old_err = err) <= x1;
      if (ChangeX)
	err += ++x1 * 2 + 1;
      if (old_err > y1 || err > x1)
	err += ++y1 * 2 + 1;
    } while (y1 <= 0);
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

  int x0 = 160;
  int y0 = 120;
  int _r = 40;
  int _color = 0;
  
  //オプション解析
  MRBC_KW_ARG(x, y, r, color);
  if( MRBC_ISNUMERIC(x) ) {
    x0 = MRBC_TO_INT(x);
  }
  if( MRBC_ISNUMERIC(y) ) {
    y0 = MRBC_TO_INT(y);
  }
  if( MRBC_ISNUMERIC(r) ) {
    _r = MRBC_TO_INT(r);
  }
  if( MRBC_ISNUMERIC(color) ) {
    _color = MRBC_TO_INT(color);
  }
  
  int x1 = 0, y1 = - 1 * _r, err = 2 - 2 * _r, old_err;
    
  do{
    ili934x_draw_pixel(spi_handle, x0 - x1, y0 + y1, _color);
    ili934x_draw_pixel(spi_handle, x0 - y1, y0 - x1, _color);
    ili934x_draw_pixel(spi_handle, x0 + x1, y0 - y1, _color);
    ili934x_draw_pixel(spi_handle, x0 + y1, y0 + x1, _color);
    if ((old_err = err) <= x1)
      err += ++x1 * 2 + 1;
    if (old_err > y1 || err > x1)
      err += ++y1 * 2 + 1;
  } while (y1 < 0); 
}

/*! Method __draw_char(x, y, color)
    @param x point x
           y point y
           c character
           color 16bit color 
           height height pixel size of drawing font
 */
static void
mrbc_esp32_ili934x_draw_string(mrb_vm* vm, mrb_value* v, int argc)
{
  spi_device_handle_t spi_handle = *((spi_device_handle_t *)(v[0].instance->data));
  char* string = (char*)GET_STRING_ARG(1);
  
  int _x = 160;
  int _y = 120;
  int _size = 0;
  int _color = 0;
  
  //オプション解析
  MRBC_KW_ARG(x, y, pointsize, color);
  if( MRBC_ISNUMERIC(x) ) {
    _x = MRBC_TO_INT(x);
  }
  if( MRBC_ISNUMERIC(y) ) {
    _y = MRBC_TO_INT(y);
  }
  if( MRBC_ISNUMERIC(pointsize) ) {
    _size = MRBC_TO_INT(pointsize);
  }
  if( MRBC_ISNUMERIC(color) ) {
    _color = MRBC_TO_INT(color);
  }

  //  #define MRBC_ISNUMERIC(val) 
  //  ((val).tt == MRBC_TT_INTEGER || (val).tt == MRBC_TT_FLOAT)

  int x0 = _x;
  int y0 = _y;
  int height = _size;
  int margin_x = 3;
  int margin_y = 3;  
  int home_x = _x;
  int ch = 0;

  for (int i = 0; i < strlen(string); i++) {
    //printf("x0 : %d \n", x0);
    char char0 = string[i];
    if (char0 == '\n') {
      x0 = home_x;
      y0 += height;
      y0 += margin_y;
      continue;
    }
    ch = (int)char0;
    x0 += ili934x_draw_char(spi_handle, x0, y0, ch, _color, _size);
    //printf("width: %d\n", ili934x_draw_char(spi_handle, _x, _y, ch, _color, _size));
    x0 += margin_x;
    //    printf("x0 : %d \n", x0);
  }
}

/*
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
*/

/*! Register SPI Class.p
 */
void
mrbc_esp32_lcdspi_gem_init(struct VM* vm)
{
  //クラスUART定義
  mrbc_class *spi = mrbc_define_class(vm, "LCDSPI", mrbc_class_object);

  // 各メソッド定義
  mrbc_define_method(vm, spi, "new",        mrbc_esp32_ili934x_new);
  mrbc_define_method(vm, spi, "initialize", mrbc_esp32_ili934x_initialize);
  mrbc_define_method(vm, spi, "rectangle",  mrbc_esp32_ili934x_draw_fillrectangle);
  mrbc_define_method(vm, spi, "fillcircle", mrbc_esp32_ili934x_draw_fillcircle);
  mrbc_define_method(vm, spi, "circle",     mrbc_esp32_ili934x_draw_circle);
  mrbc_define_method(vm, spi, "line",       mrbc_esp32_ili934x_draw_line);
  mrbc_define_method(vm, spi, "string",     mrbc_esp32_ili934x_draw_string);
}

