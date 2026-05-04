main.cpp
======================

See also: :doc:`Pico Overview <pico_overview>`

Purpose
-------

This page documents the main Pico W firmware file. The file combines hardware
drivers, helper functions, micro-ROS communication, and the main execution loop.

Responsibilities
----------------

``main.cpp`` is responsible for:

- Initialising GPIO, PWM, timing, and interrupts.
- Managing ultrasonic distance sensors.
- Reading wheel encoder counts.
- Driving the motor controller.
- Handling ``/cmd_vel`` subscription callbacks.
- Publishing ultrasonic and encoder data to ROS 2.
- Running the main control loop on the Pico W.

High-level structure
--------------------

At a high level, the file is organised into:

- **Electronics base class** - shared name/status for hardware components.
- :doc:`Ultrasonic <ultrasonic>` - blocking measurement with timeout and last-distance cache.
- :doc:`Motor <motor>` - Cytron driver wrapper, direction and PWM control helpers.
- :doc:`Encoder <encoder>` - quadrature counting via interrupts and count access.
- :doc:`PID <pid>` - simple controller with clamped integral term.
- :doc:`micro-ROS globals and callback <microros>` - subscriber, publishers, executor, and ``cmd_vel`` callback.
- :doc:`Helper functions <helper>` - deadband, range message initialisation, and timestamp handling.
- :doc:`main() Loop <main_loop>` - startup, ROS wiring, and repeated control loop execution.

Related components
------------------

.. toctree::
    :maxdepth: 1

    encoder
    ultrasonic
    motor
    pid
    helper
    microros
    main_loop