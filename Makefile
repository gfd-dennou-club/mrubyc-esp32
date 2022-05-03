#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := iotex-esp32-mrubyc

include $(IDF_PATH)/make/project.mk
include sdkconfig

MRBC = mrbc
ESPTOOL = esptool.py
MAKE = make
FLASH_FIRMWARE_CMD=$(shell make print_flash_cmd)
MKSPIFFS=$(shell which mkspiffs|xargs -I@ basename @)
SPIFFS_DATA_OFFSET=$(shell awk '/spiffs/ {print $$0}' partitions.csv| cut -d , -f 4)
SPIFFS_DATA_TABLE_SIZE=$(shell awk '/spiffs/ {print $$0}' partitions.csv| cut -d , -f 5)

.PHONY: spiffs
spiffs:
	$(MRBC) -o ./spiffs/mrbc/master.mrbc -E ./mrblib/loops/master.rb
ifeq ($(CONFIG_ENABLE_MULTITASK),y)
	$(MRBC) -o ./spiffs/mrbc/slave.mrbc -E ./mrblib/loops/slave.rb
else
	rm -f ./spiffs/mrbc/slave.mrbc
endif
	$(MKSPIFFS) -c ./spiffs/mrbc -p 256 -b 4096 -s $(SPIFFS_DATA_TABLE_SIZE) ./spiffs/mrbc.spiffs.bin
	$(ESPTOOL) --chip esp32 --baud 921600 --port $(CONFIG_ESPTOOLPY_PORT) --before default_reset --after hard_reset write_flash -z --flash_mode qio --flash_freq 80m --flash_size detect $(SPIFFS_DATA_OFFSET) ./spiffs/mrbc.spiffs.bin
