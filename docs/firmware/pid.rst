PID
===

See also: :doc:`main.cpp <main_cpp>`

Purpose
-------

The ``PID`` class corrects the wheel speed. Each wheel has its own controller.
It compares the speed you asked for with the speed the encoder measured, and it
returns a duty correction that nudges the wheel towards the target.

Units
-----

- The setpoint and the measured value are both in metres per second
- The output is a duty value in the range from negative one to one, the same
  range the motor class uses

What calculate does
-------------------

``calculate(setpoint, measured)`` runs one step of the controller:

1. **Time step**

   It calculates the time since the last call. If that time looks
   wrong, for example zero or too large after a pause, it falls back to ten
   milliseconds.

2. **Error**

   The error is the setpoint minus the measured speed.

3. **Proportional**

   The P term is the gain times the error.

4. **Integral**

   The error times the time step is added to a running total. The
   total is held inside a fixed limit so it cannot grow without bound. The I
   term is the gain times that total.

5. **Derivative**

   The D term is the gain times the change in error over the
   time step. It is skipped on the very first call so a sudden jump does not
   cause a kick.

6. **Sum and limit**

   The three terms are added. If the sum goes past the output
   limit, the output is held at the limit and the integral is wound back by the
   amount it went over. That keeps the integral from building up while the
   output is maxed out.

Resetting
---------

``reset()`` clears the integral, clears the last error, and marks the next call
as the first one. The control loop calls this whenever the tank is told to stop,
so the controller starts clean the next time the tank moves.
