/*! @file
  @brief
  mruby/c SPI class for ESP32
*/

#include "mrbc_esp32_spi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

static char* tag = "main";
#define DMA_CHAN    2

static struct RClass* mrbc_class_esp32_spi;
spi_device_handle_t spidev;
uint16_t dc;

static void spi_write_byte(const uint8_t* Data, size_t DataLength)
{
	spi_transaction_t SPITransaction;
    esp_err_t ret;

	if ( DataLength > 0 ) {
		memset( &SPITransaction, 0, sizeof( spi_transaction_t ) );
		SPITransaction.length = DataLength * 8;
		SPITransaction.tx_buffer = Data;
		ret = spi_device_transmit( spidev, &SPITransaction );
		assert(ret==ESP_OK); 
	}
}


/*! Method spi_bus_initialize(mosi, miso, sclk, cs, dc, rst, bl) body : wrapper for spi_bus_initialize.
    @param mosi MOSI Pin Number
           miso MISO Pin Number
           sclk SPI Clock Pin Number
           cs   CS Pin Number
           dc   DC Pin Number
           rst  RST Pin Number(If it is not needed, pass -1)
           bl   BL Pin Number (If it is not needed, pass -1)
    @return if succeeded, return true, otherwise return false.
 */
static void
mrbc_esp32_spi_bus_initialize(mrb_vm* vm, mrb_value* v, int argc)
{

    int gpio_mosi = GET_INT_ARG(1);
    int gpio_miso = GET_INT_ARG(2);
    int gpio_sclk = GET_INT_ARG(3);
    int gpio_cs = GET_INT_ARG(4);
    int gpio_dc = GET_INT_ARG(5);
    int gpio_rst = GET_INT_ARG(6);
    int gpio_bl = GET_INT_ARG(7);

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = gpio_mosi,
        .miso_io_num = -1,
        .sclk_io_num = gpio_sclk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };

    esp_err_t ret = spi_bus_initialize(HSPI_HOST, &bus_cfg, DMA_CHAN);
    assert(ret == ESP_OK);

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = SPI_MASTER_FREQ_40M,
        .spics_io_num = gpio_cs,
        .queue_size = 7,
        .flags = SPI_DEVICE_NO_DUMMY,
    };
    dc = gpio_dc;
    ret = spi_bus_add_device(HSPI_HOST, &dev_cfg, &spidev);
    assert(ret == ESP_OK);
}

/*! Method write_byte(data)
    @param data
 */
static void
mrbc_esp32_spi_write_byte(mrb_vm* vm, mrb_value* v, int argc)
{
    spi_transaction_t transaction;
    esp_err_t ret;
    assert(GET_ARG(1).tt == MRBC_TT_ARRAY);
    mrbc_value *data = GET_ARG(1).array->data;
    uint8_t Data[1024];
    size_t dataLength = GET_INT_ARG(2);
    for (int i = 0; i < dataLength; i++)
    {
        Data[i] = data[i].i;
    }
    memset(&transaction, 0, sizeof(spi_transaction_t));
    transaction.length = 8 * dataLength;
    transaction.tx_buffer = Data;
    ret = spi_device_transmit(spidev, &transaction);
    assert(ret == ESP_OK);
}

/*! Method read_byte()
    @return recv_data
 */
static void
mrbc_esp32_spi_read_byte(mrb_vm* vm, mrb_value* v, int argc)
{
    spi_transaction_t transaction;
    uint8_t recv_data;
    esp_err_t ret;
    memset(&transaction, 0, sizeof(transaction));
    transaction.length = 8;
    transaction.rx_buffer = &recv_data;
 
    ret=spi_device_polling_transmit(spidev, &transaction);
    assert(ret==ESP_OK);
    SET_INT_RETURN(recv_data);
}

// ILI934X library

uint8_t byte[1024];
static void ili934x_write_color(int color, int size)
{
	int index = 0;
	for(int i = 0;i < size; i++) {
		byte[index++] = (color >> 8) & 0xFF;
		byte[index++] = color & 0xFF;
	}
    gpio_set_level(dc, 1);
	spi_write_byte(byte, size * 2);
}

static void ili934x_write_command(int command)
{
    gpio_set_level(dc, 0);
	spi_write_byte(&command, 1);
}

static void ili934x_write_data(int data)
{
    gpio_set_level(dc, 1);
	spi_write_byte(&data, 1);
}

static void ili934x_write_addr(int addr1, int addr2)
{
    byte[0] = (addr1 >> 8) & 0xFF;
	byte[1] = addr1 & 0xFF;
	byte[2] = (addr2 >> 8) & 0xFF;
	byte[3] = addr2 & 0xFF;
    gpio_set_level(dc, 1);
    spi_write_byte(byte, 4);
}

