/*! @file
  @brief mruby/c LCDSPI class for ILI9342/ILI9341-compatible displays.

  Ruby API is aligned with M5LCD:
    width, height, clear, pixel, rect, circle, line, text

  LCDSPI never initializes the SPI bus. The SPI object passed to new()
  must already have initialized it.
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mrbc_esp32_lcdspi.h"
#include "mrbc_esp32_lcdspi_font.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

static const char *TAG = "LCDSPI";

#define LCDSPI_NATIVE_WIDTH   320
#define LCDSPI_NATIVE_HEIGHT  240
#define LCDSPI_DEFAULT_FREQ   SPI_MASTER_FREQ_40M
#define LCDSPI_COLOR_BLACK    0x0000
#define LCDSPI_COLOR_WHITE    0xFFFF
#define LCDSPI_TX_PIXELS      256

typedef struct {
  spi_device_handle_t spi;
  int dc_pin;
  int rst_pin;
  int bl_pin;
  int width;
  int height;
  int rotation;
  bool initialized;
} lcdspi_data_t;

static uint8_t tx_bytes[LCDSPI_TX_PIXELS * 2];

/* ---------- mruby/c argument helpers ---------- */

static bool get_pair(const mrbc_value *value, int *a, int *b, const char *name)
{
  if (value->tt != MRBC_TT_ARRAY || mrbc_array_size(value) != 2) {
    ESP_LOGE(TAG, "%s must be an array with two values", name);
    return false;
  }

  mrbc_value va = mrbc_array_get(value, 0);
  mrbc_value vb = mrbc_array_get(value, 1);
  if (!MRBC_ISNUMERIC(va) || !MRBC_ISNUMERIC(vb)) {
    ESP_LOGE(TAG, "%s values must be numeric", name);
    return false;
  }

  *a = MRBC_TO_INT(va);
  *b = MRBC_TO_INT(vb);
  return true;
}

