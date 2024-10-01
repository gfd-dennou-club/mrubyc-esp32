#set(CMAKE_CFLAGS -mlongcalls -DMRBC_USE_HAL_ESP32)
#set(CMAKE_C_FLAGS -mlongcalls)

set(srcs "src/alloc.c"
	 "src/c_array.c"
	 "src/c_hash.c"
	 "src/c_math.c"
	 "src/c_numeric.c"
	 "src/c_object.c"
	 "src/c_range.c"
	 "src/c_string.c"
	 "src/class.c"
	 "src/console.c"
	 "src/error.c"
	 "src/global.c"
	 "src/keyvalue.c"
	 "src/load.c"
	 "src/mrblib.c"
	 "src/hal.c"	 
	 "src/rrt0.c"
	 "src/symbol.c"
	 "src/value.c"
	 "src/vm.c")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS src
                       PRIV_INCLUDE_DIRS src
                       PRIV_REQUIRES driver
                       REQUIRES idf_spiffs)

#idf_component_register(SRCS ${SRCS}
#                       INCLUDE_DIRS "."
#                       PRIV_REQUIRES app_update spi_flash log)

#add_custom_command(TARGET ${SRCS}
#                  POST_BUILD
#                  COMMAND make -f src/Makefile )



#add_custom_target(mrubyc make -f src/Makefile )
#    WORKING_DIRECTORY "."
#)

#idf_component_register(SRCS "${srcs}"
#                       INCLUDE_DIRS "src"
#		       )
		       
#                       PRIV_REQUIRES driver
#                       LDFRAGMENTS linker.lf)

#idf_component_register(SRCS "${component_srcs}"
#                       INCLUDE_DIRS "include"
#                       PRIV_INCLUDE_DIRS ""
#                       PRIV_REQUIRES "driver"
#                       REQUIRES "")

#idf_component_register(SRCS "${srcs}"
#                    INCLUDE_DIRS  "."
#                    REQUIRES console nvs_flash bt
#                    REQUIRED_IDF_TARGETS esp32)
