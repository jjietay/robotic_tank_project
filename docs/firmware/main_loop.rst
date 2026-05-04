main() Loop
===========

See also: :doc:`main.cpp <main_cpp>`

Startup Sequence
----------------

The ``main()`` function performs a one-time initialisation sequence before
entering the control loop:

1. **Standard I/O and delay**

   - Calls ``stdio_init_all()`` to enable USB/serial logging.
   - Waits briefly (``sleep_ms(2000)``) to give the USB connection and host
     tools time to attach.

2. **Sensor and actuator construction**

   - Creates four ``Ultrasonic`` objects:
     - front, back, left, right sensors with their respective TRIG/ECHO pins.
   - Creates one ``Motor`` object:
     - wraps the Cytron MDD10A driver (left/right direction + PWM pins).
   - Creates two ``Encoder`` objects:
     - ``LEFT_ENCODER`` and ``RIGHT_ENCODER`` with their A/B pins.
   - Creates two ``PID`` controllers:
     - ``LEFT_PID`` and ``RIGHT_PID`` for closed-loop control.

3. **micro-ROS transport and node**

   - Configures a custom serial transport for micro-ROS over the Pico UART.
   - Initialises the default allocator and support structures.
   - Creates a micro-ROS node named ``"pico"``.

4. **Subscribers and publishers**

.. list-table::
   :header-rows: 1
   :widths: 30 30 40

   * - Role
     - Topic
     - Message type
   * - Subscriber
     - ``/cmd_vel``
     - ``geometry_msgs/msg/Twist``
   * - 4× Ultrasonic publisher
     - ``/sensors/ultrasonic/usrm_X``
     - ``sensor_msgs/msg/Range``
   * - 2× Encoder publisher
     - ``/sensors/encoders/X_ticks``
     - ``std_msgs/msg/Int32``


5. **Message setup**

   - Fills ``sensor_msgs/msg/Range`` metadata via ``init_range_msg(...)``.
   - Assigns frame IDs for each ultrasonic sensor
     (e.g. ``"ultrasonic_front_link"``).

6. **Executor configuration**

   - Initialises the micro-ROS executor with capacity for one subscription.
   - Adds the ``/cmd_vel`` subscription and associates
     ``cmd_vel_callback`` with it.

   After this, a log message indicates that micro-ROS is ready and listening
   on ``/cmd_vel``.

Control Loop
------------

After initialisation, ``main()`` enters an infinite loop that implements the
runtime behaviour of the tank:

.. code-block:: text

   while (true):
       1. Rate-limit loop execution
       2. Process incoming ROS messages
       3. Measure one ultrasonic sensor and publish
       4. Compute motor commands
       5. Apply directional safety checks
       6. Drive motors
       7. Publish encoder counts

Each step is outlined below.

1. Rate Limiting
~~~~~~~~~~~~~~~~

The loop uses a simple time-based check to limit how fast it runs:

- Stores the last loop timestamp in ``loop_last``.
- Reads the current time via ``time_us_64()``.
- If less than 10 000 µs (10 ms) have passed, it ``continue``s without
  executing the body.
- Otherwise, it updates ``loop_last`` and proceeds.

This enforces an approximate **100 Hz** loop rate and prevents the firmware
from busy-spinning at maximum speed.

2. Process ROS Messages
~~~~~~~~~~~~~~~~~~~~~~~

Within each loop iteration, the executor is given a small time budget to
handle incoming messages:

.. code-block:: cpp

   rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10));

This:

- checks the transport for new micro-ROS messages,
- calls ``cmd_vel_callback`` when a new ``/cmd_vel`` arrives,
- updates ``target_vel_l`` and ``target_vel_r`` accordingly.

3. Ultrasonic Measurement and Publishing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To avoid ultrasonic crosstalk, only **one** sensor is fired per loop tick.
A rotating index (``usrm_index``) selects which sensor to update:

- If ``usrm_index == 0`` → front sensor.
- If ``usrm_index == 1`` → back sensor.
- If ``usrm_index == 2`` → right sensor.
- If ``usrm_index == 3`` → left sensor.

For the selected sensor:

- Calls ``update()`` to perform a blocking measurement.
- Converts the result from centimetres to metres by dividing by 100.
- Updates the corresponding ``Range`` message’s ``range`` field.
- Calls ``set_msg_stamp(...)`` to fill the timestamp.
- Publishes the message on its ROS topic.

Then increments ``usrm_index`` modulo 4 so the next loop iteration uses the
next sensor.

4. Motor Command Computation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The loop constructs open-loop motor commands from the latest targets:

- Clamps ``target_vel_l`` and ``target_vel_r`` to the range
  :math:`[-1.0, 1.0]`.
- Passes them through ``apply_deadband(...)`` with a deadband of 0.15:

- Small commands are either zeroed or snapped to a minimum magnitude.
- This compensates for motor deadzone and makes low-speed control more predictable.

The result is:

- ``vel_l`` — normalised left wheel command,
- ``vel_r`` — normalised right wheel command.

5. Directional Safety Checks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The loop enforces simple directional E-stop behaviour using the front and
back ultrasonic readings:

