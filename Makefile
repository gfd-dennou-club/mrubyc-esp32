#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := iotex-esp32-mrubyc

include $(IDF_PATH)/make/project.mk
include sdkconfig
adc:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.adc master.rb

gpio:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.gpio-1 master.rb

gpio2:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.gpio-2 master.rb

gpio3:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.gpio-3 master.rb

gpio4:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.gpio-4 master.rb

i2c:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.i2c master.rb

pwm:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.pwm-1 master.rb

pwm2:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.pwm-2 master.rb

pwm3:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.pwm-3 master.rb

pwm4:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.pwm-4 master.rb

pwm5:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.pwm-5 master.rb

pwm6:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.pwm-6 master.rb

wifi:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.wifi master.rb

wifi_simple:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.wifi_simple master.rb

co2:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.co2 master.rb

http:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.httpclient master.rb

sdcard:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.sdcard master.rb

sgp30:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.sgp30 master.rb

tmp007:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.tmp007 master.rb

rtc:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.rtc master.rb

sht:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.sht master.rb

sleep:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.sleep master.rb

uart:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.uart master.rb

m5display:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.m5display master.rb

vl53l0x:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.vl53l0x-1 master.rb

vl53l0x2:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.vl53l0x-2 master.rb

env2:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.env2 master.rb

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
