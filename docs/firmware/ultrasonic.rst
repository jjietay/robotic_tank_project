Ultrasonic
==========

See also: :doc:`main.cpp <main_cpp>`

Purpose
-------

The ``Ultrasonic`` class wraps one HC-SR04-style ultrasonic sensor using a
trigger pin and an echo pin.

How it works
------------

The class:

- emits a trigger pulse,
- waits for the echo pin to go high,
- waits for the echo pulse to end,
- converts pulse duration into distance.

Important methods
-----------------

``update()``
    Fires the sensor and returns the measured distance in centimetres.

``get_distance()``
    Returns the most recent stored measurement.

Implementation notes
--------------------

The current implementation is blocking and waits for the echo pulse with a
timeout. This is simple and easy to reason about, but it can become a scaling
limit if many sensors or tighter timing constraints are introduced.

Learning note
-------------

The main embedded-system tradeoff here is simplicity versus responsiveness:

- blocking measurement is easy to debug,
- non-blocking measurement scales better but is harder to implement safely.