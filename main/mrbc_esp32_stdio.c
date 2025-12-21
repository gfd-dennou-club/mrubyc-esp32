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

  /*  
  if(fputs((char *)GET_STRING_ARG(1), fidfp) == EOF)
    SET_FALSE_RETURN();
  else
    SET_TRUE_RETURN();
  */

  const char *str = (char *)GET_STRING_ARG(1);
  
  // 1. 文字列をファイルに書き込む
  if(fputs(str, fidfp) == EOF) {
    SET_FALSE_RETURN();
    return; // 失敗したらここで終了
  }

  // 2. 改行文字 '\n' をファイルに書き込む
  if(fputc('\n', fidfp) == EOF) {
    // 改行の書き込みに失敗した場合も false を返す
    SET_FALSE_RETURN();
  } else {
    // 文字列と改行の両方の書き込みに成功した場合のみ true を返す
    SET_TRUE_RETURN();
  }
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

//////////////////////////////////////////////////

/*! fopen compatible method. Open the file and return file id.
    (In this library, Ruby cannot have FILE pointer, beacuse FIXNUM is 31 bit(maybe...))
    @param filename File Name to open
           openmode "r" read or "w" write. If binary, append "b". If appending mode, append "+"
    @return Return file id(= FIXNUM)
            If you opened more than 8 files or fopen failed, error file id, -1.
*/
static void
mrbc_esp32_fopen(mrb_vm* vm, mrb_value* v, int argc) {
  int ret_fid = 0;
  for(; ret_fid < 8; ++ret_fid)
    if((used & (1 << ret_fid)) == 0)
      break;
  if(ret_fid == 8) {
    SET_INT_RETURN(-1); // opend file are too many.
    return;
  }
  fidfptable[ret_fid] = fopen((char *)GET_STRING_ARG(1), (char *)GET_STRING_ARG(2));
  if(fidfptable[ret_fid] == NULL)
    SET_INT_RETURN(-1); // open failed.
  else {
    used |= (1 << ret_fid);
    SET_INT_RETURN(ret_fid);
  }
}

/*! fclose compatible method. Close the file given file id.
    @param fid file id to close
    @return If succeeded, return true, otherwise false.
*/
static void
mrbc_esp32_fclose(mrb_vm* vm, mrb_value* v, int argc) {
  int fid = GET_INT_ARG(1);
  if((used & (1 << fid)) == 0) {
    SET_FALSE_RETURN(); // file is not available.
    return;
  }
  if(fclose(fidfptable[fid])) // ERROR
    SET_FALSE_RETURN();
  else
    SET_TRUE_RETURN();
  fidfptable[fid] = NULL;
  used = used & ~(1 << fid);
}

/*! fputs compatible method. Write down the string onto given file.
    @param  fid file id to puts
            content content to write
    @return If succeeded, return true, otherwise false.
*/
static void
mrbc_esp32_fputs(mrb_vm * vm, mrb_value * v, int argc) {
  int fid = GET_INT_ARG(1);
  if((used & (1 << fid)) == 0) {
    SET_FALSE_RETURN(); // file is not available.
    return;
  }
  if(fputs((char *)GET_STRING_ARG(2), fidfptable[fid]) == EOF)
    SET_FALSE_RETURN();
  else
    SET_TRUE_RETURN();
}


/*! fwrite compatible method. Write down the binary array onto given file.
    @param  fid file id to puts
            content content to write
            size length of element
            nmemb count of element
    @return If succeeded, return true, otherwise false.
*/
static void
mrbc_esp32_fwrite(mrb_vm * vm, mrb_value * v, int argc) {
  int fid = GET_INT_ARG(1);
  if((used & (1 << fid)) == 0) {
    SET_FALSE_RETURN(); // file is not available.
    return;
  }
  if(fwrite((char *)GET_STRING_ARG(2), GET_INT_ARG(3), GET_INT_ARG(4), fidfptable[fid]) == EOF)
    SET_FALSE_RETURN();
  else
    SET_TRUE_RETURN();
}

/*! fputc compatible method. Write down one charactor onto given file.
    @param fid file id to putc
           ch character to write
    @return If succeeded, return true, otherwise false.
*/
static void
mrbc_esp32_fputc(mrb_vm * vm, mrb_value * v, int argc) {
  int fid = GET_INT_ARG(1);
  if((used & (1 << fid)) == 0) {
    SET_FALSE_RETURN(); // file is not available.
    return;
  }
  if(fputc((char)(GET_STRING_ARG(2))[0], fidfptable[fid]) == EOF)
    SET_FALSE_RETURN();
  else
    SET_TRUE_RETURN();
}

/*! fgets compatible method. Read from given file.
    @param fid file id to gets
           length read size
    @return If succeeded, return true, otherwise false.
*/
static void
mrbc_esp32_fgets(mrb_vm * vm, mrb_value * v, int argc) {
  int fid = GET_INT_ARG(1);
  if((used & (1 << fid)) == 0) {
    SET_NIL_RETURN(); // file is not available.
    return;
  }
  char * buf = (char *)malloc(sizeof(char) * GET_INT_ARG(2));
  if(fgets(buf, GET_INT_ARG(2), fidfptable[fid]) == NULL) {
    SET_NIL_RETURN();
    free(buf);
    return;
  }
  mrbc_value ret = mrbc_string_new_cstr(vm, buf);
  free(buf);
  SET_RETURN( ret );
}

