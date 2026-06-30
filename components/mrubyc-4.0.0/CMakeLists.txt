set(srcs "hal/esp32/hal.c"
         "src/alloc.c"
	 "src/c_array.c"
	 "src/c_hash.c"
	 "src/c_math.c"
	 "src/c_numeric.c"
	 "src/c_object.c"
 	 "src/c_proc.c"
	 "src/c_range.c"
	 "src/c_string.c"
	 "src/class.c"
	 "src/console.c"
	 "src/error.c"
	 "src/global.c"
	 "src/keyvalue.c"
	 "src/load.c"
	 "src/mrblib.c"	 
	 "src/rrt0.c"
	 "src/symbol.c"
	 "src/value.c"
	 "src/vm.c")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS src hal/esp32
                       PRIV_REQUIRES driver
                       REQUIRES esp_driver_gptimer idf_spiffs)
