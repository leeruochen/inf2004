cmake_minimum_required(VERSION 3.13)

# Copy pico_sdk_import.cmake from $PICO_SDK_PATH/external/ into this folder
include(pico_sdk_import.cmake)

project(bughunt2 C CXX ASM)

pico_sdk_init()

add_executable(bughunt2
    bughunt2_pico.c
    frame.c
)

target_link_libraries(bughunt2
    pico_stdlib
    hardware_uart
)

# printf goes over USB; the link under test is UART1 on GP8/GP9
pico_enable_stdio_usb(bughunt2 1)
pico_enable_stdio_uart(bughunt2 0)

pico_add_extra_outputs(bughunt2)

# --------------------------------------------------------------------
# The Pico SDK does not enable compiler warnings by default. Several of
# the planted defects in this hunt are reported ONLY as warnings, so the
# briefs would be asking you to read messages that are never emitted.
#
#   -Wextra is needed as well as -Wall: the "unsigned value compared
#   against < 0" defect is -Wtype-limits, which -Wall alone does not
#   include.
#
#   -Wno-unused-parameter suppresses noise from SDK callback signatures
#   that must take arguments they do not use. That noise is not a
#   planted defect.
# --------------------------------------------------------------------
target_compile_options(bughunt2 PRIVATE -Wall -Wextra -Wno-unused-parameter)

