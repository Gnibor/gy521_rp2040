cmake_minimum_required(VERSION 3.13)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Pico SDK importieren (Pfad muss passen!)
include(pico_sdk_import.cmake)

project(gy521_rp2040 C CXX ASM)

#set(CMAKE_C_STANDARD 11)
#set(CMAKE_CXX_STANDARD 17)

# SDK initialisieren
pico_sdk_init()

# Unterordner hinzufügen
add_subdirectory(src)
