#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := mrubyc-esp32

# config
port0 = /dev/ttyUSB0 
#port0 = $(shell echo $(PORT))

# include
include $(IDF_PATH)/make/project.mk
include sdkconfig

# command
MRBC     = mrbc
MAKE     = make
RM       = rm
ESPTOOL  = esptool.py
MKSPIFFS = $(shell which mkspiffs|xargs -I@ basename @)
FLASH_FIRMWARE_CMD     = $(shell make print_flash_cmd)

# files
master = master.rb
slave  = slave.rb
mrbc_exist_master := $(shell find -name master.mrbc)
mrbc_exist_slave  := $(shell find -name slave.mrbc )
mrbc_exist_bin    := $(shell find -name mrbc.spiffs.bin)
slave_exist       := $(shell find -name slave.rb)
FIRMWAREDIR   = ./firmware/single/
FIRMWAREDIR_M = ./firmware/multi/

# parameters
SPIFFS_OTA_OFFSET      = $(shell awk '/otadata/ {print $$0}' partitions.csv| cut -d , -f 4)
SPIFFS_OTA0_OFFSET     = $(shell awk '/app0/    {print $$0}' partitions.csv| cut -d , -f 4)
SPIFFS_DATA_OFFSET     = $(shell awk '/spiffs/  {print $$0}' partitions.csv| cut -d , -f 4)
SPIFFS_DATA_TABLE_SIZE = $(shell awk '/spiffs/  {print $$0}' partitions.csv| cut -d , -f 5)

gems:
	python bin/make-gems.py

gems-clean:
	python bin/clean-gems.py

.PHONY: spiffs single-task multi-task flash

spiffs: spiffs-clean
	$(MRBC) -o ./spiffs/mrbc/master.mrbc -E ./mrblib/master.rb
	echo ".... compiling master.rb ...."
ifeq ("$(slave_exist)", "./mrblib/slave.rb")
	$(MRBC) -o ./spiffs/mrbc/slave.mrbc -E ./mrblib/slave.rb
	echo ".... compiling slave.rb ...."
endif
	$(MKSPIFFS) -c ./spiffs/mrbc -p 256 -b 4096 -s $(SPIFFS_DATA_TABLE_SIZE) ./spiffs/mrbc.spiffs.bin
	$(ESPTOOL) --chip esp32 --baud 921600 --port $(CONFIG_ESPTOOLPY_PORT) --before default_reset --after hard_reset write_flash -z --flash_mode qio --flash_freq 80m --flash_size detect $(SPIFFS_DATA_OFFSET) ./spiffs/mrbc.spiffs.bin


spiffs-clean:
ifeq ("$(mrbc_exist_bin)","./spiffs/mrbc.spiffs.bin")
	$(RM) ./spiffs/mrbc.spiffs.bin
	echo ".... removing mrbc.spiffs.bin ...."
endif
ifeq ("$(mrbc_exist_master)","./spiffs/mrbc/master.mrbc")
	$(RM) ./spiffs/mrbc/master.mrbc
	echo ".... removing master.mrbc ...."
endif
ifeq ("$(mrbc_exist_slave)","./spiffs/mrbc/slave.mrbc")
	$(RM) ./spiffs/mrbc/slave.mrbc
	echo ".... removing slave.mrbc ...."
endif

store-vm:
	cp build/mrubyc-esp32.bin          ${FIRMWAREDIR}
	cp build/ota_data_initial.bin      ${FIRMWAREDIR}
	cp build/partitions.bin            ${FIRMWAREDIR}
	cp build/bootloader/bootloader.bin ${FIRMWAREDIR}

store-vm-multi:
	cp build/mrubyc-esp32.bin          ${FIRMWAREDIR_M}
	cp build/ota_data_initial.bin      ${FIRMWAREDIR_M}
	cp build/partitions.bin            ${FIRMWAREDIR_M}
	cp build/bootloader/bootloader.bin ${FIRMWAREDIR_M}

flash-vm:
	$(ESPTOOL) --chip esp32 --baud 921600 --port $(port0) --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect $(SPIFFS_OTA_OFFSET) $(FIRMWAREDIR)/ota_data_initial.bin 0x1000 $(FIRMWAREDIR)/bootloader.bin $(SPIFFS_OTA0_OFFSET) $(FIRMWAREDIR)/mrubyc-esp32.bin 0x8000 $(FIRMWAREDIR)/partitions.bin

flash-vm-multi:
	$(ESPTOOL) --chip esp32 --baud 921600 --port $(port0) --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect $(SPIFFS_OTA_OFFSET) $(FIRMWAREDIR_M)/ota_data_initial.bin 0x1000 $(FIRMWAREDIR_M)/bootloader.bin $(SPIFFS_OTA0_OFFSET) $(FIRMWAREDIR_M)/mrubyc-esp32.bin 0x8000 $(FIRMWAREDIR_M)/partitions.bin
