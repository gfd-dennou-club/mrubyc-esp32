#include <M5Unified.h>
#include "esp_log.h"

extern "C" {
#include "mrubyc.h"
}

/*
 * Rubyの [R, G, B] をM5GFX用RGB565へ変換する。
 *
 * color_valueがnilまたは不正な配列ならdefault_colorを返す。
 */
static uint16_t
mrbc_m5unified_rgb565(mrbc_value *color_value, uint16_t default_color)
{
  if (color_value == NULL ||
      color_value->tt == MRBC_TT_NIL) {
    return default_color;
  }

  if (color_value->tt != MRBC_TT_ARRAY) {
    ESP_LOGE(
      "M5Unified",
      "color must be an array: [r, g, b]"
    );
    return default_color;
  }

  if (mrbc_array_size(color_value) < 3) {
    ESP_LOGE(
      "M5Unified",
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
      "M5Unified",
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
mrbc_m5unified_get_pair(const mrbc_value *value, int *value1, int *value2, const char *keyword)
{
  if (value->tt != MRBC_TT_ARRAY ||
      mrbc_array_size(value) != 2) {
    ESP_LOGE(
      "M5Unified",
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
      "M5Unified",
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

static void mrbc_esp32_m5unified_new(struct VM *vm, mrbc_value v[], int argc)
{
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(bool));
  mrbc_instance_call_initialize( vm, v, argc );
  
}

static bool m5u_initialized = false;

static void mrbc_esp32_m5unified_initialize(struct VM *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG(rotation, imu, rtc, speaker, microphone, output_power, clear_display);
  int rotation_value = MRBC_ISNUMERIC(rotation) ? MRBC_TO_INT(rotation) : 1;
  if (rotation_value < 0 || rotation_value > 3) { SET_FALSE_RETURN(); return; }

  if (!m5u_initialized) {
    auto cfg = M5.config();
    cfg.internal_imu = imu.tt != MRBC_TT_FALSE;
    cfg.external_imu = false;
    cfg.internal_rtc = rtc.tt != MRBC_TT_FALSE;
    cfg.external_rtc = false;
    cfg.internal_spk = speaker.tt != MRBC_TT_FALSE;
    cfg.internal_mic = microphone.tt == MRBC_TT_TRUE;
    cfg.output_power = output_power.tt != MRBC_TT_FALSE;
    cfg.clear_display = clear_display.tt != MRBC_TT_FALSE;
    M5.begin(cfg);
    if (M5.Display.width() <= 0 || M5.Display.height() <= 0) {
      ESP_LOGE("M5Unified", "display initialization failed: %dx%d",
               M5.Display.width(), M5.Display.height());
      SET_FALSE_RETURN();
      return;
    }
    m5u_initialized = true;
  }
  M5.Display.setRotation(rotation_value);
  *((bool *)(v[0].instance->data)) = true;
}

static void mrbc_esp32_m5unified_width(struct VM *vm, mrbc_value v[], int argc)
{
  SET_INT_RETURN(M5.Display.width());
}

static void mrbc_esp32_m5unified_height(struct VM *vm, mrbc_value v[], int argc)
{
  SET_INT_RETURN(M5.Display.height());
}

static void
mrbc_esp32_m5unified_clear(mrbc_vm *vm, mrbc_value v[], int argc)
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
    mrbc_m5unified_rgb565(
      &color,
      TFT_BLACK
    );

  M5.Display.fillScreen(lcd_color);

  SET_NIL_RETURN();
}

static void
mrbc_esp32_m5unified_rect(mrbc_vm *vm, mrbc_value v[], int argc)
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
    if (!mrbc_m5unified_get_pair(
        &pos,
        &rect_x,
        &rect_y,
        "pos")) {
      SET_FALSE_RETURN();
      return;
    }
  }

  if (size.tt != MRBC_TT_EMPTY) {
    if (!mrbc_m5unified_get_pair(
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
      "M5Unified",
      "rectangle width and height must be positive"
    );
    SET_FALSE_RETURN();
    return;
  }

  uint16_t lcd_color =
    mrbc_m5unified_rgb565(&color, TFT_WHITE);

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
mrbc_esp32_m5unified_circle(mrbc_vm *vm, mrbc_value v[], int argc)
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
      "M5Unified",
      "pos is required"
    );
    SET_FALSE_RETURN();
    return;
  }
  
  if (!mrbc_m5unified_get_pair(
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
      "M5Unified",
      "radius must be numeric"
    );
    SET_FALSE_RETURN();
    return;
  }

  circ_radius = MRBC_TO_INT(radius);

  if (circ_radius <= 0) {
    ESP_LOGE(
      "M5Unified",
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
    mrbc_m5unified_rgb565(&color, TFT_WHITE);

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
mrbc_esp32_m5unified_line(mrbc_vm *vm, mrbc_value v[], int argc)
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
      "M5Unified",
      "pos_start is required"
    );
    SET_FALSE_RETURN();
    return;
  }

  if (!mrbc_m5unified_get_pair(
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
      "M5Unified",
      "pos_end is required"
    );
    SET_FALSE_RETURN();
    return;
  }

  if (!mrbc_m5unified_get_pair(
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
        "M5Unified",
        "width must be numeric"
      );
      SET_FALSE_RETURN();
      return;
    }

    line_width = MRBC_TO_INT(width);
  }

  if (line_width <= 0) {
    ESP_LOGE(
      "M5Unified",
      "width must be positive"
    );
    SET_FALSE_RETURN();
    return;
  }

  uint16_t lcd_color =
    mrbc_m5unified_rgb565(&color, TFT_WHITE);

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
mrbc_esp32_m5unified_text(mrbc_vm *vm, mrbc_value v[], int argc)
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
      "M5Unified",
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
    if (!mrbc_m5unified_get_pair(
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
        "M5Unified",
        "pointsize must be numeric"
      );

      SET_FALSE_RETURN();
      return;
    }

    text_pointsize = MRBC_TO_INT(pointsize);
  }

  if (text_pointsize <= 0) {
    ESP_LOGE(
      "M5Unified",
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
    mrbc_m5unified_rgb565(
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
        "M5Unified",
        "background must be [r, g, b]"
      );

      SET_FALSE_RETURN();
      return;
    }

    uint16_t background_color =
      mrbc_m5unified_rgb565(
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
mrbc_esp32_m5unified_pixel(mrbc_vm *vm, mrbc_value v[], int argc)
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
      "M5Unified",
      "pos is required"
    );

    SET_FALSE_RETURN();
    return;
  }

  /*
   * pos: [x, y] を解析する。
   */
  if (!mrbc_m5unified_get_pair(
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
      "M5Unified",
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
    mrbc_m5unified_rgb565(
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


/* M5Unified subsystems: touch, IMU, buttons, speaker, power and RTC. */
typedef struct {
  float ax, ay, az, gx, gy, gz, mx, my, mz, temperature;
  bool accel_ok, gyro_ok, mag_ok, temp_ok;
  int16_t touch_x, touch_y;
  uint8_t touch_count;
  bool touch_pressed, touch_released, touch_clicked, touching, touch_holding;
} m5u_cache_t;
static m5u_cache_t m5u_cache;

static void m5u_update(mrbc_vm *vm, mrbc_value v[], int argc) { M5.update(); SET_NIL_RETURN(); }

static void m5u_board(mrbc_vm *vm, mrbc_value v[], int argc) { SET_INT_RETURN((int)M5.getBoard()); }

static void m5u_touch_available(mrbc_vm *vm, mrbc_value v[], int argc) { if (M5.Touch.isEnabled()) SET_TRUE_RETURN(); else SET_FALSE_RETURN(); }

static void m5u_touch_read(mrbc_vm *vm, mrbc_value v[], int argc) {
  if (!M5.Touch.isEnabled()) { SET_FALSE_RETURN(); return; }
  M5.update();
  m5u_cache.touch_count=M5.Touch.getCount();
  const auto &d=M5.Touch.getDetail(0);
  m5u_cache.touching=d.isPressed();
  m5u_cache.touch_pressed=d.wasPressed();
  m5u_cache.touch_released=d.wasReleased();
  m5u_cache.touch_clicked=d.wasClicked();
  m5u_cache.touch_holding=d.isHolding();
  if (m5u_cache.touching || m5u_cache.touch_pressed || m5u_cache.touch_released || m5u_cache.touch_clicked) {
    m5u_cache.touch_x=d.x; m5u_cache.touch_y=d.y;
  }
  SET_TRUE_RETURN();
}

#define BOOL_METHOD(name, expr) static void name(mrbc_vm *vm,mrbc_value v[],int argc){if(expr)SET_TRUE_RETURN();else SET_FALSE_RETURN();}

#define INT_METHOD(name, expr) static void name(mrbc_vm *vm,mrbc_value v[],int argc){SET_INT_RETURN((int)(expr));}

#define FLOAT_METHOD(name, expr) static void name(mrbc_vm *vm,mrbc_value v[],int argc){SET_FLOAT_RETURN((mrbc_float_t)(expr));}

BOOL_METHOD(m5u_touching,m5u_cache.touching) BOOL_METHOD(m5u_touch_pressed,m5u_cache.touch_pressed)

BOOL_METHOD(m5u_touch_released,m5u_cache.touch_released) BOOL_METHOD(m5u_touch_clicked,m5u_cache.touch_clicked)

BOOL_METHOD(m5u_touch_holding,m5u_cache.touch_holding) INT_METHOD(m5u_touch_x,m5u_cache.touch_x)

INT_METHOD(m5u_touch_y,m5u_cache.touch_y) INT_METHOD(m5u_touch_count,m5u_cache.touch_count)



/* IMU calibration state.
 * Calibration should normally be initiated by the user, not on every boot.
 */
static bool m5u_calibrating = false;

static int m5u_strength(const mrbc_value *value, int default_value)
{
  if (value->tt == MRBC_TT_EMPTY || value->tt == MRBC_TT_NIL) return default_value;
  if (!MRBC_ISNUMERIC(*value)) return -1;
  int result = MRBC_TO_INT(*value);
  if (result < 0 || result > 255) return -1;
  return result;
}

/* Ruby:
 *   m5.imu_calibration_start(accel: 64, gyro: 64, mag: 64)
 * Strength: 0 disables that sensor, 1 is weakest, 255 is strongest.
 */
static void m5u_imu_calibration_start(mrbc_vm *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG(accel, gyro, mag);
  if (!M5.Imu.isEnabled()) { SET_FALSE_RETURN(); return; }

  int a = m5u_strength(&accel, 64);
  int g = m5u_strength(&gyro, 64);
  int m = m5u_strength(&mag, 64);
  if (a < 0 || g < 0 || m < 0) {
    ESP_LOGE("M5Unified", "IMU calibration strength must be 0..255");
    SET_FALSE_RETURN();
    return;
  }

  M5.Imu.clearOffsetData();
  M5.Imu.setCalibration((uint8_t)a, (uint8_t)g, (uint8_t)m);
  m5u_calibrating = true;
  SET_TRUE_RETURN();
}

/* Call repeatedly during calibration, preferably every 10 to 20 ms. */
static void m5u_imu_calibration_update(mrbc_vm *vm, mrbc_value v[], int argc)
{
  if (!M5.Imu.isEnabled() || !m5u_calibrating) { SET_FALSE_RETURN(); return; }
  M5.Imu.update();
  SET_TRUE_RETURN();
}

/* Ruby:
 *   m5.imu_calibration_finish(save: true)
 */
static void m5u_imu_calibration_finish(mrbc_vm *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG(save);
  if (!M5.Imu.isEnabled() || !m5u_calibrating) { SET_FALSE_RETURN(); return; }

  M5.Imu.setCalibration(0, 0, 0);
  m5u_calibrating = false;

  bool should_save = save.tt != MRBC_TT_FALSE;
  if (should_save && !M5.Imu.saveOffsetToNVS()) {
    ESP_LOGE("M5Unified", "failed to save IMU calibration to NVS");
    SET_FALSE_RETURN();
    return;
  }
  SET_TRUE_RETURN();
}

static void m5u_imu_calibrating(mrbc_vm *vm, mrbc_value v[], int argc)
{
  if (m5u_calibrating) SET_TRUE_RETURN(); else SET_FALSE_RETURN();
}

static void m5u_imu_calibration_load(mrbc_vm *vm, mrbc_value v[], int argc)
{
  if (!M5.Imu.isEnabled()) { SET_FALSE_RETURN(); return; }
  if (M5.Imu.loadOffsetFromNVS()) SET_TRUE_RETURN(); else SET_FALSE_RETURN();
}

static void m5u_imu_calibration_clear(mrbc_vm *vm, mrbc_value v[], int argc)
{
  if (!M5.Imu.isEnabled()) { SET_FALSE_RETURN(); return; }
  M5.Imu.setCalibration(0, 0, 0);
  M5.Imu.clearOffsetData();
  m5u_calibrating = false;
  SET_TRUE_RETURN();
}


static void m5u_imu_available(mrbc_vm *vm,mrbc_value v[],int argc){if(M5.Imu.isEnabled())SET_TRUE_RETURN();else SET_FALSE_RETURN();}

static void m5u_imu_read(mrbc_vm *vm,mrbc_value v[],int argc){
  if(!M5.Imu.isEnabled()){SET_FALSE_RETURN();return;}
  m5u_cache.accel_ok=M5.Imu.getAccel(&m5u_cache.ax,&m5u_cache.ay,&m5u_cache.az);
  m5u_cache.gyro_ok=M5.Imu.getGyro(&m5u_cache.gx,&m5u_cache.gy,&m5u_cache.gz);
  m5u_cache.mag_ok=M5.Imu.getMag(&m5u_cache.mx,&m5u_cache.my,&m5u_cache.mz);
  m5u_cache.temp_ok=M5.Imu.getTemp(&m5u_cache.temperature);
  if(m5u_cache.accel_ok||m5u_cache.gyro_ok||m5u_cache.mag_ok||m5u_cache.temp_ok)SET_TRUE_RETURN();else SET_FALSE_RETURN();
}

BOOL_METHOD(m5u_accel_available,m5u_cache.accel_ok) BOOL_METHOD(m5u_gyro_available,m5u_cache.gyro_ok)

BOOL_METHOD(m5u_mag_available,m5u_cache.mag_ok)

FLOAT_METHOD(m5u_ax,m5u_cache.ax) FLOAT_METHOD(m5u_ay,m5u_cache.ay) FLOAT_METHOD(m5u_az,m5u_cache.az)

FLOAT_METHOD(m5u_gx,m5u_cache.gx) FLOAT_METHOD(m5u_gy,m5u_cache.gy) FLOAT_METHOD(m5u_gz,m5u_cache.gz)

FLOAT_METHOD(m5u_mx,m5u_cache.mx) FLOAT_METHOD(m5u_my,m5u_cache.my) FLOAT_METHOD(m5u_mz,m5u_cache.mz)

FLOAT_METHOD(m5u_temperature,m5u_cache.temperature)

static m5::Button_Class *m5u_button(int id){switch(id){case 0:return &M5.BtnA;case 1:return &M5.BtnB;case 2:return &M5.BtnC;case 3:return &M5.BtnPWR;default:return nullptr;}}

static void m5u_button_pressed(mrbc_vm *vm,mrbc_value v[],int argc){auto b=(argc>0&&MRBC_ISNUMERIC(v[1]))?m5u_button(MRBC_TO_INT(v[1])):nullptr;if(b&&b->isPressed())SET_TRUE_RETURN();else SET_FALSE_RETURN();}

static void m5u_button_was_pressed(mrbc_vm *vm,mrbc_value v[],int argc){auto b=(argc>0&&MRBC_ISNUMERIC(v[1]))?m5u_button(MRBC_TO_INT(v[1])):nullptr;if(b&&b->wasPressed())SET_TRUE_RETURN();else SET_FALSE_RETURN();}

static void m5u_button_was_released(mrbc_vm *vm,mrbc_value v[],int argc){auto b=(argc>0&&MRBC_ISNUMERIC(v[1]))?m5u_button(MRBC_TO_INT(v[1])):nullptr;if(b&&b->wasReleased())SET_TRUE_RETURN();else SET_FALSE_RETURN();}

static void m5u_button_clicked(mrbc_vm *vm,mrbc_value v[],int argc){auto b=(argc>0&&MRBC_ISNUMERIC(v[1]))?m5u_button(MRBC_TO_INT(v[1])):nullptr;if(b&&b->wasClicked())SET_TRUE_RETURN();else SET_FALSE_RETURN();}

static void m5u_button_pressed_for(mrbc_vm *vm,mrbc_value v[],int argc){if(argc<2||!MRBC_ISNUMERIC(v[1])||!MRBC_ISNUMERIC(v[2])){SET_FALSE_RETURN();return;}auto b=m5u_button(MRBC_TO_INT(v[1]));if(b&&b->pressedFor(MRBC_TO_INT(v[2])))SET_TRUE_RETURN();else SET_FALSE_RETURN();}

static void m5u_speaker_available(mrbc_vm *vm,mrbc_value v[],int argc){if(M5.Speaker.isEnabled())SET_TRUE_RETURN();else SET_FALSE_RETURN();}

static void m5u_tone(mrbc_vm *vm,mrbc_value v[],int argc){if(argc<1||!MRBC_ISNUMERIC(v[1])){SET_FALSE_RETURN();return;}uint32_t d=(argc>1&&MRBC_ISNUMERIC(v[2]))?MRBC_TO_INT(v[2]):0;if(M5.Speaker.tone(MRBC_TO_INT(v[1]),d))SET_TRUE_RETURN();else SET_FALSE_RETURN();}

static void m5u_speaker_stop(mrbc_vm *vm,mrbc_value v[],int argc){M5.Speaker.stop();SET_NIL_RETURN();}

static void m5u_speaker_volume(mrbc_vm *vm,mrbc_value v[],int argc){if(argc<1||!MRBC_ISNUMERIC(v[1])){SET_FALSE_RETURN();return;}int n=MRBC_TO_INT(v[1]);if(n<0)n=0;if(n>255)n=255;M5.Speaker.setVolume(n);SET_TRUE_RETURN();}

INT_METHOD(m5u_battery_level,M5.Power.getBatteryLevel()) INT_METHOD(m5u_battery_voltage,M5.Power.getBatteryVoltage())

INT_METHOD(m5u_battery_current,M5.Power.getBatteryCurrent()) BOOL_METHOD(m5u_charging,M5.Power.isCharging())

static void m5u_rtc_available(mrbc_vm *vm,mrbc_value v[],int argc){if(M5.Rtc.isEnabled())SET_TRUE_RETURN();else SET_FALSE_RETURN();}

static void m5u_rtc_year(mrbc_vm *vm,mrbc_value v[],int argc){auto d=M5.Rtc.getDate();SET_INT_RETURN(d.year);} static void m5u_rtc_month(mrbc_vm *vm,mrbc_value v[],int argc){auto d=M5.Rtc.getDate();SET_INT_RETURN(d.month);} static void m5u_rtc_day(mrbc_vm *vm,mrbc_value v[],int argc){auto d=M5.Rtc.getDate();SET_INT_RETURN(d.date);}

static void m5u_rtc_hour(mrbc_vm *vm,mrbc_value v[],int argc){auto t=M5.Rtc.getTime();SET_INT_RETURN(t.hours);} static void m5u_rtc_minute(mrbc_vm *vm,mrbc_value v[],int argc){auto t=M5.Rtc.getTime();SET_INT_RETURN(t.minutes);} static void m5u_rtc_second(mrbc_vm *vm,mrbc_value v[],int argc){auto t=M5.Rtc.getTime();SET_INT_RETURN(t.seconds);}

static void m5u_rtc_set(mrbc_vm *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG(
    year,
    month,
    day,
    hour,
    minute,
    second
  );

  if (!M5.Rtc.isEnabled()) {
    ESP_LOGE(
      "M5Unified",
      "RTC is not available"
    );

    SET_FALSE_RETURN();
    return;
  }

  if (!MRBC_ISNUMERIC(year) ||
      !MRBC_ISNUMERIC(month) ||
      !MRBC_ISNUMERIC(day) ||
      !MRBC_ISNUMERIC(hour) ||
      !MRBC_ISNUMERIC(minute) ||
      !MRBC_ISNUMERIC(second)) {
    ESP_LOGE(
      "M5Unified",
      "rtc_set requires all date and time values"
    );

    SET_FALSE_RETURN();
    return;
  }

  int y  = MRBC_TO_INT(year);
  int mo = MRBC_TO_INT(month);
  int d  = MRBC_TO_INT(day);
  int h  = MRBC_TO_INT(hour);
  int mi = MRBC_TO_INT(minute);
  int s  = MRBC_TO_INT(second);

  if (y < 2000 || y > 2099 ||
      mo < 1 || mo > 12 ||
      d < 1 || d > 31 ||
      h < 0 || h > 23 ||
      mi < 0 || mi > 59 ||
      s < 0 || s > 59) {
    ESP_LOGE(
      "M5Unified",
      "invalid RTC date or time"
    );

    SET_FALSE_RETURN();
    return;
  }

  m5::rtc_datetime_t datetime;

  datetime.date.year = y;
  datetime.date.month = mo;
  datetime.date.date = d;

  datetime.time.hours = h;
  datetime.time.minutes = mi;
  datetime.time.seconds = s;

  M5.Rtc.setDateTime(datetime);

  SET_TRUE_RETURN();
}

extern "C"
void mrbc_esp32_m5unified_gem_init(struct VM* vm)
{
  mrbc_class *m5_class =
    mrbc_define_class(0, "M5Unified", mrbc_class_object);
  
 
  mrbc_define_method(0, m5_class, "new",         mrbc_esp32_m5unified_new);
  mrbc_define_method(0, m5_class, "initialize",  mrbc_esp32_m5unified_initialize);
  
  mrbc_define_method(0, m5_class, "width",       mrbc_esp32_m5unified_width);
  mrbc_define_method(0, m5_class, "height",      mrbc_esp32_m5unified_height);

  mrbc_define_method(0, m5_class, "clear",       mrbc_esp32_m5unified_clear); 
  mrbc_define_method(0, m5_class, "line",        mrbc_esp32_m5unified_line);  
  mrbc_define_method(0, m5_class, "rect",        mrbc_esp32_m5unified_rect);
  mrbc_define_method(0, m5_class, "circle",      mrbc_esp32_m5unified_circle);
  mrbc_define_method(0, m5_class, "text",        mrbc_esp32_m5unified_text);
  mrbc_define_method(0, m5_class, "pixel",       mrbc_esp32_m5unified_pixel);
  mrbc_define_method(0,m5_class,"update",m5u_update);
  mrbc_define_method(0,m5_class,"board",m5u_board);
  mrbc_define_method(0,m5_class,"touch_available?",m5u_touch_available);
  mrbc_define_method(0,m5_class,"touch_read",m5u_touch_read);
  mrbc_define_method(0,m5_class,"touching?",m5u_touching);
  mrbc_define_method(0,m5_class,"touch_pressed?",m5u_touch_pressed);
  mrbc_define_method(0,m5_class,"touch_released?",m5u_touch_released);
  mrbc_define_method(0,m5_class,"touch_clicked?",m5u_touch_clicked);
  mrbc_define_method(0,m5_class,"touch_holding?",m5u_touch_holding);
  mrbc_define_method(0,m5_class,"touch_x",m5u_touch_x);
  mrbc_define_method(0,m5_class,"touch_y",m5u_touch_y);
  mrbc_define_method(0,m5_class,"touch_count",m5u_touch_count);
  mrbc_define_method(0,m5_class,"imu_available?",m5u_imu_available);
  mrbc_define_method(0,m5_class,"imu_read",m5u_imu_read);
  mrbc_define_method(0,m5_class,"imu_calibration_start",m5u_imu_calibration_start);
  mrbc_define_method(0,m5_class,"imu_calibration_update",m5u_imu_calibration_update);
  mrbc_define_method(0,m5_class,"imu_calibration_finish",m5u_imu_calibration_finish);
  mrbc_define_method(0,m5_class,"imu_calibrating?",m5u_imu_calibrating);
  mrbc_define_method(0,m5_class,"imu_calibration_load",m5u_imu_calibration_load);
  mrbc_define_method(0,m5_class,"imu_calibration_clear",m5u_imu_calibration_clear);
  mrbc_define_method(0,m5_class,"accel_available?",m5u_accel_available);
  mrbc_define_method(0,m5_class,"accel_x",m5u_ax);
  mrbc_define_method(0,m5_class,"accel_y",m5u_ay);
  mrbc_define_method(0,m5_class,"accel_z",m5u_az);
  mrbc_define_method(0,m5_class,"gyro_available?",m5u_gyro_available);
  mrbc_define_method(0,m5_class,"gyro_x",m5u_gx);
  mrbc_define_method(0,m5_class,"gyro_y",m5u_gy);
  mrbc_define_method(0,m5_class,"gyro_z",m5u_gz);
  mrbc_define_method(0,m5_class,"magnet_available?",m5u_mag_available);
  mrbc_define_method(0,m5_class,"magnet_x",m5u_mx);
  mrbc_define_method(0,m5_class,"magnet_y",m5u_my);
  mrbc_define_method(0,m5_class,"magnet_z",m5u_mz);
  mrbc_define_method(0,m5_class,"temperature",m5u_temperature);
  mrbc_define_method(0,m5_class,"button_pressed?",m5u_button_pressed);
  mrbc_define_method(0,m5_class,"button_was_pressed?",m5u_button_was_pressed);
  mrbc_define_method(0,m5_class,"button_was_released?",m5u_button_was_released);
  mrbc_define_method(0,m5_class,"button_clicked?",m5u_button_clicked);
  mrbc_define_method(0,m5_class,"button_pressed_for?",m5u_button_pressed_for);
  mrbc_define_method(0,m5_class,"speaker_available?",m5u_speaker_available);
  mrbc_define_method(0,m5_class,"tone",m5u_tone);
  mrbc_define_method(0,m5_class,"speaker_stop",m5u_speaker_stop);
  mrbc_define_method(0,m5_class,"speaker_volume=",m5u_speaker_volume);
  mrbc_define_method(0,m5_class,"battery_level",m5u_battery_level);
  mrbc_define_method(0,m5_class,"battery_voltage",m5u_battery_voltage);
  mrbc_define_method(0,m5_class,"battery_current",m5u_battery_current);
  mrbc_define_method(0,m5_class,"charging?",m5u_charging);
  mrbc_define_method(0,m5_class,"rtc_available?",m5u_rtc_available);
  mrbc_define_method(0,m5_class,"rtc_year",m5u_rtc_year);
  mrbc_define_method(0,m5_class,"rtc_month",m5u_rtc_month);
  mrbc_define_method(0,m5_class,"rtc_day",m5u_rtc_day);
  mrbc_define_method(0,m5_class,"rtc_hour",m5u_rtc_hour);
  mrbc_define_method(0,m5_class,"rtc_minute",m5u_rtc_minute);
  mrbc_define_method(0,m5_class,"rtc_second",m5u_rtc_second);
  mrbc_define_method(0,m5_class,"rtc_set",m5u_rtc_set);
}  

