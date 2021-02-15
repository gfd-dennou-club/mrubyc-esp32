#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := iotex-esp32-mrubyc

include $(IDF_PATH)/make/project.mk

adc-ja:
	cd mrblib/loops
	pwd
	ln -sf ../../example/ja/master.rb.adc master.rb
	cd ../..

gpio-1-ja:
	cd mrblib/loops
	ln -sf ../../example/ja/master.rb.gpio-1 master.rb
	cd ../..

gpio-2-ja:
	cd mrblib/loops
	ln -sf ../../example/ja/master.rb.gpio-2 master.rb
	cd ../..

i2c-ja:
	cd mrblib/loops
	ln -sf ../../example/ja/master.rb.2c master.rb
	cd ../..

pwm-ja:
	cd mrblib/loops
	ln -sf ../../example/ja/master.rb.pwm master.rb
	cd ../..

wifi-ja:
	cd mrblib/loops
	ln -sf ../../example/ja/master.rb.wifi master.rb
	cd ../..

adc:
	cd mrblib/loops
	ln -sf ../../example/en/master.rb.adc master.rb
	cd ../..

gpio-1:
	cd mrblib/loops
	ln -sf ../../example/en/master.rb.gpio-1 master.rb
	cd ../..

gpio-2:
	cd mrblib/loops
	ln -sf ../../example/en/master.rb.gpio-2 master.rb
	cd ../..

i2c:
	cd mrblib/loops
	ln -sf ../../example/en/master.rb.i2c master.rb
	cd ../..

pwm:
	cd mrblib/loops
	ln -sf ../../example/en/master.rb.pwm master.rb
	cd ../..

wifi:
	cd mrblib/loops
	ln -sf ../../example/en/master.rb.wifi master.rb
	cd ../..
