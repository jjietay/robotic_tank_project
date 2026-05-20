cmake_minimum_required(VERSION 3.13)

# Ensure this points to your SDK. If you keep pico_sdk_import.cmake in your root,
# ensure the path relative to this file is correct.
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(tank_debug C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# We only include main.cpp (from this debug folder) and the motor.cpp (from src)
add_executable(tank_debug
    main.cpp
    ../src/motor.cpp
)

target_link_libraries(tank_debug
    pico_stdlib
    hardware_pwm
    hardware_gpio
)

# Crucial: This tells the compiler to look in the 'src' folder for your .hpp files
target_include_directories(tank_debug PUBLIC
    ../src
)

pico_enable_stdio_usb(tank_debug 1)
pico_enable_stdio_uart(tank_debug 0)
pico_add_extra_outputs(tank_debug)