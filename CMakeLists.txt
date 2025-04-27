cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico_w CACHE STRING "Board type")
include(pico_sdk_import.cmake)
project(OhmimetroV2 C CXX ASM)

pico_sdk_init()

add_executable(OhmimetroV2 OhmimetroV2.c lib/ssd1306.c)

pico_set_program_name(OhmimetroV2 "OhmimetroV2")
pico_set_program_version(OhmimetroV2 "0.1")

pico_generate_pio_header(OhmimetroV2 ${CMAKE_CURRENT_LIST_DIR}/lib/ws2812.pio)

target_link_libraries(OhmimetroV2
        pico_stdlib
        hardware_i2c
        hardware_adc
        hardware_pio
        )

target_include_directories(OhmimetroV2 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_enable_stdio_uart(OhmimetroV2 1)
pico_enable_stdio_usb(OhmimetroV2 1)

pico_add_extra_outputs(OhmimetroV2)

