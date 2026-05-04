Motor
=====

See also: :doc:`main.cpp <main_cpp>`

Purpose
-------

The ``Motor`` class wraps control of the Cytron MDD10A dual-channel motor
driver using GPIO direction pins and PWM output pins. Each instance controls
both left and right motors for the tank.

Responsibilities
----------------

- configuring PWM outputs for both motor channels,
- setting motor direction via digital GPIO pins,
- converting normalised speed commands (:math:`-1.0` to :math:`+1.0`) into PWM duty values,
- exposing simple motion helpers such as forward, backward, and tank turns.

Hardware Overview
-----------------

The Cytron MDD10A is a dual-channel DC motor driver. For each motor it exposes:

- **DIR pin** — sets the direction (forward/reverse),
- **PWM pin** — controls the speed using a duty cycle signal.

In this design:

- ``l_dir_pin`` and ``l_pwm_pin`` control the **left** motor,
- ``r_dir_pin`` and ``r_pwm_pin`` control the **right** motor.

Conceptual wiring diagram:

.. code-block:: text

   Pico GPIO           MDD10A                 Motor
   ----------          ------                 -----
   L_DIR   ───────►   DIR1   ───────►   Left motor direction
   L_PWM   ───────►   PWM1   ───────►   Left motor speed

   R_DIR   ───────►   DIR2   ───────►   Right motor direction
   R_PWM   ───────►   PWM2   ───────►   Right motor speed

Speed and Direction Mapping
---------------------------

The ``move(left, right)`` method takes normalised speed commands in the range
:math:`[-1.0, 1.0]` for each wheel:

- ``+1.0``  → full speed forward,
- ``0.0``   → stop,
- ``-1.0``  → full speed backward.

Direction is set by the digital DIR pin, and magnitude is set by PWM:

- If speed > 0: DIR = 1 (forward)
- If speed < 0: DIR = 0 (reverse)
- PWM duty = :math:`|speed| \times \text{PWM\_TOP}`

Code sketch:

.. code-block:: cpp

   void _set(uint dir_pin, uint pwm_pin, float speed)
   {
       speed = std::max(-1.0f, std::min(1.0f, speed));    // clamp
       gpio_put(dir_pin, speed > 0 ? 1 : 0);
       pwm_set_gpio_level(pwm_pin, (uint16_t)(fabsf(speed) * PWM_TOP));
   }

This keeps the public API (methods like ``move``, ``forward``, ``stop``) in intuitive units (normalised speed) while hiding
the hardware-specific PWM limits inside the class.

Movement Helpers
----------------

The helper methods combine left/right commands into standard tank motions:

.. list-table::
   :header-rows: 1
   :widths: 30 30 40

   * - Method
     - Left motor
     - Right motor
   * - ``forward()``
     - ``+DRIVE_DUTY``
     - ``+DRIVE_DUTY``
   * - ``backward()``
     - ``-DRIVE_DUTY``
     - ``-DRIVE_DUTY``
   * - ``tank_turn_left()``
     - ``+TURN_DUTY``
     - ``-TURN_DUTY``
   * - ``tank_turn_right()``
     - ``-TURN_DUTY``
     - ``+TURN_DUTY``
   * - ``stop()``
     - ``0.0``
     - ``0.0``

Illustration of tank turns:

.. code-block:: text

   tank_turn_left():
       Left  track → forward
       Right track → backward
       → Tank spins on the spot to the left

   tank_turn_right():
       Left  track → backward
       Right track → forward
       → Tank spins on the spot to the right

Example Usage
-------------

A typical usage from higher-level code:

.. code-block:: cpp

   Motor MOTOR("MOTORS", "ON", L_DIR, L_PWM, R_DIR, R_PWM);

   // Drive forward at nominal duty
   MOTOR.forward();

   // Spin in place to the right
   MOTOR.tank_turn_right();

   // Stop both motors
   MOTOR.stop();

Implementation Notes
--------------------

**1) PWM configuration**

The constructor configures each PWM pin as follows:

- sets the GPIO function to PWM,
- sets the PWM wrap value to ``PWM_TOP`` (full-scale),
- initialises channel level to zero,
- enables the PWM slice.

This establishes a consistent PWM range where:

.. math::

   \text{duty level} = |speed| \times \text{PWM\_TOP}

**2) Open-loop control**

This class currently applies **open-loop** motor commands:

- It does **not** measure actual wheel speed.
- It does **not** compensate for battery voltage, load, or friction.
- Commands are “best effort”: higher duty generally means faster motion,
  but not a guaranteed velocity.


Class Reference
---------------

.. cpp:class:: Motor : public Electronics

    - Wraps the Cytron MDD10A dual motor driver.

.. cpp:function:: void move(float left, float right)

    - Sets left and right motor commands directly in the range :math:`[-1.0, 1.0]`. Handles direction and PWM duty internally.

.. cpp:function:: void forward()
    void backward()

    - Convenience helpers for symmetric forward and backward motion at predefined duty levels.

.. cpp:function:: void tank_turn_left()
                    void tank_turn_right()

    - Convenience helpers for in-place rotation by driving tracks in opposite directions.

.. cpp:function:: void stop()

    - Stops both motors by setting PWM duty to zero on both channels.