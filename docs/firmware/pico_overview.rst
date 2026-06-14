Pico Overview
=============

See also: :doc:`main.cpp <main_cpp>`

Purpose
-------

Pico W takes care of the hardware.

The Pico W is responsible for:

- Driving the motors through the Cytron motor driver
- Reading the wheel encoders using GPIO interrupts
- Measuring distance with the ultrasonic sensors
- Talking to the ROS 2 system on the Raspberry Pi over micro ROS


Topics
------

The firmware uses these ROS topics:

- ``/cmd_vel`` for the incoming motion command
- ``/sensors/ultrasonic/usrm_front``
- ``/sensors/ultrasonic/usrm_back``
- ``/sensors/ultrasonic/usrm_left``
- ``/sensors/ultrasonic/usrm_right``
- ``/sensors/encoders/left_ticks``
- ``/sensors/encoders/right_ticks``

Layout
------

All of the firmware are in one ``main.cpp`` file + a small set of classes,
motors, encoders, ultrasonic sensors, and PID controller. Shared settings such as pins, robot size, and gains are in ``config.hpp``.
