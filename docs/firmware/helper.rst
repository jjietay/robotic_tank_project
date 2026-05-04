Helper Functions
================

See also: :doc:`main.cpp <main_cpp>`

Purpose
-------

This page documents the smaller utility functions used by the firmware.

Functions
---------

``apply_deadband(u, deadband=0.15f)``
    Forces small commands to zero or to a minimum effective magnitude.

``init_range_msg(...)``
    Initialises ``sensor_msgs/msg/Range`` fields consistently.

``set_msg_stamp(...)``
    Fills message timestamps from the Pico time source.

Why this matters
----------------

Helper functions are easy to ignore, but they often contain important system
assumptions. For example, deadband handling strongly affects how the motors
respond to small commands and whether the robot feels controllable.