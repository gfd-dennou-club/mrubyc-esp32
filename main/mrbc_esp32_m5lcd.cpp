#include <M5Unified.h>

extern "C" {
#include "mrubyc.h"
}

/*
 * Rubyの [R, G, B] をM5GFX用RGB565へ変換する。
 *
 * color_valueがnilまたは不正な配列ならdefault_colorを返す。
 */
static uint16_t
mrbc_m5lcd_rgb565(mrbc_value *color_value, uint16_t default_color)
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


static bool
mrbc_m5lcd_get_pair(const mrbc_value *value, int *value1, int *value2, const char *keyword)
{
  if (value->tt != MRBC_TT_ARRAY ||
      mrbc_array_size(value) != 2) {
    ESP_LOGE(
      "M5LCD",
      "%s must be an array with two values",
      keyword
    );
    return false;
  }

  mrbc_value v1 = mrbc_array_get(value, 0);
  mrbc_value v2 = mrbc_array_get(value, 1);

  if (!MRBC_ISNUMERIC(v1) ||
      !MRBC_ISNUMERIC(v2)) {
    ESP_LOGE(
      "M5LCD",
      "%s values must be numeric",
      keyword
    );
    return false;
  }

  *value1 = MRBC_TO_INT(v1);
  *value2 = MRBC_TO_INT(v2);

  return true;
}

//--------------------------------------------------

static void mrbc_esp32_m5lcd_new(struct VM *vm, mrbc_value v[], int argc)
{
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(bool));
  mrbc_instance_call_initialize( vm, v, argc );
  
}

