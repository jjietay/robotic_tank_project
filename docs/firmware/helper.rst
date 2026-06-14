Helper Functions
================

See also: :doc:`main.cpp <main_cpp>`

Purpose
-------

These are the small helper functions in ``main.cpp``.

clampf
------

.. code-block:: cpp

   static inline float clampf(float x, float lo, float hi)
   {
       if (x < lo) return lo;
       if (x > hi) return hi;
       return x;
   }

Keeps a value inside a low and high limit. The control loop uses it to keep the
motor duty inside the range from -1 to 1 after the feedforward and
PID are added together.

init_range_msg
--------------

.. code-block:: cpp

   static void init_range_msg(sensor_msgs__msg__Range* msg, uint8_t rad_type,
                              float fov, float min_r, float max_r)
   {
       msg->radiation_type = rad_type;
       msg->field_of_view  = fov;
       msg->min_range = min_r;
       msg->max_range = max_r;
       msg->range  = 0.0f;
   }

Fills in the fixed fields of a range message: the type, the cone angle, and the
smallest and largest distance the sensor can report. The four ultrasonic
messages are set up with the same call, so they all share the same settings and
only differ in their frame name and their measured value.

set_msg_stamp
-------------

.. code-block:: cpp

   static void set_msg_stamp(std_msgs__msg__Header* header)
   {
       uint64_t now_us = time_us_64();
       header->stamp.sec = now_us / 1000000ULL;
       header->stamp.nanosec = (now_us % 1000000ULL) * 1000ULL;
   }

Stamps a message with the current time from the Pico clock, split into seconds
and nanoseconds the way ROS 2 expects. Every published message uses this same
clock, which keeps the timestamps consistent for the Raspberry Pi to use.
