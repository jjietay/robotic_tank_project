cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(my_car_project C CXX ASM)

pico_sdk_init()

add_executable(my_car_project
    main.cpp
)

# This line tells CMake to find the PWM and GPIO hardware headers
target_link_libraries(my_car_project 
    pico_stdlib 
    hardware_pwm
)

pico_add_extra_outputs(my_car_project)

# Find your target name (e.g., mycarproject or picocar) and add these two lines:
pico_enable_stdio_usb(my_car_project 1)
pico_enable_stdio_uart(my_car_project 0)