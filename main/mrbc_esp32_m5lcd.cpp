#include <M5Unified.h>

extern "C" {
#include "mrubyc.h"
}

/*
 * Rubyの [R, G, B] をM5GFX用RGB565へ変換する。
 *
 * color_valueがnilまたは不正な配列ならdefault_colorを返す。
 */
static uint16_t mrbc_m5lcd_rgb565(
    mrbc_value *color_value,
    uint16_t default_color)
{
  if (color_value == NULL ||
      color_value->tt == MRBC_TT_NIL) {
    return default_color;
  }

  if (color_value->tt != MRBC_TT_ARRAY) {
    ESP_LOGE(
      "M5LCD",
      "color must be an array: [r, g, b]"
    );
    return default_color;
  }

  if (mrbc_array_size(color_value) < 3) {
    ESP_LOGE(
      "M5LCD",
      "color array requires three elements"
    );
    return default_color;
  }

  mrbc_value r_value = mrbc_array_get(color_value, 0);
  mrbc_value g_value = mrbc_array_get(color_value, 1);
  mrbc_value b_value = mrbc_array_get(color_value, 2);

  if (r_value.tt != MRBC_TT_INTEGER ||
      g_value.tt != MRBC_TT_INTEGER ||
      b_value.tt != MRBC_TT_INTEGER) {
    ESP_LOGE(
      "M5LCD",
      "RGB values must be integers"
    );
    return default_color;
  }

  int r = mrbc_integer(r_value);
  int g = mrbc_integer(g_value);
  int b = mrbc_integer(b_value);

  /* 0から255の範囲に収める */
  if (r < 0) r = 0;
  if (r > 255) r = 255;

  if (g < 0) g = 0;
  if (g > 255) g = 255;

  if (b < 0) b = 0;
  if (b > 255) b = 255;

  return M5.Display.color565(r, g, b);
}

//--------------------------------------------------

static void mrbc_esp32_m5lcd_new(struct VM *vm, mrbc_value v[], int argc)
{
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(bool));
  mrbc_instance_call_initialize( vm, v, argc );
  
}

static void mrbc_esp32_m5lcd_initialize(struct VM *vm, mrbc_value v[], int argc)
{
  auto cfg = M5.config();

  //ディスプレイだけ制御するための処置
  cfg.internal_imu = false;
  cfg.external_imu = false;  
  cfg.internal_rtc = false;
  cfg.external_rtc = false;
  cfg.internal_spk = false;
  cfg.internal_mic = false;
  cfg.output_power = false;
  cfg.led_brightness = 0;
  cfg.clear_display = true;
  M5.begin(cfg);

  M5.Display.setRotation(0);
  M5.Display.fillScreen(TFT_BLACK);

  *((bool *)(v[0].instance->data)) = true;
}

static void mrbc_esp32_m5lcd_width(struct VM *vm, mrbc_value v[], int argc)
{
  SET_INT_RETURN(M5.Display.width());
}

static void mrbc_esp32_m5lcd_height(struct VM *vm, mrbc_value v[], int argc)
{
  SET_INT_RETURN(M5.Display.height());
}

static void mrbc_esp32_m5lcd_clear(struct VM *vm, mrbc_value v[], int argc)
{
  uint16_t color = TFT_BLACK;

  if (argc >= 1) {
    color = GET_INT_ARG(1);
  }
  
  M5.Display.fillScreen(color);

  SET_NIL_RETURN();
}

static void
mrbc_esp32_m5lcd_rect(mrbc_vm *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG(x, y, width, height, color, fill);

  int rect_x = 0;
  int rect_y = 0;
  int rect_width = M5.Display.width();
  int rect_height = M5.Display.height();
  bool rect_fill = true;
  
  if (MRBC_ISNUMERIC(x)) {
    rect_x = MRBC_TO_INT(x);
  }
  
  if (MRBC_ISNUMERIC(y)) {
    rect_y = MRBC_TO_INT(y);
  }

  if (MRBC_ISNUMERIC(width)) {
    rect_width = MRBC_TO_INT(width);
  }

  if (MRBC_ISNUMERIC(height)) {
    rect_height = MRBC_TO_INT(height);
  }

  /*
   * fill: false のときだけ枠線にする。
   * nilまたは省略時は塗りつぶす。
   */
  if (fill.tt == MRBC_TT_FALSE) {
    rect_fill = false;
  }

  if (rect_width <= 0 || rect_height <= 0) {
    ESP_LOGE(
      "M5LCD",
      "rectangle width and height must be positive"
    );
    SET_FALSE_RETURN();
    return;
  }

  uint16_t lcd_color =
    mrbc_m5lcd_rgb565(&color, TFT_WHITE);

  if (rect_fill) {
    M5.Display.fillRect(rect_x, rect_y, rect_width, rect_height, lcd_color);
  } else {
    M5.Display.drawRect(rect_x, rect_y, rect_width, rect_height, lcd_color);
  }
  
  SET_TRUE_RETURN();
}

