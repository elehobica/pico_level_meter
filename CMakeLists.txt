cmake_minimum_required(VERSION 3.13)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Target to rp2350 (Raspberry Pi Pico 2)
set(PICO_PLATFORM rp2350)
set(PICO_BOARD pico2)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
#include($ENV{PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)

set(project_name "pico_level_meter" C CXX ASM)
project(${project_name})
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(lib/pico_st7735_80x160)
add_subdirectory(lib/pico_flash_param)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/level_meter.cpp
    src/conv_dB_level.cpp
    src/fm62429.cpp
)

# pull in common dependencies
target_link_libraries(${PROJECT_NAME}
    hardware_adc
    hardware_dma
    hardware_irq
    pico_stdlib
    pico_st7735_80x160
    pico_flash_param
)

target_include_directories(${PROJECT_NAME} INTERFACE
    src
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT_NAME})
