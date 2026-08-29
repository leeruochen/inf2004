cmake_minimum_required(VERSION 3.13)

# Copy pico_sdk_import.cmake from $PICO_SDK_PATH/external/ into this folder
include(pico_sdk_import.cmake)

project(cmdparse C CXX ASM)

pico_sdk_init()

add_executable(cmdparse
    cmdparse_pico.c
    cmdparse.c
)

target_link_libraries(cmdparse
    pico_stdlib
    hardware_uart
)

# printf goes over USB; the command link is UART1 on GP8/GP9.
# Keeping them apart is not a style preference - see "A Deliberate
# Intermittent Fault" in this lab's README for what happens when you don't.
pico_enable_stdio_usb(cmdparse 1)
pico_enable_stdio_uart(cmdparse 0)

pico_add_extra_outputs(cmdparse)

# --------------------------------------------------------------------
# The Pico SDK does not enable compiler warnings by default. String and
# pointer code is exactly where -Wall -Wextra earns its keep, so they are
# on here and your submission is expected to build clean.
# --------------------------------------------------------------------
target_compile_options(cmdparse PRIVATE -Wall -Wextra -Wno-unused-parameter)
