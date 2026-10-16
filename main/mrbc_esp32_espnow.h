#ifndef MRB_ESP32_ESPNOW_H
#define MRB_ESP32_ESPNOW_H

#include "mrubyc.h"

// mruby/c VMへMQTTクラスを登録する初期化関数
void mrbc_esp32_espnow_gem_init(mrb_vm* vm);

#endif  // MRB_ESP32_ESPNOW_H