static uint16_t rgb565_from_value(const mrbc_value *value, uint16_t default_color)
{
  if (value == NULL || value->tt == MRBC_TT_EMPTY || value->tt == MRBC_TT_NIL) {
    return default_color;
  }
  if (value->tt != MRBC_TT_ARRAY || mrbc_array_size(value) < 3) {
    ESP_LOGE(TAG, "color must be [r, g, b]");
    return default_color;
  }

  mrbc_value vr = mrbc_array_get(value, 0);
  mrbc_value vg = mrbc_array_get(value, 1);
  mrbc_value vb = mrbc_array_get(value, 2);
  if (!MRBC_ISNUMERIC(vr) || !MRBC_ISNUMERIC(vg) || !MRBC_ISNUMERIC(vb)) {
    ESP_LOGE(TAG, "RGB values must be numeric");
    return default_color;
  }

  int r = MRBC_TO_INT(vr);
  int g = MRBC_TO_INT(vg);
  int b = MRBC_TO_INT(vb);
  if (r < 0) r = 0; else if (r > 255) r = 255;
  if (g < 0) g = 0; else if (g > 255) g = 255;
  if (b < 0) b = 0; else if (b > 255) b = 255;

  return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static lcdspi_data_t *get_lcd(mrbc_value v[])
{
  if (v[0].instance == NULL) return NULL;
  return (lcdspi_data_t *)v[0].instance->data;
}

/* ---------- ILI9342 low-level operations ---------- */

static esp_err_t spi_write(lcdspi_data_t *lcd, const void *data, size_t length)
{
  if (length == 0) return ESP_OK;
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  t.length = length * 8;
  t.tx_buffer = data;
  return spi_device_transmit(lcd->spi, &t);
}

static esp_err_t write_command(lcdspi_data_t *lcd, uint8_t command)
{
  gpio_set_level(lcd->dc_pin, 0);
  return spi_write(lcd, &command, 1);
}

static esp_err_t write_data(lcdspi_data_t *lcd, const void *data, size_t length)
{
  gpio_set_level(lcd->dc_pin, 1);
  return spi_write(lcd, data, length);
}

static esp_err_t write_data8(lcdspi_data_t *lcd, uint8_t data)
{
  return write_data(lcd, &data, 1);
}

static esp_err_t set_window(lcdspi_data_t *lcd, int x0, int y0, int x1, int y1)
{
  uint8_t data[4];
  esp_err_t err;

  err = write_command(lcd, 0x2A);
  if (err != ESP_OK) return err;
  data[0] = (uint8_t)(x0 >> 8); data[1] = (uint8_t)x0;
  data[2] = (uint8_t)(x1 >> 8); data[3] = (uint8_t)x1;
  err = write_data(lcd, data, sizeof(data));
  if (err != ESP_OK) return err;

  err = write_command(lcd, 0x2B);
  if (err != ESP_OK) return err;
  data[0] = (uint8_t)(y0 >> 8); data[1] = (uint8_t)y0;
  data[2] = (uint8_t)(y1 >> 8); data[3] = (uint8_t)y1;
  err = write_data(lcd, data, sizeof(data));
  if (err != ESP_OK) return err;

  return write_command(lcd, 0x2C);
}

static bool clip_rect(lcdspi_data_t *lcd, int *x, int *y, int *w, int *h)
{
  if (*x < 0) { *w += *x; *x = 0; }
  if (*y < 0) { *h += *y; *y = 0; }
  if (*x + *w > lcd->width)  *w = lcd->width - *x;
  if (*y + *h > lcd->height) *h = lcd->height - *y;
  return *w > 0 && *h > 0;
}

static esp_err_t fill_rect_raw(lcdspi_data_t *lcd, int x, int y, int w, int h, uint16_t color)
{
  if (!clip_rect(lcd, &x, &y, &w, &h)) return ESP_OK;
  esp_err_t err = set_window(lcd, x, y, x + w - 1, y + h - 1);
  if (err != ESP_OK) return err;

  for (int i = 0; i < LCDSPI_TX_PIXELS; ++i) {
    tx_bytes[i * 2] = (uint8_t)(color >> 8);
    tx_bytes[i * 2 + 1] = (uint8_t)color;
  }

  int remaining = w * h;
  gpio_set_level(lcd->dc_pin, 1);
  while (remaining > 0) {
    int count = remaining > LCDSPI_TX_PIXELS ? LCDSPI_TX_PIXELS : remaining;
    err = spi_write(lcd, tx_bytes, (size_t)count * 2);
    if (err != ESP_OK) return err;
    remaining -= count;
  }
  return ESP_OK;
}

static esp_err_t draw_pixel_raw(lcdspi_data_t *lcd, int x, int y, uint16_t color)
{
  if (x < 0 || x >= lcd->width || y < 0 || y >= lcd->height) return ESP_OK;
  uint8_t data[2] = {(uint8_t)(color >> 8), (uint8_t)color};
  esp_err_t err = set_window(lcd, x, y, x, y);
  if (err != ESP_OK) return err;
  return write_data(lcd, data, sizeof(data));
}

static void draw_line_raw(lcdspi_data_t *lcd, int x0, int y0, int x1, int y1, int width, uint16_t color)
{
  if (width < 1) width = 1;

  /* Fast paths: one rectangular transfer rather than one transfer per pixel. */
  if (y0 == y1) {
    int x = x0 < x1 ? x0 : x1;
    int w = abs(x1 - x0) + 1;
    fill_rect_raw(lcd, x, y0 - width / 2, w, width, color);
    return;
  }
  if (x0 == x1) {
    int y = y0 < y1 ? y0 : y1;
    int h = abs(y1 - y0) + 1;
    fill_rect_raw(lcd, x0 - width / 2, y, width, h, color);
    return;
  }

  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  int radius = (width - 1) / 2;
  for (;;) {
    if (width == 1) draw_pixel_raw(lcd, x0, y0, color);
    else fill_rect_raw(lcd, x0 - radius, y0 - radius, width, width, color);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}
static void draw_circle_raw(lcdspi_data_t *lcd, int x0, int y0, int radius, bool fill, uint16_t color)
{
  int x = radius;
  int y = 0;
  int err = 1 - x;

  while (x >= y) {
    if (fill) {
      draw_line_raw(lcd, x0 - x, y0 + y, x0 + x, y0 + y, 1, color);
      draw_line_raw(lcd, x0 - x, y0 - y, x0 + x, y0 - y, 1, color);
      draw_line_raw(lcd, x0 - y, y0 + x, x0 + y, y0 + x, 1, color);
      draw_line_raw(lcd, x0 - y, y0 - x, x0 + y, y0 - x, 1, color);
    } else {
      draw_pixel_raw(lcd, x0 + x, y0 + y, color);
      draw_pixel_raw(lcd, x0 + y, y0 + x, color);
      draw_pixel_raw(lcd, x0 - y, y0 + x, color);
      draw_pixel_raw(lcd, x0 - x, y0 + y, color);
      draw_pixel_raw(lcd, x0 - x, y0 - y, color);
      draw_pixel_raw(lcd, x0 - y, y0 - x, color);
      draw_pixel_raw(lcd, x0 + y, y0 - x, color);
      draw_pixel_raw(lcd, x0 + x, y0 - y, color);
    }
    ++y;
    if (err < 0) err += 2 * y + 1;
    else { --x; err += 2 * (y - x) + 1; }
  }
}

/* ---------- Baseline-aware proportional font ---------- */

static bool glyph_bit(const lcdspi_glyph_t *g, int x, int y)
{
  int bit_index = y * g->width + x;
  uint8_t b = lcdspi_font_bitmap[g->bitmap_offset + bit_index / 8];
  return (b & (0x80u >> (bit_index & 7))) != 0;
}

static inline void put_rgb565(uint8_t *buffer, int index, uint16_t color)
{
  buffer[index * 2] = (uint8_t)(color >> 8);
  buffer[index * 2 + 1] = (uint8_t)color;
}

/* Opaque glyph: build one RGB565 rectangle and transfer it in a single SPI transaction. */
static int draw_char_buffered(lcdspi_data_t *lcd, int pen_x, int baseline_y, uint8_t c,
                              int pointsize, uint16_t foreground, uint16_t background)
{
  if (c < LCDSPI_FONT_FIRST || c > LCDSPI_FONT_LAST) c = '?';
  const lcdspi_glyph_t *g = &lcdspi_font_glyphs[c - LCDSPI_FONT_FIRST];
  float scale = (float)pointsize / (float)LCDSPI_FONT_BASE_SIZE;
  if (scale < 0.5f) scale = 0.5f;

  int out_w = (int)(g->width * scale + 0.5f);
  int out_h = (int)(g->height * scale + 0.5f);
  int x_offset = (int)(g->x_offset * scale);
  int y_offset = (int)(g->y_offset * scale);
  int advance = (int)(g->advance * scale + 0.5f);
  if (advance < 1) advance = 1;
  if (out_w <= 0 || out_h <= 0) return advance;

  int draw_x = pen_x + x_offset;
  int draw_y = baseline_y + y_offset;
  if (draw_x < 0 || draw_y < 0 || draw_x + out_w > lcd->width || draw_y + out_h > lcd->height) {
    /* Clipped glyphs use transparent runs to avoid an oversized clipping buffer. */
    return advance;
  }

  size_t bytes = (size_t)out_w * (size_t)out_h * 2;
  uint8_t *buffer = (uint8_t *)heap_caps_malloc(bytes, MALLOC_CAP_DMA);
  if (!buffer) {
    ESP_LOGE(TAG, "glyph buffer allocation failed: %u bytes", (unsigned)bytes);
    return advance;
  }

  for (int oy = 0; oy < out_h; ++oy) {
    int sy = (int)(oy / scale);
    if (sy >= g->height) sy = g->height - 1;
    for (int ox = 0; ox < out_w; ++ox) {
      int sx = (int)(ox / scale);
      if (sx >= g->width) sx = g->width - 1;
      uint16_t pixel = glyph_bit(g, sx, sy) ? foreground : background;
      put_rgb565(buffer, oy * out_w + ox, pixel);
    }
  }

  esp_err_t err = set_window(lcd, draw_x, draw_y, draw_x + out_w - 1, draw_y + out_h - 1);
  if (err == ESP_OK) {
    gpio_set_level(lcd->dc_pin, 1);
    err = spi_write(lcd, buffer, bytes);
  }
  heap_caps_free(buffer);
  if (err != ESP_OK) ESP_LOGE(TAG, "glyph transfer failed: %s", esp_err_to_name(err));
  return advance;
}

/* Transparent glyph: transfer each continuous foreground run, not each pixel. */
static int draw_char_transparent_runs(lcdspi_data_t *lcd, int pen_x, int baseline_y, uint8_t c,
                                      int pointsize, uint16_t foreground)
{
  if (c < LCDSPI_FONT_FIRST || c > LCDSPI_FONT_LAST) c = '?';
  const lcdspi_glyph_t *g = &lcdspi_font_glyphs[c - LCDSPI_FONT_FIRST];
  float scale = (float)pointsize / (float)LCDSPI_FONT_BASE_SIZE;
  if (scale < 0.5f) scale = 0.5f;
  int out_w = (int)(g->width * scale + 0.5f);
  int out_h = (int)(g->height * scale + 0.5f);
  int draw_x = pen_x + (int)(g->x_offset * scale);
  int draw_y = baseline_y + (int)(g->y_offset * scale);
  int advance = (int)(g->advance * scale + 0.5f);
  if (advance < 1) advance = 1;
  if (out_w <= 0 || out_h <= 0) return advance;

  uint8_t *line = (uint8_t *)heap_caps_malloc((size_t)out_w * 2, MALLOC_CAP_DMA);
  if (!line) return advance;
  for (int i = 0; i < out_w; ++i) put_rgb565(line, i, foreground);

  for (int oy = 0; oy < out_h; ++oy) {
    int sy = (int)(oy / scale);
    if (sy >= g->height) sy = g->height - 1;
    int ox = 0;
    while (ox < out_w) {
      while (ox < out_w) {
        int sx = (int)(ox / scale);
        if (sx < g->width && glyph_bit(g, sx, sy)) break;
        ++ox;
      }
      int run_start = ox;
      while (ox < out_w) {
        int sx = (int)(ox / scale);
        if (sx >= g->width || !glyph_bit(g, sx, sy)) break;
        ++ox;
      }
      int run_width = ox - run_start;
      int tx = draw_x + run_start;
      int ty = draw_y + oy;
      if (run_width > 0 && ty >= 0 && ty < lcd->height) {
        int skip = 0;
        if (tx < 0) { skip = -tx; run_width -= skip; tx = 0; }
        if (tx + run_width > lcd->width) run_width = lcd->width - tx;
        if (run_width > 0) {
          esp_err_t err = set_window(lcd, tx, ty, tx + run_width - 1, ty);
          if (err == ESP_OK) {
            gpio_set_level(lcd->dc_pin, 1);
            spi_write(lcd, line + skip * 2, (size_t)run_width * 2);
          }
        }
      }
    }
  }
  heap_caps_free(line);
  return advance;
}

/*
 * 文字列1行分の表示幅を求める。
 *
 * text:
 *   行頭を指す
 *
 * pointsize:
 *   表示サイズ
 *
 * 戻り値:
 *   改行または文字列末尾までの幅
 */
static int
measure_text_line(
  const unsigned char *text,
  int pointsize
)
{
  float scale =
    (float)pointsize /
    (float)LCDSPI_FONT_BASE_SIZE;

  if (scale < 0.5f) {
    scale = 0.5f;
  }

  int width = 0;

  while (*text != '\0' &&
         *text != '\n') {
    unsigned char c = *text;

    if (c < LCDSPI_FONT_FIRST ||
        c > LCDSPI_FONT_LAST) {
      c = '?';
    }

    const lcdspi_glyph_t *glyph =
      &lcdspi_font_glyphs[
        c - LCDSPI_FONT_FIRST
      ];

    int advance =
      (int)(
        glyph->advance *
        scale +
        0.5f
      );

    if (advance < 1) {
      advance = 1;
    }

    width += advance;
    ++text;
  }

  return width;
}


/*
 * 文字列を描画する。
 *
 * backgroundが指定されている場合:
 *   1. 行全体を長方形で塗りつぶす
 *   2. 文字だけを透明背景方式で重ねる
 *
 * これにより、文字ごとの縁取り状の背景ではなく、
 * 以前と同じ連続した長方形背景になる。
 */
static void
draw_text_buffered(
  lcdspi_data_t *lcd,
  const char *text,
  int x,
  int y,
  int pointsize,
  uint16_t color,
  bool has_background,
  uint16_t background
)
{
  float scale =
    (float)pointsize /
    (float)LCDSPI_FONT_BASE_SIZE;

  if (scale < 0.5f) {
    scale = 0.5f;
  }

  int ascent =
    (int)(
      LCDSPI_FONT_ASCENT *
      scale +
      0.5f
    );

  int descent =
    (int)(
      LCDSPI_FONT_DESCENT *
      scale +
      0.5f
    );

  int line_margin =
    (int)(
      2.0f *
      scale +
      0.5f
    );

  if (line_margin < 1) {
    line_margin = 1;
  }

  /*
   * 文字背景に使う実際の高さ。
   *
   * ベースラインより上がascent、
   * 下がdescent。
   */
  int background_height =
    ascent +
    descent;

  int line_height =
    background_height +
    line_margin;

  int pen_x = x;
  int line_top = y;
  int baseline_y =
    line_top +
    ascent;

  const unsigned char *p =
    (const unsigned char *)text;

  /*
   * 最初の行の背景を描く。
   */
  if (has_background) {
    int line_width =
      measure_text_line(
        p,
        pointsize
      );

    if (line_width > 0) {
      fill_rect_raw(
        lcd,
        x,
        line_top,
        line_width,
        background_height,
        background
      );
    }
  }

  while (*p != '\0') {
    if (*p == '\n') {
      ++p;

      pen_x = x;
      line_top += line_height;
      baseline_y =
        line_top +
        ascent;

      /*
       * 次の行の背景を長方形で描く。
       */
      if (has_background) {
        int line_width =
          measure_text_line(
            p,
            pointsize
          );

        if (line_width > 0) {
          fill_rect_raw(
            lcd,
            x,
            line_top,
            line_width,
            background_height,
            background
          );
        }
      }

      continue;
    }

    /*
     * 背景はすでに行全体を塗ってあるので、
     * 文字部分だけを透明背景方式で重ねる。
     */
    pen_x +=
      draw_char_transparent_runs(
        lcd,
        pen_x,
        baseline_y,
        *p,
        pointsize,
        color
      );

    ++p;
  }
}



/* ---------- Panel initialization ---------- */

static esp_err_t panel_set_rotation(lcdspi_data_t *lcd, int rotation)
{
  /* ILI9342 Classic Core. rotation=1 retains the former 320x240 setup. */
  static const uint8_t madctl[4] = {0x68, 0x08, 0xA8, 0xC8};
  if (rotation < 0 || rotation > 3) return ESP_ERR_INVALID_ARG;
  esp_err_t err = write_command(lcd, 0x36);
  if (err != ESP_OK) return err;
  err = write_data8(lcd, madctl[rotation]);
  if (err != ESP_OK) return err;
  lcd->rotation = rotation;
  if (rotation & 1) { lcd->width = 320; lcd->height = 240; }
  else { lcd->width = 240; lcd->height = 320; }
  return ESP_OK;
}

static esp_err_t panel_init(lcdspi_data_t *lcd, int rotation)
{
  esp_err_t err;
  gpio_set_level(lcd->rst_pin, 0);
  vTaskDelay(pdMS_TO_TICKS(20));
  gpio_set_level(lcd->rst_pin, 1);
  vTaskDelay(pdMS_TO_TICKS(120));

#define CMD(c) do { err = write_command(lcd, (c)); if (err != ESP_OK) return err; } while (0)
#define DATA(d) do { err = write_data8(lcd, (d)); if (err != ESP_OK) return err; } while (0)

  CMD(0xC0); DATA(0x23);
  CMD(0xC1); DATA(0x10);
  CMD(0xC5); DATA(0x3E); DATA(0x28);
  CMD(0xC7); DATA(0x86);
  CMD(0x3A); DATA(0x55);
  CMD(0x21);
  CMD(0xB1); DATA(0x00); DATA(0x18);
  CMD(0xB6); DATA(0x08); DATA(0xA2); DATA(0x27); DATA(0x00);
  CMD(0x26); DATA(0x01);

  static const uint8_t gamma_p[] = {0x0F,0x31,0x2B,0x0C,0x0E,0x08,0x4E,0xF1,0x37,0x07,0x10,0x03,0x0E,0x09,0x00};
  static const uint8_t gamma_n[] = {0x00,0x0E,0x14,0x03,0x11,0x07,0x31,0xC1,0x48,0x08,0x0F,0x0C,0x31,0x36,0x0F};
  CMD(0xE0); err = write_data(lcd, gamma_p, sizeof(gamma_p)); if (err != ESP_OK) return err;
  CMD(0xE1); err = write_data(lcd, gamma_n, sizeof(gamma_n)); if (err != ESP_OK) return err;

  err = panel_set_rotation(lcd, rotation);
  if (err != ESP_OK) return err;
  CMD(0x11);
  vTaskDelay(pdMS_TO_TICKS(120));
  CMD(0x29);

#undef CMD
#undef DATA
  return ESP_OK;
}

/* ---------- mruby/c methods ---------- */

static void lcdspi_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(lcdspi_data_t));
  mrbc_instance_call_initialize(vm, v, argc);
}

