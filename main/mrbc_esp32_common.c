/*! @file
  @brief
  mruby/c I2C class for ESP32
  initialize メソッドは Ruby コード側で定義する
*/

#include "esp_err.h"
#include "esp_log.h"
#include "mrubyc.h"

//================================================================
/*! make output buffer
// ITOC 東さんのソースコードをコピー

  @param vm     Pointer to vm
  @param v      argments
  @param argc   num of arguments
  @param start_idx  Argument parsing start position.
  @param ret_bufsiz allocated buffer size.
  @return       pointer to allocated buffer, or NULL is error.
*/

uint8_t * make_output_buffer(mrb_vm *vm, mrb_value v[], int argc,
                             int start_idx, int *ret_bufsiz)
{
  uint8_t *ret = 0;

  // calc temporary buffer size.
  int bufsiz = 0;
  for( int i = start_idx; i <= argc; i++ ) {
    switch( v[i].tt ) {
    case MRBC_TT_INTEGER:
      bufsiz += 1;
      break;

    case MRBC_TT_STRING:
      bufsiz += mrbc_string_size(&v[i]);
      break;

    case MRBC_TT_ARRAY:
      bufsiz += mrbc_array_size(&v[i]);
      break;

    default:
      goto ERROR_PARAM;
    }
  }
  *ret_bufsiz = bufsiz;
  if( bufsiz == 0 ) goto ERROR_PARAM;

  // alloc buffer and copy data
  ret = mrbc_alloc(vm, bufsiz);
  uint8_t *pbuf = ret;
  for( int i = start_idx; i <= argc; i++ ) {
    switch( v[i].tt ) {
    case MRBC_TT_INTEGER:
      *pbuf++ = mrbc_integer(v[i]);
      break;

    case MRBC_TT_STRING:
      memcpy( pbuf, mrbc_string_cstr(&v[i]), mrbc_string_size(&v[i]) );
      pbuf += mrbc_string_size(&v[i]);
      break;

    case MRBC_TT_ARRAY: {
      for( int j = 0; j < mrbc_array_size(&v[i]); j++ ) {
        mrbc_value val = mrbc_array_get(&v[i], j);
        if( val.tt != MRBC_TT_INTEGER ) goto ERROR_PARAM;
        *pbuf++ = mrbc_integer(val);
      }
    } break;

    default:
      //
    }
  }
  return ret;

 ERROR_PARAM:
  mrbc_raise(vm, MRBC_CLASS(ArgumentError), "Output parameter error.");
  if( ret != 0 ) {
    mrbc_free( vm, ret );
  }

  return 0;
}

