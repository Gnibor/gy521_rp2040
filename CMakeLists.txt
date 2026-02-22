cmake_minimum_required(VERSION 3.13)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Pico SDK importieren (Pfad muss passen!)
include(pico_sdk_import.cmake)

project(gy521_rp2040 C CXX ASM)

# SDK initialisieren
pico_sdk_init()

add_executable(${PROJECT_NAME}
    src/main.c
    src/default.c
    src/gy521.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_i2c
    pico_multicore
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})
