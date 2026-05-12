cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(main)

# ==============================================================================
# ユーザープログラム (src/*.rb -> SPIFFS) の設定
# ==============================================================================

find_program(MRBC_EXE mrbc)
if(NOT MRBC_EXE)
    message(FATAL_ERROR "mrbc command not found.")
endif()

set(SPIFFS_DIR "${CMAKE_SOURCE_DIR}/spiffs/mrbc")
file(MAKE_DIRECTORY ${SPIFFS_DIR})

# src フォルダ内のファイルのみを個別にコンパイル
file(GLOB RB_SOURCES "${CMAKE_SOURCE_DIR}/src/*.rb")
set(MRBC_OUTPUTS "")

foreach(rb_file ${RB_SOURCES})
    get_filename_component(name ${rb_file} NAME_WE)
    set(out_file "${SPIFFS_DIR}/${name}.mrbc")
    
    add_custom_command(
        OUTPUT ${out_file}
        COMMAND ${MRBC_EXE} -o ${out_file} ${rb_file}
        DEPENDS ${rb_file}
        COMMENT "Compiling user script: ${name}.rb -> .mrbc"
    )
    list(APPEND MRBC_OUTPUTS ${out_file})
endforeach()

add_custom_target(mrbc_user_scripts ALL DEPENDS ${MRBC_OUTPUTS})

# SPIFFSイメージ作成
spiffs_create_partition_image(spiffs ${SPIFFS_DIR} FLASH_IN_PROJECT)

if(TARGET spiffs_spiffs_bin)
    add_dependencies(spiffs_spiffs_bin mrbc_user_scripts)
elseif(TARGET spiffs_bin)
    add_dependencies(spiffs_bin mrbc_user_scripts)
endif()
