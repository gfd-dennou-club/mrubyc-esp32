#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := iotex-esp32-mrubyc

include $(IDF_PATH)/make/project.mk

adc-ja:
	cd mrblib/loops; \
	ln -sf ../../example/ja/master.rb.adc master.rb

gpio-1-ja:
	cd mrblib/loops; \
	ln -sf ../../example/ja/master.rb.gpio-1 master.rb

gpio-2-ja:
	cd mrblib/loops; \
	ln -sf ../../example/ja/master.rb.gpio-2 master.rb

i2c-ja:
	cd mrblib/loops; \
	ln -sf ../../example/ja/master.rb.2c master.rb

pwm-ja:
	cd mrblib/loops; \
	ln -sf ../../example/ja/master.rb.pwm master.rb

wifi-ja:
	cd mrblib/loops; \
	ln -sf ../../example/ja/master.rb.wifi master.rb

adc:
	cd mrblib/loops; \
	ln -sf ../../example/en/master.rb.adc master.rb

gpio-1:
	cd mrblib/loops; \
	ln -sf ../../example/en/master.rb.gpio-1 master.rb

gpio-2:
	cd mrblib/loops; \
	ln -sf ../../example/en/master.rb.gpio-2 master.rb

i2c:
	cd mrblib/loops; \
	ln -sf ../../example/en/master.rb.i2c master.rb
pwm:
	cd mrblib/loops; \
	ln -sf ../../example/en/master.rb.pwm master.rb

wifi:
	cd mrblib/loops; \
	ln -sf ../../example/en/master.rb.wifi master.rb
