#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := iotex-esp32-mrubyc

include $(IDF_PATH)/make/project.mk

adc:
	ln -sf master.rb.adc ./mrblib/loops/master.rb

gpio-1:
	ln -sf master.rb.gpio-1 ./mrblib/loops/master.rb

gpio-2:
	ln -sf master.rb.gpio-2 ./mrblib/loops/master.rb

i2c:
	ln -sf master.rb.i2c ./mrblib/loops/master.rb

pwm:
	ln -sf master.rb.pwm ./mrblib/loops/master.rb

wifi:
	ln -sf master.rb.wifi ./mrblib/loops/master.rb
