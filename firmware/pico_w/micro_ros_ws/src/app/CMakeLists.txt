cmake_minimum_required(VERSION 3.12)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(pico_micro_ros C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# micro-ROS static library directory
link_directories(${CMAKE_CURRENT_LIST_DIR}/../../libmicroros)

# Executable: main.cpp + pico_uart_transport.c (one directory up)
add_executable(pico_micro_ros
    main.cpp
    ../pico_uart_transport.c
)

# Link against Pico stdlib, hardware libs, and micro-ROS
target_link_libraries(pico_micro_ros
    pico_stdlib
    hardware_pwm
    hardware_gpio
    hardware_sync
    microros
)

# micro-ROS headers
target_include_directories(pico_micro_ros PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/../../libmicroros/include
)

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")

pico_enable_stdio_usb(pico_micro_ros 1)
pico_enable_stdio_uart(pico_micro_ros 0)

pico_add_extra_outputs(pico_micro_ros)