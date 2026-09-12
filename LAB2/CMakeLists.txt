# cmake_minimum_required(VERSION 3.13)

# include(pico_sdk_import.cmake)

# project(pico_blink_button C CXX ASM)

# pico_sdk_init()

# add_executable(picow_blink_button picow_blink_button.c)
# target_link_libraries(picow_blink_button pico_stdlib)
# pico_enable_stdio_usb(picow_blink_button 1)
# pico_enable_stdio_uart(picow_blink_button 0)
# pico_add_extra_outputs(picow_blink_button)

# add_executable(uart_rx uart_rx.c)

# # pull in common dependencies
# target_link_libraries(uart_rx pico_stdlib)

# # enable usb output, enable uart output
# pico_enable_stdio_usb(uart_rx 1)
# pico_enable_stdio_uart(uart_rx 1)

# # create map/bin/hex file etc.
# pico_add_extra_outputs(uart_rx)

# # # add url via pico_set_program_url
# # example_auto_set_url(uart_rx)


# for main.c (CHALLENGE)
cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(uart_button_project C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(uart_button_project
    main.c
)

# Send printf() output over USB, not the default UART, since
# UART1 (GP8/GP9) is already used for the loopback.
pico_enable_stdio_usb(uart_button_project 1)
pico_enable_stdio_uart(uart_button_project 0)

pico_add_extra_outputs(uart_button_project)

target_link_libraries(uart_button_project
    pico_stdlib
    hardware_uart
)
