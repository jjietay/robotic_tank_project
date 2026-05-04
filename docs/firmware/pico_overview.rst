Pico Overview
=============

See also: :doc:`main.cpp <main_cpp>`

Purpose
-------

The Pico W firmware handles low-level hardware control that is better suited
to a microcontroller than the Raspberry Pi 4.

In this project, the Pico W is responsible for:

- Motor actuation through the Cytron motor driver.
- Wheel encoder handling through GPIO interrupts.
- Ultrasonic distance measurement.
- micro-ROS communication with the ROS 2 system on the Raspberry Pi 4.

Role in the system
------------------

The Pico W acts as a real-time hardware interface layer. It receives motion
commands from ROS 2, applies low-level motor control, and publishes sensor
feedback such as ultrasonic ranges and encoder tick counts.

Topics handled
--------------

The firmware currently interacts with these ROS interfaces:

- ``/cmd_vel`` for incoming motion commands.
- ``/sensors/ultrasonic/usrm_front``
- ``/sensors/ultrasonic/usrm_back``
- ``/sensors/ultrasonic/usrm_left``
- ``/sensors/ultrasonic/usrm_right``
- ``/sensors/encoders/left_ticks``
- ``/sensors/encoders/right_ticks``

Design notes
------------

This firmware combines embedded GPIO/PWM logic with micro-ROS messaging in a
single ``main.cpp`` file. Over time, this can be refactored into clearer
modules, but the current structure is still good enough for learning and
system bring-up.