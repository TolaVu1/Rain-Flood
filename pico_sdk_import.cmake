set(PICO_SDK_PATH "C:/Users/thanh/pico/pico-sdk" CACHE PATH "Path to pico-sdk")

if(NOT EXISTS "${PICO_SDK_PATH}")
    message(FATAL_ERROR "Directory '${PICO_SDK_PATH}' not found. Please check the path.")
endif()

include("${PICO_SDK_PATH}/pico_sdk_init.cmake")