static void lcdspi_initialize(mrbc_vm *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG(cs_pin, dc_pin, rst_pin, bl_pin, rotation);
  if (argc < 1 || v[1].tt != MRBC_TT_OBJECT || v[1].instance == NULL) {
    ESP_LOGE(TAG, "first argument must be an SPI object");
    SET_FALSE_RETURN();
    return;
  }

  int cs = MRBC_ISNUMERIC(cs_pin) ? MRBC_TO_INT(cs_pin) : 14;
  int dc = MRBC_ISNUMERIC(dc_pin) ? MRBC_TO_INT(dc_pin) : 27;
  int rst = MRBC_ISNUMERIC(rst_pin) ? MRBC_TO_INT(rst_pin) : 33;
  int bl = MRBC_ISNUMERIC(bl_pin) ? MRBC_TO_INT(bl_pin) : 32;
  int rot = MRBC_ISNUMERIC(rotation) ? MRBC_TO_INT(rotation) : 1;
  if (rot < 0 || rot > 3) {
    ESP_LOGE(TAG, "rotation must be between 0 and 3");
    SET_FALSE_RETURN();
    return;
  }

  spi_host_device_t host = *((spi_host_device_t *)v[1].instance->data);
  if (host != SPI2_HOST && host != SPI3_HOST) {
    ESP_LOGE(TAG, "invalid SPI host: %d", (int)host);
    SET_FALSE_RETURN();
    return;
  }

  lcdspi_data_t *lcd = get_lcd(v);
  memset(lcd, 0, sizeof(*lcd));
  lcd->dc_pin = dc; lcd->rst_pin = rst; lcd->bl_pin = bl;

  spi_device_interface_config_t dev = {
    .command_bits = 0,
    .address_bits = 0,
    .dummy_bits = 0,
    .mode = 0,
    .clock_speed_hz = LCDSPI_DEFAULT_FREQ,
    .spics_io_num = cs,
    .flags = SPI_DEVICE_NO_DUMMY,
    .queue_size = 1,
  };

  esp_err_t err = spi_bus_add_device(host, &dev, &lcd->spi);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "spi_bus_add_device failed: %s", esp_err_to_name(err));
    SET_FALSE_RETURN();
    return;
  }

  gpio_reset_pin(dc); gpio_set_direction(dc, GPIO_MODE_OUTPUT);
  gpio_reset_pin(rst); gpio_set_direction(rst, GPIO_MODE_OUTPUT);
  gpio_reset_pin(bl); gpio_set_direction(bl, GPIO_MODE_OUTPUT);
  gpio_set_level(bl, 1);

  err = panel_init(lcd, rot);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "panel initialization failed: %s", esp_err_to_name(err));
    SET_FALSE_RETURN();
    return;
  }

  lcd->initialized = true;
  fill_rect_raw(lcd, 0, 0, lcd->width, lcd->height, LCDSPI_COLOR_BLACK);
  ESP_LOGI(TAG, "initialized: %dx%d rotation=%d", lcd->width, lcd->height, rot);
}