static void mrbc_esp32_m5lcd_initialize(struct VM *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG(rotation);
 
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

  int rotation_value = 1;
  
  if (rotation.tt != MRBC_TT_EMPTY) {
    if (!MRBC_ISNUMERIC(rotation)) {
      ESP_LOGE("M5LCD", "rotation must be numeric");
      SET_FALSE_RETURN();
      return;
    }

    rotation_value = MRBC_TO_INT(rotation);
  }

  if (rotation_value < 0 || rotation_value > 3) {
    ESP_LOGE("M5LCD", "rotation must be between 0 and 3");
  }
  
  M5.Display.setRotation(rotation_value);
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

static void
mrbc_esp32_m5lcd_clear(mrbc_vm *vm, mrbc_value v[], int argc)
{
  /*
   * Ruby側:
   *
   * display.clear
   * display.clear(color: [255, 0, 0])
   */
  MRBC_KW_ARG(color);

  /*
   * color省略時は黒。
   * 指定時は [R, G, B] をRGB565へ変換する。
   */
  uint16_t lcd_color =
    mrbc_m5lcd_rgb565(
      &color,
      TFT_BLACK
    );

  M5.Display.fillScreen(lcd_color);

  SET_NIL_RETURN();
}

static void
mrbc_esp32_m5lcd_rect(mrbc_vm *vm, mrbc_value v[], int argc)
{
  /*
   * Ruby側:
   *
   * display.rect(
   *   pos: [10, 20],
   *   size: [100, 50],
   *   color: [255, 0, 0],
   *   fill: false
   * )
   */
  MRBC_KW_ARG(pos, size, color, fill);

  int rect_x = 0;
  int rect_y = 0;
  int rect_width = M5.Display.width();
  int rect_height = M5.Display.height();
  bool rect_fill = true;

  if (pos.tt != MRBC_TT_EMPTY) {
    if (!mrbc_m5lcd_get_pair(
        &pos,
        &rect_x,
        &rect_y,
        "pos")) {
      SET_FALSE_RETURN();
      return;
    }
  }

  if (size.tt != MRBC_TT_EMPTY) {
    if (!mrbc_m5lcd_get_pair(
        &size,
        &rect_width,
        &rect_height,
        "size")) {
      SET_FALSE_RETURN();
      return;
    }
  }

  /*
   * fill: false の場合だけ枠線にする。
   *
   * fillがtrue、省略、nilの場合は塗りつぶす。
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
    M5.Display.fillRect(
      rect_x,
      rect_y,
      rect_width,
      rect_height,
      lcd_color
    );
  } else {
    M5.Display.drawRect(
      rect_x,
      rect_y,
      rect_width,
      rect_height,
      lcd_color
    );
  }

  SET_TRUE_RETURN();
}


static void
mrbc_esp32_m5lcd_circle(mrbc_vm *vm, mrbc_value v[], int argc)
{
  /*
   * Ruby側:
   *
   * display.circle(
   *   pos: [70, 76],
   *   radius: 20,
   *   color: [0, 0, 255],
   *   fill: false
   * )
   */
  MRBC_KW_ARG(pos, radius, color, fill);

  int circ_x = 0;
  int circ_y = 0;
  int circ_radius = 0;
  bool circ_fill = true;

  /*
   * pos: [x, y] を円の中心座標として解析する。
   * pos は必須とする
   */
  if (pos.tt == MRBC_TT_EMPTY) {
    ESP_LOGE(
      "M5LCD",
      "pos is required"
    );
    SET_FALSE_RETURN();
    return;
  }
  
  if (!mrbc_m5lcd_get_pair(
      &pos,
      &circ_x,
      &circ_y,
      "pos")) {
    SET_FALSE_RETURN();
    return;
  }
  
  /*
   * radiusは必須。
   */
  if (!MRBC_ISNUMERIC(radius)) {
    ESP_LOGE(
      "M5LCD",
      "radius must be numeric"
    );
    SET_FALSE_RETURN();
    return;
  }

  circ_radius = MRBC_TO_INT(radius);

  if (circ_radius <= 0) {
    ESP_LOGE(
      "M5LCD",
      "radius must be positive"
    );
    SET_FALSE_RETURN();
    return;
  }

  /*
   * fill: false のときだけ枠線にする。
   * true、nil、または省略時は塗りつぶす。
   */
  if (fill.tt == MRBC_TT_FALSE) {
    circ_fill = false;
  }

  uint16_t lcd_color =
    mrbc_m5lcd_rgb565(&color, TFT_WHITE);

  if (circ_fill) {
    M5.Display.fillCircle(
      circ_x,
      circ_y,
      circ_radius,
      lcd_color
    );
  } else {
    M5.Display.drawCircle(
      circ_x,
      circ_y,
      circ_radius,
      lcd_color
    );
  }

  SET_TRUE_RETURN();
}


static void
mrbc_esp32_m5lcd_line(mrbc_vm *vm, mrbc_value v[], int argc)
{
  /*
   * Ruby側:
   *
   * display.line(
   *   pos_start: [10, 46],
   *   pos_end: [23, 107],
   *   width: 3,
   *   color: [255, 255, 255]
   * )
   */
  MRBC_KW_ARG(pos_start, pos_end, width, color);

  int line_x1 = 0;
  int line_y1 = 0;
  int line_x2 = 0;
  int line_y2 = 0;
  int line_width = 1;

  /*
   * pos_start: [x, y] は必須。
   */
  if (pos_start.tt == MRBC_TT_EMPTY) {
    ESP_LOGE(
      "M5LCD",
      "pos_start is required"
    );
    SET_FALSE_RETURN();
    return;
  }

  if (!mrbc_m5lcd_get_pair(
        &pos_start,
        &line_x1,
        &line_y1,
        "pos_start")) {
    SET_FALSE_RETURN();
    return;
  }

  /*
   * pos_end: [x, y] も必須。
   */
  if (pos_end.tt == MRBC_TT_EMPTY) {
    ESP_LOGE(
      "M5LCD",
      "pos_end is required"
    );
    SET_FALSE_RETURN();
    return;
  }

  if (!mrbc_m5lcd_get_pair(
        &pos_end,
        &line_x2,
        &line_y2,
        "pos_end")) {
    SET_FALSE_RETURN();
    return;
  }

  /*
   * width省略時は1ピクセル。
   */
  if (width.tt != MRBC_TT_EMPTY) {
    if (!MRBC_ISNUMERIC(width)) {
      ESP_LOGE(
        "M5LCD",
        "width must be numeric"
      );
      SET_FALSE_RETURN();
      return;
    }

    line_width = MRBC_TO_INT(width);
  }

  if (line_width <= 0) {
    ESP_LOGE(
      "M5LCD",
      "width must be positive"
    );
    SET_FALSE_RETURN();
    return;
  }

  uint16_t lcd_color =
    mrbc_m5lcd_rgb565(&color, TFT_WHITE);

  /*
   * 線幅1の場合は通常の線描画を使う。
   */
  if (line_width == 1) {
    M5.Display.drawLine(
      line_x1,
      line_y1,
      line_x2,
      line_y2,
      lcd_color
    );
  } else {
    /*
     * M5GFXの太線描画。
     *
     * 座標と線幅はfloat型で渡す。
     */
    M5.Display.drawWideLine(
      (float)line_x1,
      (float)line_y1,
      (float)line_x2,
      (float)line_y2,
      (float)line_width,
      lcd_color
    );
  }

  SET_TRUE_RETURN();
}


static void
mrbc_esp32_m5lcd_text(mrbc_vm *vm, mrbc_value v[], int argc)
{
  /*
   * Ruby側:
   *
   * display.text(
   *   "Matz.ba.Gany ROBO",
   *   pos: [0, 90],
   *   pointsize: 10,
   *   color: [0, 0, 0],
   *   background: [255, 255, 255]
   * )
   */

  /*
   * 第1引数の文字列を確認する。
   */
  if (argc < 1 || v[1].tt != MRBC_TT_STRING) {
    ESP_LOGE(
      "M5LCD",
      "text requires a String as the first argument"
    );

    SET_FALSE_RETURN();
    return;
  }

  const char *text =
    (const char *)mrbc_string_cstr(&v[1]);

  /*
   * pos:        [x, y]
   * pointsize:  文字サイズ
   * color:      文字色 [r, g, b]
   * background: 背景色 [r, g, b]
   */
  MRBC_KW_ARG(pos, pointsize, color, background);

  int text_x = 0;
  int text_y = 0;
  int text_pointsize = 8;

  /*
   * pos: [x, y] の解析。
   *
   * posを省略した場合は [0, 0] とする。
   */
  if (pos.tt != MRBC_TT_EMPTY) {
    if (!mrbc_m5lcd_get_pair(
          &pos,
          &text_x,
          &text_y,
          "pos")) {
      SET_FALSE_RETURN();
      return;
    }
  }

  /*
   * pointsizeを解析する。
   *
   * 省略時は8。
   */
  if (pointsize.tt != MRBC_TT_EMPTY) {
    if (!MRBC_ISNUMERIC(pointsize)) {
      ESP_LOGE(
        "M5LCD",
        "pointsize must be numeric"
      );

      SET_FALSE_RETURN();
      return;
    }

    text_pointsize = MRBC_TO_INT(pointsize);
  }

  if (text_pointsize <= 0) {
    ESP_LOGE(
      "M5LCD",
      "pointsize must be positive"
    );

    SET_FALSE_RETURN();
    return;
  }

  /*
   * 文字色をRGB565へ変換する。
   *
   * color省略時は白。
   */
  uint16_t text_color =
    mrbc_m5lcd_rgb565(
      &color,
      TFT_WHITE
    );

  /*
   * M5GFX標準フォントの基本高さを8ピクセルとして、
   * pointsizeから表示倍率を計算する。
   */
  float text_scale =
    (float)text_pointsize / 8.0f;

  if (text_scale < 1.0f) {
    text_scale = 1.0f;
  }

  M5.Display.setTextSize(text_scale);

  /*
   * posは文字列の左上座標とする。
   */
  M5.Display.setTextDatum(top_left);

  /*
   * backgroundが指定されている場合は背景色も設定する。
   */
  if (background.tt != MRBC_TT_EMPTY) {
    if (background.tt != MRBC_TT_ARRAY) {
      ESP_LOGE(
        "M5LCD",
        "background must be [r, g, b]"
      );

      SET_FALSE_RETURN();
      return;
    }

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
    /*
     * 背景色なし。
     *
     * M5GFXでは通常、背景を塗りつぶさず文字だけを描画する。
     */
    M5.Display.setTextColor(text_color);
  }

  M5.Display.drawString(
    text,
    text_x,
    text_y
  );

  SET_TRUE_RETURN();
}

static void
mrbc_esp32_m5lcd_pixel(mrbc_vm *vm, mrbc_value v[], int argc)
{
  /*
   * Ruby側:
   *
   * display.pixel(
   *   pos: [64, 64],
   *   color: [255, 0, 0]
   * )
   */
  MRBC_KW_ARG(pos, color);

  int pixel_x = 0;
  int pixel_y = 0;

  /*
   * pos: [x, y] は必須。
   */
  if (pos.tt == MRBC_TT_EMPTY) {
    ESP_LOGE(
      "M5LCD",
      "pos is required"
    );

    SET_FALSE_RETURN();
    return;
  }

  /*
   * pos: [x, y] を解析する。
   */
  if (!mrbc_m5lcd_get_pair(
        &pos,
        &pixel_x,
        &pixel_y,
        "pos")) {
    SET_FALSE_RETURN();
    return;
  }

  /*
   * 画面の範囲外ならエラーとする。
   */
  if (pixel_x < 0 ||
      pixel_x >= M5.Display.width() ||
      pixel_y < 0 ||
      pixel_y >= M5.Display.height()) {
    ESP_LOGE(
      "M5LCD",
      "pixel position is outside the display: "
      "pos=[%d, %d], display=%dx%d",
      pixel_x,
      pixel_y,
      M5.Display.width(),
      M5.Display.height()
    );

    SET_FALSE_RETURN();
    return;
  }

  /*
   * color: [R, G, B] をRGB565へ変換する。
   *
   * color省略時は白。
   */
  uint16_t lcd_color =
    mrbc_m5lcd_rgb565(
      &color,
      TFT_WHITE
    );

  M5.Display.drawPixel(
    pixel_x,
    pixel_y,
    lcd_color
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
  mrbc_define_method(0, lcd, "text",        mrbc_esp32_m5lcd_text);
  mrbc_define_method(0, lcd, "pixel",       mrbc_esp32_m5lcd_pixel);
}  

