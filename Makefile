#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := mrubyc-esp32

# config
ifeq ("$(PORT)","")
PORT0 = /dev/ttyUSB0 
else
PORT0 = $(shell echo $(PORT))
endif

ifeq ("$(BAUD)","")
BAUD0 = 921600
else
BAUD0 = $(shell echo $(BAUD))
endif

# include
ifneq ("$(IDF_PATH)","")
include $(IDF_PATH)/make/project.mk
endif

# command
MRBC     = mrbc
MAKE     = make
RM       = rm
ESPTOOL  = esptool.py
MKSPIFFS = $(shell which mkspiffs|xargs -I@ basename @)
FLASH_FIRMWARE_CMD = $(shell make print_flash_cmd)

# files
master = master.rb
slave  = slave.rb
FIRMWAREDIR = firmware

# parameters
SPIFFS_OTA_OFFSET      = $(shell awk '/otadata/ {print $$0}' partitions.csv| cut -d , -f 4)
SPIFFS_OTA0_OFFSET     = $(shell awk '/app0/    {print $$0}' partitions.csv| cut -d , -f 4)
SPIFFS_DATA_OFFSET     = $(shell awk '/spiffs/  {print $$0}' partitions.csv| cut -d , -f 4)
SPIFFS_DATA_TABLE_SIZE = $(shell awk '/spiffs/  {print $$0}' partitions.csv| cut -d , -f 5)

gems: mrblib
	python bin/make-gems.py

gems-clean:
	python bin/clean-gems.py

.PHONY: spiffs spiffs-clean spiffs-monitor store-vm flash-vm 

spiffs: 
	@echo $(MRBC) -o ./spiffs/mrbc/master.mrbc -E ./src/master.rb
	$(MRBC) -o ./spiffs/mrbc/master.mrbc -E ./src/master.rb
	@echo $(MRBC) -o ./spiffs/mrbc/slave.mrbc -E ./src/slave.rb
	$(MRBC) -o ./spiffs/mrbc/slave.mrbc -E ./src/slave.rb
	@echo $(MKSPIFFS) -c ./spiffs/mrbc -p 256 -b 4096 -s $(SPIFFS_DATA_TABLE_SIZE) ./spiffs/mrbc.spiffs.bin
	$(MKSPIFFS) -c ./spiffs/mrbc -p 256 -b 4096 -s $(SPIFFS_DATA_TABLE_SIZE) ./spiffs/mrbc.spiffs.bin
	@echo $(ESPTOOL) --chip esp32 --baud $(BAUD0) --port $(PORT0) --before default_reset --after hard_reset write_flash -z --flash_mode qio --flash_freq 80m --flash_size detect $(SPIFFS_DATA_OFFSET) ./spiffs/mrbc.spiffs.bin
	$(ESPTOOL) --chip esp32 --baud $(BAUD0) --port $(PORT0) --before default_reset --after hard_reset write_flash -z --flash_mode qio --flash_freq 80m --flash_size detect $(SPIFFS_DATA_OFFSET) ./spiffs/mrbc.spiffs.bin

spiffs-clean:
	@echo $(RM) ./spiffs/mrbc.spiffs.bin
	$(RM) ./spiffs/mrbc.spiffs.bin
	@echo $(RM) ./spiffs/mrbc/master.mrbc
	$(RM) ./spiffs/mrbc/master.mrbc
	@echo $(RM) ./spiffs/mrbc/slave.mrbc
	$(RM) ./spiffs/mrbc/slave.mrbc

spiffs-monitor:
	python bin/idf_monitor.py --port $(PORT0) firmware/mrubyc-esp32.elf

store-vm:
	cp build/mrubyc-esp32.bin          ${FIRMWAREDIR}
	cp build/mrubyc-esp32.elf          ${FIRMWAREDIR}
	cp build/ota_data_initial.bin      ${FIRMWAREDIR}
	cp build/partitions.bin            ${FIRMWAREDIR}
	cp build/bootloader/bootloader.bin ${FIRMWAREDIR}

flash-vm:
	$(ESPTOOL) --chip esp32 --baud $(BAUD0) --port $(PORT0) --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect $(SPIFFS_OTA_OFFSET) $(FIRMWAREDIR)/ota_data_initial.bin 0x1000 $(FIRMWAREDIR)/bootloader.bin $(SPIFFS_OTA0_OFFSET) $(FIRMWAREDIR)/mrubyc-esp32.bin 0x8000 $(FIRMWAREDIR)/partitions.bin
