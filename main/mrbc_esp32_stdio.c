/*! @file
   @brief
   POSIX compatible? mruby/c stdio class for ESP32
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "esp_log.h"
#include "mrbc_esp32_stdio.h"

static struct RClass * mrbc_class_esp32_stdio;

static FILE* fidfptable[8] = {0};
static char used = 0;

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
  mrbc_dec_ref_counter(v);
  v[0] = ret;
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
  mrbc_dec_ref_counter(v);
  v[0] = ret;
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
  mrbc_dec_ref_counter(v);
  v[0] = ret;
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
  mrbc_class_esp32_stdio = mrbc_define_class(vm, "ESP32_STDIO", mrbc_class_object);
  mrbc_define_method(vm, mrbc_class_esp32_stdio, "fopen", mrbc_esp32_fopen);
  mrbc_define_method(vm, mrbc_class_esp32_stdio, "fputs", mrbc_esp32_fputs);
  mrbc_define_method(vm, mrbc_class_esp32_stdio, "fputc", mrbc_esp32_fputc);
  mrbc_define_method(vm, mrbc_class_esp32_stdio, "fgets", mrbc_esp32_fgets);
  mrbc_define_method(vm, mrbc_class_esp32_stdio, "fgetc", mrbc_esp32_fgetc);
  mrbc_define_method(vm, mrbc_class_esp32_stdio, "fflush", mrbc_esp32_fflush);
  mrbc_define_method(vm, mrbc_class_esp32_stdio, "fgetpos", mrbc_esp32_fgetpos);
  mrbc_define_method(vm, mrbc_class_esp32_stdio, "fsetpos", mrbc_esp32_fsetpos);
  mrbc_define_method(vm, mrbc_class_esp32_stdio, "fseek", mrbc_esp32_fseek);
  mrbc_define_method(vm, mrbc_class_esp32_stdio, "remove", mrbc_esp32_remove);
  mrbc_define_method(vm, mrbc_class_esp32_stdio, "fclose", mrbc_esp32_fclose);
  mrbc_define_method(vm, mrbc_class_esp32_stdio, "rename", mrbc_esp32_rename);
  mrbc_define_method(vm, mrbc_class_esp32_stdio, "fread", mrbc_esp32_fread);
  mrbc_define_method(vm, mrbc_class_esp32_stdio, "fwrite", mrbc_esp32_fwrite);
}
