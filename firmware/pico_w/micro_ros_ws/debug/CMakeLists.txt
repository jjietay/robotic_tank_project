cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(tank_debug C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# Point at the same micro-ROS library used by the main build
set(MICROROS_LIB_DIR     ${CMAKE_CURRENT_LIST_DIR}/../src/libmicroros)
set(MICROROS_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/../src/libmicroros/include)

link_directories(${MICROROS_LIB_DIR})

add_executable(tank_debug
    main.cpp
    ../src/motor.cpp
    ../src/pico_uart_transport.c
)

target_link_libraries(tank_debug
    pico_stdlib
    hardware_pwm
    hardware_gpio
    hardware_sync
    microros
)

target_include_directories(tank_debug PUBLIC
    ../src
    ${MICROROS_INCLUDE_DIR}
)

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")

target_compile_definitions(tank_debug PRIVATE
    PICO_UART_ENABLE_CRLF_SUPPORT=0
    PICO_STDIO_ENABLE_CRLF_SUPPORT=0
    PICO_STDIO_DEFAULT_CRLF=0
)

pico_enable_stdio_usb(tank_debug 1)
pico_enable_stdio_uart(tank_debug 0)
pico_add_extra_outputs(tank_debug)