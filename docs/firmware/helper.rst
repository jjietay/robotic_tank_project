Helper Functions
================

See also: :doc:`main.cpp <main_cpp>`

Purpose
-------

This page documents the small utility functions used by the firmware.
They are easy to overlook, but they encode important assumptions about
timing, message formats, and how commands are shaped before reaching
the hardware.

Functions
---------

(a) apply_deadband(u, deadband=0.15f)
~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   float apply_deadband(float u, float deadband = 0.15f)
   {
       if (u == 0.0f) return 0.0f;
       float sign = (u > 0.0f) ? 1.0f : -1.0f;
       float mag  = std::fabs(u);
       if (mag < deadband) mag = deadband;
       if (mag > 1.0f) mag = 1.0f;
       return sign * mag;
   }

**1) Purpose**

- Forces very small commands either to zero or to a minimum effective magnitude.
- Compensates for motor deadzone: a small PWM duty often produces no movement.

**2) Behaviour**

- If ``u`` is exactly zero, the output is zero.
- Otherwise, the sign of ``u`` is preserved, but the magnitude is:
  - raised to at least ``deadband`` (e.g. 0.15),
  - clamped to at most 1.0.


(b) init_range_msg(...)
~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void init_range_msg(sensor_msgs__msg__Range* msg, uint8_t rad_type,
                       float fov, float min_r, float max_r)
   {
       msg->radiation_type = rad_type;
       msg->field_of_view  = fov;
       msg->min_range      = min_r;
       msg->max_range      = max_r;
       msg->range          = 0.0f;
   }

**1) Purpose**

- Initialises a ``sensor_msgs/msg/Range`` message with consistent metadata for
  all ultrasonic sensors.

**2) Fields set**

- ``radiation_type`` — e.g. 0 for ULTRASOUND.
- ``field_of_view`` — sensor cone angle in radians (e.g. ≈ 0.26 rad).
- ``min_range`` / ``max_range`` — valid measurement range in metres.
- ``range`` — initial measurement value (set to 0.0).

By centralising this initialisation, all range topics share identical metadata
and only differ in their frame IDs and measured values.

(c) set_msg_stamp(...)
~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void set_msg_stamp(std_msgs__msg__Header* header)
   {
       uint64_t now_us = time_us_64();
       header->stamp.sec     = now_us / 1000000ULL;
       header->stamp.nanosec = (now_us % 1000000ULL) * 1000ULL;
   }

**1) Purpose**

- Fills the ROS 2 message header timestamp using the Pico’s microsecond timer.

**2) Details**

- ``time_us_64()`` provides the current time in microseconds since boot.
- The function converts this to:
  - seconds (``stamp.sec``),
  - nanoseconds (``stamp.nanosec``),
  matching ROS 2’s standard time format.

This ensures all published messages (ultrasonic readings, encoder ticks, etc.)
are timestamped from a single consistent clock, which is important for
time-synchronised processing on the Raspberry Pi.

Why This Matters
----------------

Helper functions are easy to ignore, but they often contain important
system assumptions:

- ``apply_deadband`` shapes how control commands feel — it determines whether
  small inputs do nothing, jitter, or move smoothly.
- ``init_range_msg`` guarantees that all ultrasonic topics present a uniform
  interface to ROS 2, simplifying downstream consumers.
- ``set_msg_stamp`` anchors every message to the same time base, which is
  critical for sensor fusion and replaying bag files.

Tweaking these helpers changes the *character* of the robot without touching
the higher-level logic.