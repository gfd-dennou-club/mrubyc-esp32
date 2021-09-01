#include "mrbc_esp32_tof.h"
#include "driver/gpio.h"
#include <time.h>

static struct RClass *mrbc_class_esp32_tof;

static void mrbc_millis(mrb_vm *vm, mrb_value *v, int argc)
{
  struct timespec tp;
  int n;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  n = (int)(tp.tv_sec * 1000ul + tp.tv_nsec / 1000000);
  SET_INT_RETURN(n);
}

void mrbc_mruby_esp32_tof_gem_init(struct VM *vm)
{
  mrbc_class_esp32_tof = mrbc_define_class(vm, "VL53L0X", mrbc_class_object);
  mrbc_define_method(vm, mrbc_class_esp32_tof, "millis", mrbc_millis);
}
