add_executable(${PROJECT_NAME}
    main.c
    gy521.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../include
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_i2c
    pico_multicore
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})