/*! fread compatible method. Read from given file.
    @param fid file id to gets
           size length of element
           nmemb count of element
    @return If succeeded, return true, otherwise false.
*/
static void
mrbc_esp32_fread(mrb_vm * vm, mrb_value * v, int argc) {
  int fid = GET_INT_ARG(1);
  if((used & (1 << fid)) == 0) {
    SET_NIL_RETURN(); // file is not available.
    return;
  }
  char * buf = (char *)malloc(GET_INT_ARG(2) * GET_INT_ARG(3));
  int read_len = fread(buf, GET_INT_ARG(2), GET_INT_ARG(3), fidfptable[fid]);
  if( read_len == 0) {
    SET_NIL_RETURN();
    free(buf);
    return;
  }
  mrbc_value ret = mrbc_string_new(vm, buf, GET_INT_ARG(2) * read_len);
  free(buf);
  SET_RETURN( ret );
}

/*! fgetc compatible method. Read one charactor from given file.
    @param fid file id to gets
    @return If succeeded, return true, otherwise nil.
*/
static void
mrbc_esp32_fgetc(mrb_vm * vm, mrb_value *v, int argc) {
  int fid = GET_INT_ARG(1);
  if((used & (1 << fid)) == 0) {
    SET_NIL_RETURN(); // file is not available.
    return;
  }
  mrbc_value ret = mrbc_string_new(vm, NULL, 1);
  ret.string->data[0] = (uint8_t)fgetc(fidfptable[fid]);
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

/*! fflush compatible method. Flush pending data immediately.
    @param fid file id to be flushed
    @return If succeeded, return true, otherwise false.
*/
static void
mrbc_esp32_fflush(mrb_vm * vm, mrb_value * v, int argc) {
  int fid = GET_INT_ARG(1);
  if((used & (1 << fid)) == 0) {
    SET_FALSE_RETURN(); // file is not available.
    return;
  }
  if(fflush(fidfptable[fid])) {
    // error.
    SET_FALSE_RETURN();
  } else {
    SET_TRUE_RETURN();
  }
}

/*! fgetpos compatible method. Get position of cursor.
    @param fid file id to get position
    @return If succeeded, return position of cursor, otherwise nil.
*/
static void
mrbc_esp32_fgetpos(mrb_vm * vm, mrb_value * v, int argc) {
  int fid = GET_INT_ARG(1);
  if((used & (1 << fid)) == 0) {
    SET_NIL_RETURN(); // file is not available.
    return;
  }
  fpos_t ret;
  if(fgetpos(fidfptable[fid], &ret))
    SET_NIL_RETURN();
  else
    SET_INT_RETURN(ret);
}

/*! fsetpos compatible method. Set position of cursor.
    @param fid file id to set position
    @return If succeeded, return true, otherwise false.
*/
static void
mrbc_esp32_fsetpos(mrb_vm * vm, mrb_value * v, int argc) {
  int fid = GET_INT_ARG(1);
  if((used & (1 << fid)) == 0) {
    SET_FALSE_RETURN(); // file is not available.
    return;
  }
  fpos_t val = (fpos_t)GET_INT_ARG(2);
  if(fsetpos(fidfptable[fid], &val))
    SET_FALSE_RETURN();
  else
    SET_TRUE_RETURN();
}

/*! fseek compatible method. Seek position of cursor.
    @param fid file id to seek cursor
           origin SEEK_SET(0) or SEEK_CUR(1) or SEEK_END(2)
                  from head   or from cursor or from end of file
    @return If succeeded, return true, otherwise false.
*/
static void
mrbc_esp32_fseek(mrb_vm * vm, mrb_value * v, int argc) {
  int fid = GET_INT_ARG(1);
  if((used & (1 << fid)) == 0) {
    SET_FALSE_RETURN(); // file is not available.
    return;
  }
  if(fseek(fidfptable[fid], GET_INT_ARG(2), GET_INT_ARG(3)))
    SET_FALSE_RETURN();
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

  mrbc_class *stdio = mrbc_define_class(vm, "STDIO", 0);
  mrbc_define_method(vm, stdio, "fopen", mrbc_esp32_fopen);
  mrbc_define_method(vm, stdio, "fputs", mrbc_esp32_fputs);
  mrbc_define_method(vm, stdio, "fputc", mrbc_esp32_fputc);
  mrbc_define_method(vm, stdio, "fgets", mrbc_esp32_fgets);
  mrbc_define_method(vm, stdio, "fgetc", mrbc_esp32_fgetc);
  mrbc_define_method(vm, stdio, "fflush", mrbc_esp32_fflush);
  mrbc_define_method(vm, stdio, "fgetpos", mrbc_esp32_fgetpos);
  mrbc_define_method(vm, stdio, "fsetpos", mrbc_esp32_fsetpos);
  mrbc_define_method(vm, stdio, "fseek", mrbc_esp32_fseek);
  mrbc_define_method(vm, stdio, "remove", mrbc_esp32_remove);
  mrbc_define_method(vm, stdio, "fclose", mrbc_esp32_fclose);
  mrbc_define_method(vm, stdio, "rename", mrbc_esp32_rename);
  mrbc_define_method(vm, stdio, "fread", mrbc_esp32_fread);
  mrbc_define_method(vm, stdio, "fwrite", mrbc_esp32_fwrite);
}
