micro-ROS
=========

See also: :doc:`main.cpp <main_cpp>`

Purpose
-------

This page documents the micro-ROS-specific parts of the Pico W firmware.

Main elements
-------------

The firmware defines:

- one ``/cmd_vel`` subscriber,
- ultrasonic range publishers,
- encoder tick publishers,
- message buffers,
- executor, support, allocator, and node objects.

Callback flow
-------------

The ``cmd_vel_callback`` receives a ``geometry_msgs/msg/Twist`` message and
maps linear and angular command values into left and right wheel targets.

Design note
-----------

This is the communication bridge between the Raspberry Pi 4 ROS 2 system and
the low-level Pico control loop. If messages are delayed, dropped, or mis-scaled,
the whole robot feels wrong even if the hardware code is correct.