static void lcdspi_width(mrbc_vm *vm, mrbc_value v[], int argc)
{
  lcdspi_data_t *lcd = get_lcd(v);
  SET_INT_RETURN(lcd && lcd->initialized ? lcd->width : 0);
}

static void lcdspi_height(mrbc_vm *vm, mrbc_value v[], int argc)
{
  lcdspi_data_t *lcd = get_lcd(v);
  SET_INT_RETURN(lcd && lcd->initialized ? lcd->height : 0);
}

static void lcdspi_clear(mrbc_vm *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG(color);
  lcdspi_data_t *lcd = get_lcd(v);
  if (!lcd || !lcd->initialized) { SET_FALSE_RETURN(); return; }
  uint16_t c = rgb565_from_value(&color, LCDSPI_COLOR_BLACK);
  if (fill_rect_raw(lcd, 0, 0, lcd->width, lcd->height, c) != ESP_OK) { SET_FALSE_RETURN(); return; }
  SET_NIL_RETURN();
}

static void lcdspi_pixel(mrbc_vm *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG(pos, color);
  lcdspi_data_t *lcd = get_lcd(v);
  int x, y;
  if (!lcd || !lcd->initialized || pos.tt == MRBC_TT_EMPTY || !get_pair(&pos, &x, &y, "pos")) { SET_FALSE_RETURN(); return; }
  if (x < 0 || x >= lcd->width || y < 0 || y >= lcd->height) { SET_FALSE_RETURN(); return; }
  draw_pixel_raw(lcd, x, y, rgb565_from_value(&color, LCDSPI_COLOR_WHITE));
  SET_TRUE_RETURN();
}

