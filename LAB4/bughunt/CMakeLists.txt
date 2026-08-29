cmake_minimum_required(VERSION 3.13)

# Copy pico_sdk_import.cmake from $PICO_SDK_PATH/external/ into this folder
include(pico_sdk_import.cmake)

project(bughunt4 C CXX ASM)

pico_sdk_init()

add_executable(bughunt4
    bughunt4.c
    algo.c
)

target_link_libraries(bughunt4
    pico_stdlib
    hardware_pwm
    hardware_adc
)

pico_enable_stdio_usb(bughunt4 1)
pico_enable_stdio_uart(bughunt4 0)

pico_add_extra_outputs(bughunt4)

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
target_compile_options(bughunt4 PRIVATE -Wall -Wextra -Wno-unused-parameter)

