/*! @file
  @brief
  mruby/c ADC class for ESP32
  本クラスはインスタンスを生成せず利用する
*/

#include "mrbc_esp32_adc.h"

#include "driver/adc.h"
#include "esp_adc_cal.h"

#define DEFAULT_VREF    1100
#define NO_OF_SAMPLES   64

static struct RClass* mrbc_class_esp32_adc;
static int unreferenced;

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_unit_t unit1 = ADC_UNIT_1;
static const adc_unit_t unit2 = ADC_UNIT_2;


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

/*! メソッド init_adc1(channel, atten, width)
  @param channel  ADC のチャンネル (ピンによって決まっている)
  @param atten    ADC の入力電圧の減衰率
  @param width    ADC のキャプチャ幅
*/
static void mrbc_esp32_adc_init_adc1(mrb_vm *vm, mrb_value *v, int argc){

  adc1_channel_t channel = GET_INT_ARG(1);
  adc_atten_t    atten   = GET_INT_ARG(2);
  int            width   = GET_INT_ARG(3);
   
  adc1_config_width( width );
  adc1_config_channel_atten( channel, atten);

  adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
  esp_adc_cal_characterize(unit1, atten, width, DEFAULT_VREF, adc_chars);
}

/*! メソッド read_adc1(channel)
  @param channel  ADC のチャンネル (ピンによって決まっている)
*/
static void mrbc_esp32_adc_read_adc1(mrb_vm *vm, mrb_value *v, int argc){

  uint32_t adc_reading = 0;

  adc1_channel_t channel = GET_INT_ARG(1);
  
  //Multisampling
  for (int i = 0; i < NO_OF_SAMPLES; i++) {
    adc_reading += adc1_get_raw( channel );
  }
  adc_reading /= NO_OF_SAMPLES;
  
  //Convert adc_reading to voltage in mV
  uint32_t millivolts = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);

  //値を戻す
  SET_INT_RETURN(millivolts);
}

/*! メソッド init_adc2(channel, atten, width)
  @param channel  ADC のチャンネル (ピンによって決まっている)
  @param atten    ADC の入力電圧の減衰率
  @param width    ADC のキャプチャ幅
*/
static void mrbc_esp32_adc_init_adc2(mrb_vm *vm, mrb_value *v, int argc){
  adc2_channel_t channel = GET_INT_ARG(1);
  adc_atten_t    atten   = GET_INT_ARG(2);
  int            width   = GET_INT_ARG(3);

  adc2_config_channel_atten(channel, atten);
  adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
  esp_adc_cal_characterize(unit2, atten, width, DEFAULT_VREF, adc_chars);
}

/*! メソッド read_adc2(channel, width)
  @param channel  ADC のチャンネル (ピンによって決まっている)
  @param width    ADC のキャプチャ幅
*/
static void mrbc_esp32_adc_read_adc2(mrb_vm *vm, mrb_value *v, int argc){
  uint32_t adc_reading = 0;

  adc2_channel_t channel = GET_INT_ARG(1);
  int            width   = GET_INT_ARG(2);

  for (int i = 0; i < NO_OF_SAMPLES; i++) {
    int raw;
    adc2_get_raw(channel, width, &raw);
    adc_reading += raw;
  }
  adc_reading /= NO_OF_SAMPLES;
  uint32_t millivolts = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
  SET_INT_RETURN(millivolts);
}

/*! クラス定義処理を記述した関数
  この関数を呼ぶことでクラス ADC が定義される

  @param vm mruby/c VM
*/
void
mrbc_esp32_adc_gem_init(struct VM* vm)
{
/*
ADC.init_adc1(pin)
ADC.read_adc1(pin)
ADC.init_adc2(pin)
ADC.read_adc2(pin)
*/
  // クラス ADC 定義
  mrbc_class_esp32_adc = mrbc_define_class(vm, "ADC", mrbc_class_object);

  // 各メソッド定義（mruby/c ではインスタンスメソッドをクラスメソッドとしても呼び出し可能）
  mrbc_define_method(vm, mrbc_class_esp32_adc, "init_adc1",      mrbc_esp32_adc_init_adc1);
  mrbc_define_method(vm, mrbc_class_esp32_adc, "read_adc1",      mrbc_esp32_adc_read_adc1);
  mrbc_define_method(vm, mrbc_class_esp32_adc, "init_adc2",      mrbc_esp32_adc_init_adc2);
  mrbc_define_method(vm, mrbc_class_esp32_adc, "read_adc2",      mrbc_esp32_adc_read_adc2);
  mrbc_define_method(vm, mrbc_class_esp32_adc, "nop",            mrbc_nop);
}