static void lcdspi_rect(mrbc_vm *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG(pos, size, color, fill);
  lcdspi_data_t *lcd = get_lcd(v);
  if (!lcd || !lcd->initialized) { SET_FALSE_RETURN(); return; }
  int x = 0, y = 0, w = lcd->width, h = lcd->height;
  if (pos.tt != MRBC_TT_EMPTY && !get_pair(&pos, &x, &y, "pos")) { SET_FALSE_RETURN(); return; }
  if (size.tt != MRBC_TT_EMPTY && !get_pair(&size, &w, &h, "size")) { SET_FALSE_RETURN(); return; }
  if (w <= 0 || h <= 0) { SET_FALSE_RETURN(); return; }
  uint16_t c = rgb565_from_value(&color, LCDSPI_COLOR_WHITE);
  if (fill.tt == MRBC_TT_FALSE) {
    fill_rect_raw(lcd, x, y, w, 1, c); fill_rect_raw(lcd, x, y + h - 1, w, 1, c);
    fill_rect_raw(lcd, x, y, 1, h, c); fill_rect_raw(lcd, x + w - 1, y, 1, h, c);
  } else fill_rect_raw(lcd, x, y, w, h, c);
  SET_TRUE_RETURN();
}

static void lcdspi_line(mrbc_vm *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG(pos_start, pos_end, width, color);
  lcdspi_data_t *lcd = get_lcd(v);
  int x0, y0, x1, y1;
  if (!lcd || !lcd->initialized ||
      !get_pair(&pos_start, &x0, &y0, "pos_start") ||
      !get_pair(&pos_end, &x1, &y1, "pos_end")) { SET_FALSE_RETURN(); return; }
  int line_width = MRBC_ISNUMERIC(width) ? MRBC_TO_INT(width) : 1;
  if (line_width <= 0) { SET_FALSE_RETURN(); return; }
  draw_line_raw(lcd, x0, y0, x1, y1, line_width, rgb565_from_value(&color, LCDSPI_COLOR_WHITE));
  SET_TRUE_RETURN();
}

