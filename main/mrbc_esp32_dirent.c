/*! @file
   @brief
   mruby/c dirent class for ESP32
*/

#include "dirent.h"
#include "sys/stat.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "esp_log.h"
#include "mrbc_esp32_dirent.h"

static struct RClass * mrbc_class_esp32_dirent;

/*! Enumearte files contained selected directory.
    @param directryname Selecting Directory Name
    @return if succeeded, return String Array contains file names.
            otherwise nil.
 */
static void
mrbc_esp32_dir_files(mrb_vm * vm, mrb_value * v, int argc) {
  DIR * dir = NULL;
  struct dirent * dirInfo;
  dir = opendir((char *)GET_STRING_ARG(1));
  if(!dir) {
    SET_NIL_RETURN();
    return;
  }
  uint16_t fileCount = 0;
  while(1) {
    dirInfo = readdir(dir);
    if(dirInfo == NULL) break;
    if(dirInfo->d_type == DT_REG)
      fileCount++;
  }
  //  mrbc_dec_ref_counter(v);
  mrbc_decref_empty(v);
  v[0] = mrbc_array_new(vm, (int)fileCount);
  if(fileCount <= 0)
    return;
  rewinddir(dir);
  for(int i = 0; i < fileCount;) {
    dirInfo = readdir(dir); 
    if(dirInfo == NULL) break; // ERROR!
    if(dirInfo->d_type != DT_REG)
      continue;
    mrbc_value val = mrbc_string_new_cstr(vm, dirInfo->d_name);
    mrbc_array_set(v, i, &val);
    i++;
  }
}

/*! Enumearte directories contained selected directory.
    @param directryname Selecting Directory Name
    @return if succeeded, return String Array contains directory names.
            otherwise nil.
*/
static void
mrbc_esp32_dir_dirs(mrb_vm * vm, mrb_value * v, int argc) {
  DIR * dir = NULL;
  struct dirent * dirInfo;
  dir = opendir((char *)GET_STRING_ARG(1));
  if(!dir) {
    SET_NIL_RETURN();
    return;
  }
  uint16_t dirCount = 0;
  while(1) {
    dirInfo = readdir(dir);
    if(dirInfo == NULL) break;
    if(dirInfo->d_type == DT_DIR)
      dirCount++;
  }
  mrbc_decref_empty(v);
  //  mrbc_dec_ref_counter(v);
  v[0] = mrbc_array_new(vm, (int)dirCount);
  if(dirCount <= 0)
    return;
  rewinddir(dir);
  for(int i = 0; i < dirCount;) {
    dirInfo = readdir(dir); 
    if(dirInfo == NULL) break; // ERROR!
    if(dirInfo->d_type != DT_DIR)
      continue;
    mrbc_value val = mrbc_string_new_cstr(vm, dirInfo->d_name);
    mrbc_array_set(v, i, &val);
    i++;
  }
}

/*! Enumearte files and directories (= entry) contained selected directory.
    @param directryname Selecting Directory Name
    @return if succeeded, return String Array contains entry names.
            otherwise nil.
*/
static void
mrbc_esp32_dir_children(mrb_vm * vm, mrb_value * v, int argc) {
  DIR * dir = NULL;
  struct dirent * dirInfo;
  dir = opendir((char *)GET_STRING_ARG(1));
  if(!dir) {
    SET_NIL_RETURN();
    return;
  }
  uint16_t childCount = 0;
  while(1) {
    dirInfo = readdir(dir);
    if(dirInfo == NULL) break;
    childCount++;
  }
  mrbc_decref_empty(v);  
  //  mrbc_dec_ref_counter(v);
  v[0] = mrbc_array_new(vm, (int)childCount);
  if(childCount <= 0)
    return;
  rewinddir(dir);
  for(int i = 0; i < childCount;) {
    dirInfo = readdir(dir); 
    if(dirInfo == NULL) break; // ERROR!
    mrbc_value val = mrbc_string_new_cstr(vm, dirInfo->d_name);
    mrbc_array_set(v, i, &val);
    i++;
  }
}

void mrbc_esp32_dirent_gem_init(struct VM* vm)
{
  mrbc_class_esp32_dirent = mrbc_define_class(vm, "Dir", mrbc_class_object);
  mrbc_define_method(vm, mrbc_class_esp32_dirent, "childrenFiles", mrbc_esp32_dir_files);
  mrbc_define_method(vm, mrbc_class_esp32_dirent, "childrenDirs", mrbc_esp32_dir_dirs);
  mrbc_define_method(vm, mrbc_class_esp32_dirent, "children", mrbc_esp32_dir_children);
}
