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
BAUD0 = 115200
else
BAUD0 = $(shell echo $(BAUD))
endif

# command
MRBC     = mrbc
RM       = rm
CP       = cp
ESPTOOL  = esptool.py
IDFTOOL  = idf.py
MKSPIFFS = $(shell which mkspiffs|xargs -I@ basename @)

# parameters
SPIFFS_OTA_OFFSET      = $(shell awk '/otadata/ {print $$0}' partitions.csv| cut -d , -f 4)
SPIFFS_OTA0_OFFSET     = $(shell awk '/app0/    {print $$0}' partitions.csv| cut -d , -f 4)
SPIFFS_DATA_OFFSET     = $(shell awk '/spiffs/  {print $$0}' partitions.csv| cut -d , -f 4)
SPIFFS_DATA_TABLE_SIZE = $(shell awk '/spiffs/  {print $$0}' partitions.csv| cut -d , -f 5)

PROJECT_PATH = .

SRCDIR   = $(PROJECT_PATH)/src
SRCFILES = $(wildcard $(SRCDIR)/*.rb)
OBJS     = $(patsubst %.rb,%.h,$(SRCFILES))

CLASSDIR   = $(PROJECT_PATH)/components/*/mrblib
CLASSFILES = $(wildcard $(CLASSDIR)/*.rb)
MYCLASS    = myclass_bytecode

MAINDIR  = $(PROJECT_PATH)/main
FIRMWAREDIR = $(PROJECT_PATH)/firmware
SPIFFSDIR = $(PROJECT_PATH)/spiffs/mrbc
SPIFFSFILE = $(PROJECT_PATH)/spiffs/mrbc.spiffs.bin

.PHONY: spiffs flash monitor store-vm 

all: $(OBJS)
ifneq ("$(CLASSFILES)", "")
	$(MRBC) -B $(MYCLASS) --remove-lv -o $(PROJECT_PATH)/main/mrblib.c  $(CLASSFILES)
endif
	$(IDFTOOL) build

flash: all
	$(IDFTOOL) flash --baud $(BAUD0) --port $(PORT0)

monitor:
	$(IDFTOOL) monitor --monitor-baud $(BAUD0) --port $(PORT0)

clean:
	$(IDFTOOL) fullclean

$(SRCDIR)/%.h: $(SRCDIR)/%.rb
	$(MRBC) -B $(basename $(notdir $@)) -o $(MAINDIR)/$(basename $(notdir $@)).h $^
	$(MRBC) -o $(SPIFFSDIR)/$(basename $(notdir $@)).mrbc $^

gems: 
	python bin/make-gems.py

gems-clean:
	python bin/clean-gems.py

spiffs:  $(OBJS)
	$(MKSPIFFS) -c $(SPIFFSDIR) -p 256 -b 4096 -s $(SPIFFS_DATA_TABLE_SIZE) $(SPIFFSFILE)
	$(ESPTOOL) --chip esp32 --baud $(BAUD0) --port $(PORT0) --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect $(SPIFFS_DATA_OFFSET) $(SPIFFSFILE)

store-vm:
	$(CP) build/main.bin                               ${FIRMWAREDIR}
	$(CP) build/ota_data_initial.bin                   ${FIRMWAREDIR}
	$(CP) build/partition_table/partition-table.bin    ${FIRMWAREDIR}
	$(CP) build/bootloader/bootloader.bin              ${FIRMWAREDIR}

menuconfig:
	$(IDFTOOL) menuconfig