static void lcdspi_circle(mrbc_vm *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG(pos, radius, color, fill);
  lcdspi_data_t *lcd = get_lcd(v);
  int x, y;
  if (!lcd || !lcd->initialized || !get_pair(&pos, &x, &y, "pos") || !MRBC_ISNUMERIC(radius)) { SET_FALSE_RETURN(); return; }
  int r = MRBC_TO_INT(radius);
  if (r <= 0) { SET_FALSE_RETURN(); return; }
  draw_circle_raw(lcd, x, y, r, fill.tt != MRBC_TT_FALSE, rgb565_from_value(&color, LCDSPI_COLOR_WHITE));
  SET_TRUE_RETURN();
}

static void lcdspi_text(mrbc_vm *vm, mrbc_value v[], int argc)
{
  if (argc < 1 || v[1].tt != MRBC_TT_STRING) { SET_FALSE_RETURN(); return; }
  MRBC_KW_ARG(pos, pointsize, color, background);
  lcdspi_data_t *lcd = get_lcd(v);
  int x = 0, y = 0;
  if (!lcd || !lcd->initialized ||
      (pos.tt != MRBC_TT_EMPTY && !get_pair(&pos, &x, &y, "pos"))) { SET_FALSE_RETURN(); return; }
  int size = MRBC_ISNUMERIC(pointsize) ? MRBC_TO_INT(pointsize) : 16;
  if (size <= 0) { SET_FALSE_RETURN(); return; }
  const char *text = (const char *)mrbc_string_cstr(&v[1]);
  bool has_bg = background.tt == MRBC_TT_ARRAY;
  draw_text_buffered(lcd, text, x, y, size,
                     rgb565_from_value(&color, LCDSPI_COLOR_WHITE),
                     has_bg, rgb565_from_value(&background, LCDSPI_COLOR_BLACK));
  SET_TRUE_RETURN();
}

void mrbc_esp32_lcdspi_gem_init(struct VM *vm)
{
  mrbc_class *lcd = mrbc_define_class(vm, "LCDSPI", mrbc_class_object);
  mrbc_define_method(vm, lcd, "new",        lcdspi_new);
  mrbc_define_method(vm, lcd, "initialize", lcdspi_initialize);
  mrbc_define_method(vm, lcd, "width",      lcdspi_width);
  mrbc_define_method(vm, lcd, "height",     lcdspi_height);
  mrbc_define_method(vm, lcd, "clear",      lcdspi_clear);
  mrbc_define_method(vm, lcd, "pixel",      lcdspi_pixel);
  mrbc_define_method(vm, lcd, "rect",       lcdspi_rect);
  mrbc_define_method(vm, lcd, "line",       lcdspi_line);
  mrbc_define_method(vm, lcd, "circle",     lcdspi_circle);
  mrbc_define_method(vm, lcd, "text",       lcdspi_text);
}
