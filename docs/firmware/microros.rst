micro ROS
=========

See also: :doc:`main.cpp <main_cpp>`

Purpose
-------

This page covers the micro ROS parts of the firmware. micro ROS is the bridge
between the ROS 2 system on the Raspberry Pi 4 and the control loop on the
Pico W.

What it sets up
---------------

The firmware creates:

- one ``/cmd_vel`` subscriber (``geometry_msgs/msg/Twist``),
- four ultrasonic range publishers (``sensor_msgs/msg/Range``),
- two encoder tick publishers (``std_msgs/msg/Int32``),
- the message buffers for each of these,
- the micro ROS support objects: the allocator, the support struct, the
  executor, and the node

Node and transport
------------------

Micro ROS uses the Pico UART as its transport, then
creates a node that shows up in the ROS 2 graph on the Raspberry Pi:

.. code-block:: cpp

   rmw_uros_set_custom_transport(
       true, NULL,
       pico_serial_transport_open,  pico_serial_transport_close,
       pico_serial_transport_write, pico_serial_transport_read);

   allocator = rcl_get_default_allocator();
   rclc_support_init(&support, 0, NULL, &allocator);
   rclc_node_init_default(&node, "pico", "", &support);

The node is named ``pico``, so tools like ``ros2 node list`` on the Pi can find
it.

The command callback
--------------------

When a ``/cmd_vel`` message arrives, the callback reads the forward speed and
the turn rate and splits them into a target speed for each track:

.. code-block:: cpp

   void cmd_vel_callback(const void* msg_in) {
       const auto* msg = (const geometry_msgs__msg__Twist*)msg_in;
       float v_mps = (float)msg->linear.x;
       float w_rps = (float)msg->angular.z;
       target_vel_l_mps = v_mps - w_rps * (WHEEL_BASE_M * 0.5f);
       target_vel_r_mps = v_mps + w_rps * (WHEEL_BASE_M * 0.5f);
       last_cmd_vel_time_us = time_us_64();
   }

This is the standard differential drive split. The forward speed moves both
tracks together, and the turn rate speeds up one track and slows the other.
Both targets are kept in metres per second, which is what the control loop and
the encoders use. The time of the last command is saved so the loop can stop
the tank if commands stop arriving.

The executor
------------

The executor checks the transport for new messages and runs the callback:

.. code-block:: cpp

   rclc_executor_init(&executor, &support.context, 1, &allocator);
   rclc_executor_add_subscription(
       &executor, &cmd_vel_sub, &cmd_vel_msg, &cmd_vel_callback, ON_NEW_DATA);

The control loop calls ``rclc_executor_spin_some`` once per cycle, so the
callback runs often without blocking the rest of the loop.

Data flow
---------

End to end, the flow is:

1. A node on the Raspberry Pi publishes ``/cmd_vel``.
2. micro ROS on the Pico reads it and updates the two target speeds.
3. The control loop reads the sensors, runs the safety checks and the PID, and
   drives the motors.
4. The ultrasonic ranges and encoder ticks are published back to ROS 2.
