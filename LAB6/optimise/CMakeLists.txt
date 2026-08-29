cmake_minimum_required(VERSION 3.13)

# Copy pico_sdk_import.cmake from $PICO_SDK_PATH/external/ into this folder
include(pico_sdk_import.cmake)

project(optimise C CXX ASM)

pico_sdk_init()

add_executable(optimise
    optimise_pico.c
    optimise.c
)

target_link_libraries(optimise
    pico_stdlib
)

pico_enable_stdio_usb(optimise 1)
pico_enable_stdio_uart(optimise 0)

pico_add_extra_outputs(optimise)

target_compile_options(optimise PRIVATE -Wall -Wextra)

# --------------------------------------------------------------------
# BUILD THIS THREE TIMES. The optimisation level is part of the answer.
#
#   mkdir build-O0 && cd build-O0
#   cmake -DPICO_BOARD=pico_w -DCMAKE_BUILD_TYPE=Debug ..          && make
#
#   mkdir build-O2 && cd build-O2
#   cmake -DPICO_BOARD=pico_w -DCMAKE_BUILD_TYPE=RelWithDebInfo .. && make
#
#   mkdir build-O3 && cd build-O3
#   cmake -DPICO_BOARD=pico_w -DCMAKE_BUILD_TYPE=Release ..        && make
#
# Some of your hand optimisations will turn out to be things the compiler
# was already doing at -O2. Finding out WHICH ones is most of the exercise:
# an "optimisation" that only helps at -O0 is not an optimisation, it is
# you doing unpaid work on the compiler's behalf.
#
# To see what it actually emitted:
#   arm-none-eabi-objdump -d build-O2/optimise.elf > O2.asm
# --------------------------------------------------------------------
