cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(tank_debug C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")

# Shared sources/settings for both standalone (no micro-ROS) debug binaries.
function(add_debug_target target srcfile)
    add_executable(${target}
        ${srcfile}
        ../src/motor.cpp
        ../src/encoder.cpp
    )
    target_link_libraries(${target}
        pico_stdlib
        hardware_pwm
        hardware_gpio
        hardware_sync
    )
    target_include_directories(${target} PUBLIC ../src)
    target_compile_definitions(${target} PRIVATE
        PICO_STDIO_ENABLE_CRLF_SUPPORT=0
        PICO_STDIO_DEFAULT_CRLF=0
    )
    pico_enable_stdio_usb(${target} 1)
    pico_enable_stdio_uart(${target} 0)
    pico_add_extra_outputs(${target})
endfunction()

# Minimum-PWM sweep
add_debug_target(tank_debug         main.cpp)

# Maximum-velocity test
add_debug_target(tank_debug_maxvel  main1.cpp)
