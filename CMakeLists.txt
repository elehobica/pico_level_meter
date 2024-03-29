cmake_minimum_required(VERSION 3.12)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
#include($ENV{PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)

set(project_name "simple_test" C CXX ASM)
project(${project_name})
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(lib/st7735_80x160)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/level_meter.cpp
    src/conv_dB_level.cpp
)

# pull in common dependencies
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_adc
    hardware_dma
    hardware_irq
    hardware_pwm
    st7735_80x160
)

target_include_directories(${PROJECT_NAME} INTERFACE src)

# create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT_NAME})
