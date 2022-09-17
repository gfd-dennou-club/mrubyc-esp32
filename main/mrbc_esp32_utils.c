#include <stdio.h>

#include "mrbc_esp32_utils.h"
#include "mrubyc.h"
#include <time.h>

//================================================================
/*! cast
  ["01000101001101001110000100111100"].pack('B*').unpack('g') ができないために用意
*/
static void c_floatCast(struct VM *vm, mrbc_value *v, int argc)
{

  float Value;
  uint32_t val = 0;
  uint32_t arg1 = GET_INT_ARG(1);
  uint32_t arg2 = GET_INT_ARG(2);
  uint32_t arg3 = GET_INT_ARG(3);
  uint32_t arg4 = GET_INT_ARG(4);
  mrbc_value result;
  result = mrbc_array_new(vm, 0);

  val |= arg1;
  val <<= 8;
  val |= arg2;
  val <<= 8;
  val |= arg3;
  val <<= 8;
  val |= arg4;
  memcpy(&Value, &val, sizeof(Value));

  mrbc_array_set(&result, 0, &mrbc_fixnum_value(Value * 100.0));
  SET_RETURN(result);
  //   result = *(float*) &tempU32;
}

static void c_millis(mrb_vm *vm, mrb_value *v, int argc)
{
  struct timespec tp;
  int n;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  n = (int)(tp.tv_sec * 1000ul + tp.tv_nsec / 1000000);
  SET_INT_RETURN(n);
}


void mrbc_esp32_utils_gem_init(struct VM* vm)
{
  mrbc_define_method(0, mrbc_class_object, "floatCast", c_floatCast);
  mrbc_define_method(0, mrbc_class_object, "millis",    c_millis);
}

