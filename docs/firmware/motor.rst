Motor
=====

See also: :doc:`main.cpp <main_cpp>`

Purpose
-------

The ``Motor`` class wraps control of the Cytron MDD10A motor driver using GPIO
direction pins and PWM output pins.

Responsibilities
----------------

The class is responsible for:

- configuring PWM output,
- setting motor direction,
- converting normalised speed commands into PWM duty values,
- exposing simple motion helpers such as forward, backward, and tank turns.

Important methods
-----------------

``move(left, right)``
    Sets left and right motor commands directly.

``forward()`` / ``backward()``
    Convenience helpers for symmetric motion.

``tank_turn_left()`` / ``tank_turn_right()``
    Convenience helpers for in-place turning.

``stop()``
    Stops both motors.

Implementation notes
--------------------

This class currently applies open-loop motor commands. That is enough for basic
teleoperation and initial experiments, but it does not guarantee accurate wheel
speed under changing load or battery conditions.