static void ili934x_draw_pixel(int x, int y, int color)
{
    ili934x_write_command(0x2a);
    ili934x_write_addr(x, x);
    ili934x_write_command(0x2b);
    ili934x_write_addr(y, y);
    ili934x_write_command(0x2c);
    byte[0] = (color >> 8) & 0xFF;
    byte[1] = color & 0xFF;
    gpio_set_level(dc, 1);
    spi_write_byte(byte, 2);
}

static void ili934x_draw_line(int x1, int y1, int x2, int y2, int color)
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
            ili934x_draw_pixel(x1, y1, color);
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
            ili934x_draw_pixel(x1, y1, color);
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
    int x1 = GET_INT_ARG(1);
    int y1 = GET_INT_ARG(2);
    int x2 = GET_INT_ARG(3);
    int y2 = GET_INT_ARG(4);
    int color = GET_INT_ARG(5);
    ili934x_write_command(0x2a);
    ili934x_write_addr(x1, x2);
    ili934x_write_command(0x2b);
    ili934x_write_addr(y1, y2);
    ili934x_write_command(0x2c);
    int size = y2 - y1 + 1;
    for (int i = x1; i <= x2; i++)
        ili934x_write_color(color, size);
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
    int x1 = GET_INT_ARG(1);
    int y1 = GET_INT_ARG(2);
    int x2 = GET_INT_ARG(3);
    int y2 = GET_INT_ARG(4);
    int color = GET_INT_ARG(5);
    ili934x_draw_line(x1, y1, x2, y2, color);
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
    int x0 = GET_INT_ARG(1);
    int y0 = GET_INT_ARG(2);
    int r = GET_INT_ARG(3);
    int color = GET_INT_ARG(4);
    int x = 0, y = -r, err = 2 - 2 * r, old_err;
    do
    {
        ili934x_draw_pixel(x0 - x, y0 + y, color);
        ili934x_draw_pixel(x0 - y, y0 - x, color);
        ili934x_draw_pixel(x0 + x, y0 - y, color);
        ili934x_draw_pixel(x0 + y, y0 + x, color);
        if ((old_err = err) <= x)
            err += ++x * 2 + 1;
        if (old_err > y || err > x)
            err += ++y * 2 + 1;
    } while (y < 0);
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
    int x0 = GET_INT_ARG(1);
    int y0 = GET_INT_ARG(2);
    int r = GET_INT_ARG(3);
    int color = GET_INT_ARG(4);
    int x = 0, y = -r, err = 2 - 2 * r, old_err, ChangeX = 1;

    do
    {
        if (ChangeX)
        {
            ili934x_draw_line(x0 - x, y0 - y, x0 - x, y0 + y, color);
            ili934x_draw_line(x0 + x, y0 - y, x0 + x, y0 + y, color);
        } // endif
        ChangeX = (old_err = err) <= x;
        if (ChangeX)
            err += ++x * 2 + 1;
        if (old_err > y || err > x)
            err += ++y * 2 + 1;
    } while (y <= 0);
}

/*! Method __draw_pixel(x, y, color)
    @param x point x
           y point y
           color 16bit color 
 */
static void
mrbc_esp32_ili934x_draw_pixel(mrb_vm* vm, mrb_value* v, int argc)
{
    int x = GET_INT_ARG(1);
    int y = GET_INT_ARG(2);
    int color = GET_INT_ARG(3);
    ili934x_draw_pixel(x, y, color);
}

/*! Register SPI Class.
 */
void
mrbc_mruby_esp32_spi_gem_init(struct VM* vm)
{
  // クラス SPI 定義
  mrbc_class_esp32_spi = mrbc_define_class(vm, "SPI", mrbc_class_object);
  // 各メソッド定義
  mrbc_define_method(vm, mrbc_class_esp32_spi, "bus_initialize",      mrbc_esp32_spi_bus_initialize);
  mrbc_define_method(vm, mrbc_class_esp32_spi, "write_byte",          mrbc_esp32_spi_write_byte);
  mrbc_define_method(vm, mrbc_class_esp32_spi, "read_byte",           mrbc_esp32_spi_read_byte);
  mrbc_define_method(vm, mrbc_class_esp32_spi, "__draw_fillrectangle",mrbc_esp32_ili934x_draw_fillrectangle);
  mrbc_define_method(vm, mrbc_class_esp32_spi, "__draw_circle",       mrbc_esp32_ili934x_draw_circle);
  mrbc_define_method(vm, mrbc_class_esp32_spi, "__draw_fillcircle",   mrbc_esp32_ili934x_draw_fillcircle);
  mrbc_define_method(vm, mrbc_class_esp32_spi, "__draw_line",         mrbc_esp32_ili934x_draw_line);
  mrbc_define_method(vm, mrbc_class_esp32_spi, "__draw_pixel",        mrbc_esp32_ili934x_draw_pixel);
}
