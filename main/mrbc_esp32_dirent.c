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
static struct RClass * mrbc_class_esp32_filetime;
static mrbc_sym symid_systemtime_year;
static mrbc_sym symid_systemtime_month;
static mrbc_sym symid_systemtime_day;
static mrbc_sym symid_systemtime_dayOfWeek;
static mrbc_sym symid_systemtime_hour;
static mrbc_sym symid_systemtime_minute;
static mrbc_sym symid_systemtime_second;

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
  mrbc_dec_ref_counter(v);
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
  mrbc_dec_ref_counter(v);
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
  mrbc_dec_ref_counter(v);
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

/*! Get file created time. 
    @param name file or directory name
    @return if succeeded, return ESP32_FILETIME.
            otherwise nil.
*/
static void
mrbc_esp32_file_time(mrb_vm* vm, mrb_value* v, int argc) {
  struct stat fileInfo;
  memset(&fileInfo, 0, sizeof(struct stat));
  if(stat((char *)GET_STRING_ARG(1), &fileInfo)) {
    SET_NIL_RETURN();
  } else {
    struct tm * ptm = gmtime(&fileInfo.st_ctime);
    mrbc_value instance = mrbc_instance_new(vm, mrbc_class_esp32_filetime, 0);
    mrbc_value year = mrbc_fixnum_value(ptm->tm_year + 1900);
    mrbc_value month = mrbc_fixnum_value(ptm->tm_mon + 1);
    mrbc_value day = mrbc_fixnum_value(ptm->tm_mday);
    mrbc_value dayOfWeek = mrbc_fixnum_value(ptm->tm_wday);
    mrbc_value hour = mrbc_fixnum_value(ptm->tm_hour);
    mrbc_value minute = mrbc_fixnum_value(ptm->tm_min);
    mrbc_value second = mrbc_fixnum_value(ptm->tm_sec);
    mrbc_instance_setiv(&instance, symid_systemtime_year, &year);
    mrbc_instance_setiv(&instance, symid_systemtime_month, &month);
    mrbc_instance_setiv(&instance, symid_systemtime_day, &day);
    mrbc_instance_setiv(&instance, symid_systemtime_dayOfWeek, &dayOfWeek);
    mrbc_instance_setiv(&instance, symid_systemtime_hour, &hour);
    mrbc_instance_setiv(&instance, symid_systemtime_minute, &minute);
    mrbc_instance_setiv(&instance, symid_systemtime_second, &second);
    mrbc_dec_ref_counter(v);
    v[0] = instance;
  }
}

static void
mrbc_esp32_file_time_getYear(mrb_vm * vm, mrb_value * v, int argc) {
  SET_RETURN(mrbc_instance_getiv(v, symid_systemtime_year));
}
static void
mrbc_esp32_file_time_getMonth(mrb_vm * vm, mrb_value * v, int argc){
  SET_RETURN(mrbc_instance_getiv(v, symid_systemtime_month));
}
static void
mrbc_esp32_file_time_getDay(mrb_vm * vm, mrb_value * v, int argc) {
  SET_RETURN(mrbc_instance_getiv(v, symid_systemtime_day));
}
static void
mrbc_esp32_file_time_getDayOfWeek(mrb_vm * vm, mrb_value * v, int argc) {
  SET_RETURN(mrbc_instance_getiv(v, symid_systemtime_dayOfWeek));
}
static void
mrbc_esp32_file_time_getHour(mrb_vm * vm, mrb_value * v, int argc) {
  SET_RETURN(mrbc_instance_getiv(v, symid_systemtime_hour));
}
static void
mrbc_esp32_file_time_getMinute(mrb_vm * vm, mrb_value * v, int argc) {
  SET_RETURN(mrbc_instance_getiv(v, symid_systemtime_minute));
}
static void
mrbc_esp32_file_time_getSecond(mrb_vm * vm, mrb_value * v, int argc) {
  SET_RETURN(mrbc_instance_getiv(v, symid_systemtime_second));
}
void mrbc_esp32_dirent_gem_init(struct VM* vm)
{
  mrbc_class_esp32_dirent = mrbc_define_class(vm, "ESP32_DIRENT", mrbc_class_object);
  mrbc_define_method(vm, mrbc_class_esp32_dirent, "childrenFiles", mrbc_esp32_dir_files);
  mrbc_define_method(vm, mrbc_class_esp32_dirent, "childrenDirs", mrbc_esp32_dir_dirs);
  mrbc_define_method(vm, mrbc_class_esp32_dirent, "children", mrbc_esp32_dir_children);
  mrbc_define_method(vm, mrbc_class_esp32_dirent, "fileTime", mrbc_esp32_file_time);

  mrbc_class_esp32_filetime = mrbc_define_class(vm, "ESP32_FILETIME", mrbc_class_object);
  mrbc_define_method(vm, mrbc_class_esp32_filetime, "getYear", mrbc_esp32_file_time_getYear);
  mrbc_define_method(vm, mrbc_class_esp32_filetime, "getMonth", mrbc_esp32_file_time_getMonth);
  mrbc_define_method(vm, mrbc_class_esp32_filetime, "getDay", mrbc_esp32_file_time_getDay);
  mrbc_define_method(vm, mrbc_class_esp32_filetime, "getDayOfWeek", mrbc_esp32_file_time_getDayOfWeek);
  mrbc_define_method(vm, mrbc_class_esp32_filetime, "getHour", mrbc_esp32_file_time_getHour);
  mrbc_define_method(vm, mrbc_class_esp32_filetime, "getMinute", mrbc_esp32_file_time_getMinute);
  mrbc_define_method(vm, mrbc_class_esp32_filetime, "getSecond", mrbc_esp32_file_time_getSecond);
  
  symid_systemtime_year = str_to_symid("year");
  symid_systemtime_month = str_to_symid("month");
  symid_systemtime_day = str_to_symid("day");
  symid_systemtime_dayOfWeek = str_to_symid("dayOfWeek");
  symid_systemtime_hour = str_to_symid("hour");
  symid_systemtime_minute = str_to_symid("minute");
  symid_systemtime_second = str_to_symid("second");
}
