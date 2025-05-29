cmake_minimum_required(VERSION 3.13)

# Import SDK
include(pico_sdk_import.cmake)

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
project(lcd_i2c_project C CXX ASM)
set(PICOTOOL_FETCH_FROM_GIT_PATH "C:/pico/picotool")
set(PICO_SDK_PATH "C:/Users/thanh/pico/pico-sdk")
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(lcd_i2c_project
    main.c
    lib/lcd_i2c.c
)

target_link_libraries(lcd_i2c_project
    pico_stdlib
    hardware_i2c
)

pico_add_extra_outputs(lcd_i2c_project)
