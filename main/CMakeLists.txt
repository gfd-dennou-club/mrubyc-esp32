set(srcs "mrbc_esp32_gpio.c"
	 "mrbc_esp32_ledc.c"
	 "mrbc_esp32_adc.c"	 
	 "mrbc_esp32_uart.c"
	 "mrbc_esp32_i2c.c"
	 "mrbc_esp32_spi.c"
#	 "mrbc_esp32_ili934x.c"
	 "mrbc_esp32_wifi.c"
	 "mrbc_esp32_http_client.c"
	 "mrbc_esp32_sntp.c"
#	 "mrbc_esp32_dirent.c"
#	 "mrbc_esp32_stdio.c"
#	 "mrbc_esp32_sdspi.c"
	 "mrbc_esp32_sleep.c"
	 "mrbc_esp32_utils.c"
	 "mrblib.c"
	 "main.c")

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS ".")