static void
mrbc_esp32_m5lcd_circle(mrbc_vm *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG(x, y, radius, color, fill);

  int circ_x = 0;
  int circ_y = 0;
  int circ_radius = 0;
  bool circ_fill = true;
  
  if (MRBC_ISNUMERIC(x)) {
    circ_x = MRBC_TO_INT(x);
  }
  
  if (MRBC_ISNUMERIC(y)) {
    circ_y = MRBC_TO_INT(y);
  }

  if (MRBC_ISNUMERIC(radius)) {
    circ_radius = MRBC_TO_INT(radius);
  }

  /*
   * fill: false のときだけ枠線にする。
   * nilまたは省略時は塗りつぶす。
   */
  if (fill.tt == MRBC_TT_FALSE) {
    circ_fill = false;
  }

  if (circ_radius <= 0) {
    ESP_LOGE(
      "M5LCD",
      "radius must be positive"
    );
    SET_FALSE_RETURN();
    return;
  }

  uint16_t lcd_color =
    mrbc_m5lcd_rgb565(&color, TFT_WHITE);

  if (circ_fill) {
    M5.Display.fillCircle(circ_x, circ_y, circ_radius, lcd_color);
  } else {
    M5.Display.drawCircle(circ_x, circ_y, circ_radius, lcd_color);
  }
  
  SET_TRUE_RETURN();
}


static void
mrbc_esp32_m5lcd_line(mrbc_vm *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG(x1, y1, x2, y2, color);

  if (!MRBC_ISNUMERIC(x1) ||
      !MRBC_ISNUMERIC(y1) ||
      !MRBC_ISNUMERIC(x2) ||
      !MRBC_ISNUMERIC(y2)) {
    ESP_LOGE(
      "M5LCD",
      "line requires x1, y1, x2 and y2"
    );

    SET_FALSE_RETURN();
    return;
  }

  int line_x1 = MRBC_TO_INT(x1);
  int line_y1 = MRBC_TO_INT(y1);
  int line_x2 = MRBC_TO_INT(x2);
  int line_y2 = MRBC_TO_INT(y2);

  uint16_t lcd_color =
    mrbc_m5lcd_rgb565(&color, TFT_WHITE);

  M5.Display.drawLine(line_x1, line_y1, line_x2, line_y2, lcd_color);

  SET_TRUE_RETURN();
}


static void
mrbc_esp32_m5lcd_string(mrbc_vm *vm, mrbc_value v[], int argc)
{
  if (argc < 1 || v[1].tt != MRBC_TT_STRING) {
    ESP_LOGE(
      "M5LCD",
      "string requires a String as the first argument"
    );

    SET_FALSE_RETURN();
    return;
  }

  const char *text =
    (const char *)mrbc_string_cstr(&v[1]);

  /*
   * 第1引数は文字列なので、
   * キーワード引数の解析対象はその後ろになる。
   *
   * MRBC_KW_ARGがv[]全体からキーワードを探す実装なら
   * このままでよい。
   */
  MRBC_KW_ARG(x, y, pointsize, color, background);

  int text_x = 0;
  int text_y = 0;
  int text_pointsize = 8;

  if (MRBC_ISNUMERIC(x)) {
    text_x = MRBC_TO_INT(x);
  }

  if (MRBC_ISNUMERIC(y)) {
    text_y = MRBC_TO_INT(y);
  }

  if (MRBC_ISNUMERIC(pointsize)) {
    text_pointsize = MRBC_TO_INT(pointsize);
  }

  if (text_pointsize < 1) {
    text_pointsize = 1;
  }

  uint16_t text_color =
    mrbc_m5lcd_rgb565(&color, TFT_WHITE);

  /*
   * M5GFXの標準フォントはおおむね高さ8ピクセル。
   * pointsizeをピクセル高さと解釈して倍率を計算する。
   */
  float text_scale =
    (float)text_pointsize / 8.0f;

  if (text_scale < 1.0f) {
    text_scale = 1.0f;
  }

  M5.Display.setTextSize(text_scale);
  M5.Display.setTextDatum(top_left);

  if (background.tt == MRBC_TT_ARRAY) {
    uint16_t background_color =
      mrbc_m5lcd_rgb565(
        &background,
        TFT_BLACK
      );

    M5.Display.setTextColor(
      text_color,
      background_color
    );
  } else {
    M5.Display.setTextColor(text_color);
  }

  M5.Display.drawString(
    text,
    text_x,
    text_y
  );

  SET_TRUE_RETURN();
}


extern "C"
void mrbc_esp32_m5lcd_gem_init(struct VM* vm)
{
  mrbc_class *lcd =
    mrbc_define_class(0, "M5LCD", mrbc_class_object);
  
 
  mrbc_define_method(0, lcd, "new",         mrbc_esp32_m5lcd_new);
  mrbc_define_method(0, lcd, "initialize",  mrbc_esp32_m5lcd_initialize);
  
  mrbc_define_method(0, lcd, "width",       mrbc_esp32_m5lcd_width);
  mrbc_define_method(0, lcd, "height",      mrbc_esp32_m5lcd_height);
  mrbc_define_method(0, lcd, "clear",       mrbc_esp32_m5lcd_clear);
  
  mrbc_define_method(0, lcd, "line",        mrbc_esp32_m5lcd_line);  
  mrbc_define_method(0, lcd, "rect",        mrbc_esp32_m5lcd_rect);
  mrbc_define_method(0, lcd, "circle",      mrbc_esp32_m5lcd_circle);
  mrbc_define_method(0, lcd, "string",      mrbc_esp32_m5lcd_string);
}  

