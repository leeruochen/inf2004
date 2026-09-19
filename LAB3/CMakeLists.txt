# for sensor
# cmake_minimum_required(VERSION 3.13)

# # Initialize the Pico SDK
# include(pico_sdk_import.cmake)

# project(WheelEncoder C CXX ASM)

# pico_sdk_init()

# # Point this to your wheel encoder C file
# add_executable(hello_gpio_irq
#     hello_gpio_irq.c
# )

# target_link_libraries(hello_gpio_irq pico_stdlib)

# # Enable USB output for the Serial Monitor
# pico_enable_stdio_usb(hello_gpio_irq 1)
# pico_enable_stdio_uart(hello_gpio_irq 0)

# pico_add_extra_outputs(hello_gpio_irq)

# # Keep the compiler warnings from bughunt3 to catch hidden bugs
# target_compile_options(hello_gpio_irq PRIVATE -Wall -Wextra -Wno-unused-parameter)



# for ultrasonic ranger
# cmake_minimum_required(VERSION 3.13)

# # Initialize the Pico SDK
# include(pico_sdk_import.cmake)

# project(UltrasonicRanger C CXX ASM)

# pico_sdk_init()

# # Point this to your new ultrasonic C file
# add_executable(ultrasonic
#     ultrasonic.c
# )

# target_link_libraries(ultrasonic pico_stdlib)

# # Enable USB output for the Serial Monitor
# pico_enable_stdio_usb(ultrasonic 1)
# pico_enable_stdio_uart(ultrasonic 0)

# pico_add_extra_outputs(ultrasonic)

# # Keep the compiler warnings to help find the planted bugs
# target_compile_options(ultrasonic PRIVATE -Wall -Wextra -Wno-unused-parameter)

# stop watch
cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(Stopwatch C CXX ASM)

pico_sdk_init()

add_executable(stopwatch stopwatch.c)

target_link_libraries(stopwatch pico_stdlib)

pico_enable_stdio_usb(stopwatch 1)
pico_enable_stdio_uart(stopwatch 0)
pico_add_extra_outputs(stopwatch)

target_compile_options(stopwatch PRIVATE -Wall -Wextra)