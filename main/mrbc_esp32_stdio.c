/*! @file
   @brief
   POSIX compatible? mruby/c stdio class for ESP32
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "esp_log.h"
#include "mrbc_esp32_stdio.h"

static FILE* fidfptable[8] = {0};
static char used = 0;

//////////////////////////////////////////////////

static void mrbc_esp32_file_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(FILE*));

  //initialize を call
  mrbc_instance_call_initialize( vm, v, argc );

  vTaskDelay(100 / portTICK_PERIOD_MS);  //wait
}

static void mrbc_esp32_file_initialize(mrbc_vm *vm, mrbc_value v[], int argc)
{
  FILE* fidfp = fopen((char *)GET_STRING_ARG(1), (char *)GET_STRING_ARG(2));

  if(fidfp == NULL)
    SET_INT_RETURN(-1); // open failed.
  else {
    *((FILE **)(v[0].instance->data)) = fidfp;
  }
}

static void
mrbc_esp32_file_close(mrb_vm* vm, mrb_value* v, int argc) {
  FILE* fidfp = *((FILE **)(v[0].instance->data));
  
  if( fclose(fidfp) ) // ERROR
    SET_FALSE_RETURN();
  else
    SET_TRUE_RETURN();
}

static void
mrbc_esp32_file_puts(mrb_vm * vm, mrb_value * v, int argc) {
  FILE* fidfp = *((FILE **)(v[0].instance->data));

  // 1. 文字列と、その長さを取得
  const char *str = (char *)GET_STRING_ARG(1);
  int len = mrbc_string_size(&v[1]);
 
  // 2. 文字列をファイルに書き込む
  if(fputs(str, fidfp) == EOF) {
    SET_FALSE_RETURN();
    return; // 失敗したらここで終了
  }

  // 3. Ruby流の puts: 末尾が \n でない場合のみ \n を追加する
  // 空文字の場合、または最後の文字が \n 以外の場合に実行
  if(len == 0 || str[len - 1] != '\n') {
    if(fputc('\n', fidfp) == EOF) {
      SET_FALSE_RETURN();
      return;
    }
  }

  SET_TRUE_RETURN();

}

static void
mrbc_esp32_file_gets(mrb_vm * vm, mrb_value * v, int argc) {
  FILE* fidfp = *((FILE **)(v[0].instance->data));
  char * buf  = (char *)malloc(sizeof(char) * 1024);

  if(fgets(buf, 1024, fidfp) == NULL) {
    SET_NIL_RETURN();
    free(buf);
    return;
  }
  mrbc_value ret = mrbc_string_new_cstr(vm, buf);
  free(buf);
  SET_RETURN( ret );
}

static void
mrbc_esp32_file_read(mrb_vm * vm, mrb_value * v, int argc) {
  FILE* fidfp = *((FILE **)(v[0].instance->data));
  char * buf = (char *)malloc(sizeof(char) * 1024);
  int read_len = fread(buf, sizeof(char), 1024, fidfp);

  if( read_len == 0) {
    SET_NIL_RETURN();
    free(buf);
    return;
  }
  mrbc_value ret = mrbc_string_new(vm, buf, sizeof(char) * read_len);
  free(buf);
  SET_RETURN( ret );
}

/*! remove compatible method. Remove file whose name is given name.
    @param filename file to be removed
    @return if succeeded, return true, otherwise false.
*/
static void
mrbc_esp32_remove(mrb_vm * vm, mrb_value * v, int argc) {
  if(remove((char *)GET_STRING_ARG(1))) {
    // error.
    SET_FALSE_RETURN();
  } else {
    SET_TRUE_RETURN();
  }
}

/*! rename compatible method. Rename files.
    @param src file to be renamed
           dst destination file name
    @return if succeeded, return true, otherwise false.
*/
static void
mrbc_esp32_rename(mrb_vm* vm, mrb_value* v, int argc) {

  if(rename((char *)GET_STRING_ARG(1), (char *)GET_STRING_ARG(2)))
    SET_FALSE_RETURN(); // rename failed.
  else
    SET_TRUE_RETURN();
}

void mrbc_esp32_stdio_gem_init(struct VM* vm) {

  //mrbc_define_class でクラス名を定義
  mrbc_class *fileIO = mrbc_define_class(vm, "File", 0);
  mrbc_define_method(vm, fileIO, "new",        mrbc_esp32_file_new);
  mrbc_define_method(vm, fileIO, "initialize", mrbc_esp32_file_initialize);
  mrbc_define_method(vm, fileIO, "open",       mrbc_esp32_file_new);
  mrbc_define_method(vm, fileIO, "close",      mrbc_esp32_file_close);
  mrbc_define_method(vm, fileIO, "puts",       mrbc_esp32_file_puts);
  mrbc_define_method(vm, fileIO, "gets",       mrbc_esp32_file_gets);
  mrbc_define_method(vm, fileIO, "read",       mrbc_esp32_file_read);
  mrbc_define_method(vm, fileIO, "delete",     mrbc_esp32_remove);
  mrbc_define_method(vm, fileIO, "rename",     mrbc_esp32_rename);
}
