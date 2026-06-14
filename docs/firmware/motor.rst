Motor
=====

See also: :doc:`main.cpp <main_cpp>`

Purpose
-------

The ``Motor`` class drives the two tracks through the Cytron MDD10A motor
driver. One ``Motor`` object controls both the left and the right side.

How it works
------------

For each side the driver has two pins:

- a direction pin that sets forward or reverse
- a PWM pin that sets the speed

In this design ``l_dir_pin`` and ``l_pwm_pin`` control the left side, and
``r_dir_pin`` and ``r_pwm_pin`` control the right side.

The move method
---------------

``move(duty_left, duty_right)`` takes a duty value for each side in the range
from negative one to one:

- ``+1`` is full speed forward,
- ``0`` is stop,
- ``-1`` is full speed reverse

Before the command reaches the pins, ``move`` does three things:

1. **Trim**

   Each side is scaled by its trim factor. This helps account for manufacturing defects for straight driving.

2. **Minimum duty**

   A small duty often will not move the motor at all. If the
   original command was above the minimum, the duty is pushed up to at least the
   minimum so the wheel actually turns. The minimum is only applied when the
   original command was accepted, so it does not cancel out the trim.

3. **Invert**

   If a side is wired in reverse, an invert flag flips its sign, so
   the rest of the code can always treat positive as forward.

Setting one side
----------------

Each side is then sent to the driver:

.. code-block:: cpp

   void Motor::set_one_side(uint dir_pin, uint pwm_pin, float duty)
   {
       if (duty >  1.0f) duty =  1.0f;
       if (duty < -1.0f) duty = -1.0f;
       gpio_put(dir_pin, duty >= 0.0f ? 1 : 0);
       pwm_set_gpio_level(pwm_pin, (uint16_t)(std::fabs(duty) * (float)PWM_TOP));
   }

The duty is clamped to the safe range, the sign sets the direction pin, and the
size sets the PWM level. ``PWM_TOP`` is the full scale value, so a duty of one
gives the highest PWM level.

Other methods
-------------

.. cpp:function:: void stop()

Sets both sides to zero so both motors stop.

Setup
-----

The constructor stores the pins, the invert flags, and the trim values, sets the
two direction pins as outputs, and sets up PWM on the two speed pins.
