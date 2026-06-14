main() Loop
===========

See also: :doc:`main.cpp <main_cpp>`

Startup
-------

Before the loop starts, ``main()`` does a one time setup:

1. Starts USB and serial output, then waits two seconds so the host can attach.
2. Builds the four ultrasonic sensors, the motor driver, the two encoders, and
   the two PID controllers.
3. Sets up the micro ROS transport over the Pico UART and creates a node named
   ``pico``.
4. Creates the ``/cmd_vel`` subscriber and the publishers for the ultrasonic
   ranges and the encoder ticks.
5. Fills in the range message settings and the frame names.
6. Adds the ``/cmd_vel`` subscription to the executor.

The publishers and subscriber are:

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Role
     - Topic
     - Message type
   * - Subscriber
     - ``/cmd_vel``
     - ``geometry_msgs/msg/Twist``
   * - Ultrasonic publishers (4x)
     - ``/sensors/ultrasonic/usrm_*``
     - ``sensor_msgs/msg/Range``
   * - Encoder publishers (2x)
     - ``/sensors/encoders/*_ticks``
     - ``std_msgs/msg/Int32``

The control loop
----------------

After setup, ``main()`` runs forever at about 100 Hz. Each pass through the
loop does the following.

1. Mark the time
~~~~~~~~~~~~~~~~

The loop records the start time so it can pace itself to 100 Hz at the end.

2. Handle ROS messages
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   rclc_executor_spin_some(&executor, RCL_MS_TO_NS(2));

This gives the executor a short period of time to read new micro ROS messages.
When a new ``/cmd_vel`` arrives, the callback updates the left and right target
speeds in metres per second.

3. Fire one ultrasonic sensor
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Only one ultrasonic sensor fires per cycle, and only on every third cycle. A
rotating index picks the front, back, right, or left sensor in turn. The chosen
sensor takes a reading, the range and timestamp are filled in, and the message
is published. Spacing the sensors out keeps a slow reading from blocking the
loop timing.

4. Read the command and check the watchdog
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The loop copies the latest target speeds. If no ``/cmd_vel`` has arrived for
more than 500 ms, both targets are set to zero, so the tank stops if the link
goes quiet.

5. Safety stop
~~~~~~~~~~~~~~

The loop looks at the average target speed to decide if the tank wants to go
forward or backward. If it wants to go forward and the front sensor sees an
object closer than the front stop distance, both speeds are set to zero. The
same check is done for the back sensor when reversing.

6. Feedforward and PID
~~~~~~~~~~~~~~~~~~~~~~~

The loop reads the measured speed of each wheel from the encoders. For each
side it works out a feedforward duty from the target speed, then adds the PID
correction, and clamps the result to the range from negative one to one:

.. code-block:: cpp

   float ff_l = clampf(vel_l / V_MAX_MPS, -1.0f, 1.0f);
   duty_l = clampf(ff_l + pid_l.calculate(vel_l, meas_l), -1.0f, 1.0f);

When both targets are zero, the PID controllers are reset and the duty is set
to zero, so the integral term does not build up while the tank is stopped.

7. Drive the motors
~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   MOTOR.move(duty_l, duty_r);

The motor class takes the two duty values and sets the direction pins and PWM
levels.

8. Publish encoder ticks
~~~~~~~~~~~~~~~~~~~~~~~~

The loop reads the signed tick count from each encoder and publishes them so
the odometry node on the Raspberry Pi can use them.

9. Pace the loop
~~~~~~~~~~~~~~~~

The loop busy waits until 10 ms have passed since the start time, which gives a
steady 100 Hz rate.
