cmake_minimum_required(VERSION 3.13)

# Copy pico_sdk_import.cmake from $PICO_SDK_PATH/external/ into this folder.
include(pico_sdk_import.cmake)

# FREERTOS_KERNEL_PATH must point at the FreeRTOS-Kernel you downloaded in
# the main part of Lab 5. Set it in the CMake Tools environment, or pass
#   cmake -DFREERTOS_KERNEL_PATH=/path/to/FreeRTOS-Kernel ..
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(bughunt5 C CXX ASM)

pico_sdk_init()

add_executable(bughunt5
    bughunt5.c
)

# FreeRTOSConfig.h lives in this folder
target_include_directories(bughunt5 PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(bughunt5
    pico_stdlib
    hardware_adc
    FreeRTOS-Kernel
    FreeRTOS-Kernel-Heap4
)

pico_enable_stdio_usb(bughunt5 1)
pico_enable_stdio_uart(bughunt5 0)

pico_add_extra_outputs(bughunt5)

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
target_compile_options(bughunt5 PRIVATE -Wall -Wextra -Wno-unused-parameter)

