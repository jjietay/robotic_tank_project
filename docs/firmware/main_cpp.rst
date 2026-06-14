main.cpp
========

See also: :doc:`Pico Overview <pico_overview>`

Purpose
-------

``main.cpp`` sets up the hardware, sets up the
micro ROS link, and then runs the control loop that keeps the tank moving.

What it does
------------

- Sets up GPIO, PWM, timing, and interrupts
- Builds the four ultrasonic sensors, the motor driver, the two encoders, and
  the two PID controllers
- Sets up the micro ROS node, the ``/cmd_vel`` subscriber, and the publishers
- Reads ``/cmd_vel`` and turns it into a target speed for each track
- Publishes ultrasonic ranges and encoder ticks back to ROS 2
- Runs the control loop at 100 Hz

Structure
---------

The file is split into a few parts:

- The :doc:`Electronics <main_cpp>` base class gives every hardware class a name
  and a status
- :doc:`Ultrasonic <ultrasonic>` measures distance with a timeout
- :doc:`Motor <motor>` drives the Cytron motor driver
- :doc:`Encoder <encoder>` counts wheel ticks using interrupts and works out
  wheel speed
- :doc:`PID <pid>` corrects the wheel speed
- :doc:`micro ROS <microros>` holds the subscriber, the publishers, and the
  command callback
- :doc:`Helper functions <helper>` set up range messages, stamp messages, and
  clamp values
- :doc:`main() Loop <main_loop>` runs the repeating control cycle

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