- Determines intent with ``want_forward``  if both targets > 0 and ``want_backward`` if both targets < 0.
- Converts front/back distances from metres to centimetres
- ``d_front = usrm_front_msg.range * 100.0f``
- ``d_back  = usrm_back_msg.range  * 100.0f``
- If moving forward and a front obstacle is closer than ``FRONT_STOP_DIST`` (cm), sets ``vel_l = vel_r = 0.0f``
- If moving backward and a rear obstacle is closer than ``BACK_STOP_DIST`` (cm), also sets ``vel_l = vel_r = 0.0f``
- This prevents the tank from driving directly into obstacles in the direction of current motion.

6. Drive Motors
~~~~~~~~~~~~~~~

The motor driver is commanded with the final, safety-checked velocities:

.. code-block:: cpp

   MOTOR.move(-vel_l, vel_r);

Note the sign flip on the left side:

- The physical mounting of the motors/wiring means that a positive command on the left channel would otherwise rotate the track in the opposite direction to the right.
- Negating the left command aligns the coordinate frame so that positive values mean “forward” for both tracks from the controller’s perspective.

7. Publish Encoder Counts
~~~~~~~~~~~~~~~~~~~~~~~~~

At the end of each loop iteration:

- Reads signed tick counts from both encoders
- ``enc_left_msg.data  = - LEFT_ENCODER.get_count();``
- ``enc_right_msg.data =  RIGHT_ENCODER.get_count();``
- Publishes them on their respective topics.
- The left count is negated to ensure that forward motion yields consistent signs on both sides in ROS 2.

8. Closed-Loop Velocity Control (PID)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In the closed-loop version of the firmware, the open-loop motor commands
are refined using per-wheel PID controllers:

- ``LEFT_PID``  regulates the left wheel velocity,
- ``RIGHT_PID`` regulates the right wheel velocity.

Each controller tries to drive the measured wheel speed towards the
corresponding target from ``/cmd_vel``.

**Step 1 — Measure actual wheel velocities**

At the start of the control step (after processing ROS messages and updating
encoders), the loop computes linear velocity for each wheel using the
``Encoder::get_vel()`` method:

.. code-block:: cpp

   float vel_meas_l = LEFT_ENCODER.get_vel();   // m/s
   float vel_meas_r = RIGHT_ENCODER.get_vel();  // m/s

These values represent the estimated ground speed of each track based on
tick counts and wheel geometry.

**Step 2 — Map targets into physical units**

The ``cmd_vel_callback`` and main loop initially work with normalised
commands in :math:`[-1.0, 1.0]`:

- ``target_vel_l``, ``target_vel_r`` ∈ :math:`[-1, 1]`

To use PID in physical units, the loop scales these targets by a maximum
wheel speed constant (``V_MAX``) to obtain desired linear velocities:

.. code-block:: cpp

   float v_ref_l = target_vel_l * V_MAX;   // m/s
   float v_ref_r = target_vel_r * V_MAX;   // m/s

**Step 3 — Compute PID outputs**

Each PID controller compares the reference velocity to the measured velocity
and returns a control effort in the same normalised range used by the
``Motor`` class:

.. code-block:: cpp

   float u_l = LEFT_PID.calculate(v_ref_l, vel_meas_l);   // ~[-1, 1]
   float u_r = RIGHT_PID.calculate(v_ref_r, vel_meas_r);  // ~[-1, 1]

Internally, ``PID::calculate`` maintains:

- the **integral** of the error (for steady-state correction),
- the **previous error** (for the derivative term),
- the **time step** between calls based on ``time_us_64()``.

The result is a closed-loop command that tries to keep each wheel at its
requested speed despite load changes, battery sag, or friction.

**Step 4 — Deadband and safety**

The PID outputs are then shaped and constrained in the same way as the
open-loop version:

.. code-block:: cpp

   // Normalise to [-1, 1] if needed, then apply deadband
   float vel_l = apply_deadband(std::max(-1.0f, std::min(1.0f, u_l)), 0.15f);
   float vel_r = apply_deadband(std::max(-1.0f, std::min(1.0f, u_r)), 0.15f);

Existing directional safety checks (front/back stop distances) are applied
*after* PID, so obstacle avoidance always takes precedence over velocity
tracking. If an obstacle is too close, the loop forces:

.. code-block:: cpp

   vel_l = 0.0f;
   vel_r = 0.0f;

regardless of what the PID controllers request.

**Step 5 — Send commands to the motor driver**

Finally, the safety-checked PID outputs are passed to the low-level motor
interface:

.. code-block:: cpp

   MOTOR.move(-vel_l, vel_r);

The sign flip on the left motor remains for the same physical reasons as in
open-loop control (wiring / mounting orientation).

Why This Matters
----------------

This loop is the behavioural heart of the Pico firmware. It is where:

- sensing (ultrasonics, encoders),
- communication (micro-ROS),
- and actuation (motor driver)

are combined into a single, time-bounded control cycle.

Any change to timing, safety checks, or command computation here will
directly affect how the robot *feels* to drive and how safely it interacts
with its environment.