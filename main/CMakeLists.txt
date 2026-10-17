# ==============================================================================
# 共通クラス (.rb -> mrblib.c) の自動生成
# ==============================================================================

# components 以下の対象となる .rb ファイルを検索
file(GLOB CLASS_FILES "${CMAKE_SOURCE_DIR}/components/*/mrblib/*.rb")

# もし対象の .rb ファイルが見つかった場合のみ、mrblib.c を生成するルールを追加
if(CLASS_FILES)
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/mrblib.c"
        COMMAND mrbc -B myclass_bytecode --remove-lv -o "${CMAKE_CURRENT_SOURCE_DIR}/mrblib.c" ${CLASS_FILES}
        DEPENDS ${CLASS_FILES}
        COMMENT "Generating mrblib.c from component ruby files..."
    )
    # CMake にこのファイルが自動生成されるものであることを教える
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/mrblib.c" PROPERTIES GENERATED TRUE)
endif()

# ==============================================================================
# メインコンポーネントのビルド設定
# ==============================================================================
set(srcs 
    "mrbc_esp32_common.c"
    "mrbc_esp32_gpio.c"
    "mrbc_esp32_ledc.c"
    "mrbc_esp32_adc.c"   
    "mrbc_esp32_uart.c"
    "mrbc_esp32_i2c.c"
    "mrbc_esp32_spi.c"
    "mrbc_esp32_lcdspi.c"
    "mrbc_esp32_sdspi.c"
    "mrbc_esp32_stdio.c"    
    "mrbc_esp32_dirent.c"   
    "mrbc_esp32_wifi.c"
    "mrbc_esp32_http_client.c"
    "mrbc_esp32_http_camera.c"
    "mrbc_esp32_sntp.c"
    "mrbc_esp32_time.c"
    "mrbc_esp32_sleep.c"
    "mrbc_esp32_utils.c"
    "mrbc_esp32_mqtt.c"
    "mrbc_esp32_espnow.c"
    "mrblib.c"
    "main.c")

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ".")
