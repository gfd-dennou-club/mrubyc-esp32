# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(main)

# ==============================================================================
# mruby/c ユーザープログラム (.rb -> .mrbc) と SPIFFS イメージの自動生成
# ==============================================================================

# 1. mrbc コマンドが環境変数(PATH)にあるか確認
find_program(MRBC_EXE mrbc)
if(NOT MRBC_EXE)
    message(FATAL_ERROR "mrbc command not found. Please ensure it is in your PATH.")
endif()

# 2. 出力先のディレクトリ作成
set(SPIFFS_DIR "${CMAKE_SOURCE_DIR}/spiffs/mrbc")
file(MAKE_DIRECTORY ${SPIFFS_DIR})

# 3. 検索対象ディレクトリの定義
# (1) プロジェクト直下の src
# (2) components 以下の各ディレクトリにある mrblib
file(GLOB SRC_RB_SOURCES "${CMAKE_SOURCE_DIR}/src/*.rb")
file(GLOB COMP_RB_SOURCES "${CMAKE_SOURCE_DIR}/components/*/mrblib/*.rb")

# 全てのソースを一つのリストに統合
set(ALL_RB_SOURCES ${SRC_RB_SOURCES} ${COMP_RB_SOURCES})
set(MRBC_OUTPUTS "")

# 4. 各 .rb ファイルに対して mrbc コンパイルコマンドを定義
foreach(rb_file ${RB_SOURCES})
    get_filename_component(name ${rb_file} NAME_WE)
    set(out_file "${SPIFFS_DIR}/${name}.mrbc")
    
    add_custom_command(
        OUTPUT ${out_file}
        COMMAND ${MRBC_EXE} -o ${out_file} ${rb_file}
        DEPENDS ${rb_file}
        COMMENT "Compiling ${name}.rb to .mrbc ..."
    )
    list(APPEND MRBC_OUTPUTS ${out_file})
endforeach()

# 5. ビルド時に必ず .mrbc を生成するターゲットを作成
add_custom_target(mrbc_build ALL DEPENDS ${MRBC_OUTPUTS})

# 6. SPIFFS イメージの生成と書き込み設定
spiffs_create_partition_image(spiffs ${SPIFFS_DIR} FLASH_IN_PROJECT)

# 7. 依存関係: SPIFFS を作る「前」に必ず mrbc のコンパイルを終わらせる
if(TARGET spiffs_spiffs_bin)
    add_dependencies(spiffs_spiffs_bin mrbc_build)
elseif(TARGET spiffs_bin)
    add_dependencies(spiffs_bin mrbc_build)
endif()
