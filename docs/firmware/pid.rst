PID
===

See also: :doc:`main.cpp <main_cpp>`

Purpose
-------

The ``PID`` class provides a simple proportional-integral-derivative controller
for wheel-speed correction.

What it does
------------

Given a setpoint and a measured value, the controller computes:

- proportional response from current error,
- integral response from accumulated error,
- derivative response from rate of change of error.

Implementation notes
--------------------

The implementation clamps the integral term to reduce windup. In the current
code, the gains appear simple and conservative, which is sensible for early
testing but may not be enough for precise closed-loop control.

Learning note
-------------

You should treat this page as a place to explain:

- what the setpoint is,
- what the measured value is,
- what the controller output drives,
- how tuning affects tank behaviour.