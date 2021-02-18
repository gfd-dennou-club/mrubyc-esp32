#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := iotex-esp32-mrubyc

include $(IDF_PATH)/make/project.mk

adc:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.adc master.rb

gpio-2:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.gpio-2 master.rb

gpio-3:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.gpio-3 master.rb

i2c:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.i2c master.rb
pwm:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.pwm master.rb

pwm2:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.pwm-2 master.rb

pwm3:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.pwm-3 master.rb

wifi:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.wifi master.rb

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

rtc:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.rtc master.rb

sht:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.sht master.rb

uart:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.uart master.rb

sample1:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.sample1 master.rb

sample2:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.sample2 master.rb

sample3:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.sample3 master.rb

sample4:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.sample4 master.rb

sample5:
	cd mrblib/loops; \
	ln -sf ../../example/master.rb.sample5 master.rb
