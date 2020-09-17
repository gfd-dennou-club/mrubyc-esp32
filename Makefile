#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := iotex-esp32-mrubyc

include $(IDF_PATH)/make/project.mk

adc:
	ln -sf master.rb.adc ./example/ja/master.rb

gpio-1:
	ln -sf master.rb.gpio-1 ./example/ja/master.rb

gpio-2:
	ln -sf master.rb.gpio-2 ./example/ja/master.rb

i2c:
	ln -sf master.rb.i2c ./example/ja/master.rb

pwm:
	ln -sf master.rb.pwm ./example/ja/master.rb

wifi:
	ln -sf master.rb.wifi ./example/ja/master.rb

adc:
	ln -sf master.rb.adc ./example/ja/master.rb

gpio-1:
	ln -sf master.rb.gpio-1 ./example/en/master.rb

gpio-2:
	ln -sf master.rb.gpio-2 ./example/en/master.rb

i2c:
	ln -sf master.rb.i2c ./example/en/master.rb

pwm:
	ln -sf master.rb.pwm ./example/en/master.rb

wifi:
	ln -sf master.rb.wifi ./example/en/master.rb
