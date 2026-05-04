micro-ROS
=========

See also: :doc:`main.cpp <main_cpp>`

Purpose
-------

This page documents the micro-ROS-specific parts of the Pico W firmware.
These elements form the communication bridge between the Raspberry Pi 4
ROS 2 system and the low-level control loop on the Pico.

Main Elements
-------------

The firmware sets up the following micro-ROS entities:

- one ``/cmd_vel`` subscriber (``geometry_msgs/msg/Twist``),
- four ultrasonic range publishers (``sensor_msgs/msg/Range``),
- two encoder tick publishers (``std_msgs/msg/Int32``),
- message buffers for all of the above,
- the micro-ROS support objects: allocator, support, executor, and node.


Node and Transport Setup
------------------------

The firmware configures micro-ROS to use the Pico’s UART as its transport and
creates a node that will appear in the ROS 2 graph on the Raspberry Pi:

.. code-block:: cpp

   rmw_uros_set_custom_transport(
       true,
       NULL,
       pico_serial_transport_open,
       pico_serial_transport_close,
       pico_serial_transport_write,
       pico_serial_transport_read);

   allocator = rcl_get_default_allocator();
   rclc_support_init(&support, 0, NULL, &allocator);
   rclc_node_init_default(&node, "pico", "", &support);

Key points:

- A **custom transport** bridges Pico UART ↔ micro-ROS client library.
- The node is named ``"pico"``, so tools like ``ros2 node list`` on the Pi
  can discover it.

Subscribers and Publishers
--------------------------

The firmware declares one subscriber and several publishers:

.. code-block:: cpp

   // /cmd_vel subscriber
   rclc_subscription_init_default(
       &cmd_vel_sub, &node,
       ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist),
       "/cmd_vel");

   // Ultrasonic sensors
   rclc_publisher_init_default(
       &usrm_front_pub, &node,
       ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Range),
       "/sensors/ultrasonic/usrm_front");
   // ... similarly for back, left, right

   // Encoders
   rclc_publisher_init_default(
       &enc_left_pub,  &node,
       ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
       "/sensors/encoders/left_ticks");
       
   rclc_publisher_init_default(
       &enc_right_pub, &node,
       ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
       "/sensors/encoders/right_ticks");

Each publisher/subscriber has a corresponding **message buffer** allocated
statically (e.g. ``geometry_msgs__msg__Twist cmd_vel_msg;``). The executor
uses these buffers when dispatching callbacks and publishing data.

Callback Flow
-------------

The ``cmd_vel_callback`` receives a ``geometry_msgs/msg/Twist`` message and
maps linear and angular velocity commands into left and right wheel targets:

.. code-block:: cpp

   void cmd_vel_callback(const void* msg_in) {
       const geometry_msgs__msg__Twist* msg =
           (const geometry_msgs__msg__Twist*)msg_in;

       float linear  = (float)msg->linear.x;
       float angular = (float)msg->angular.z;

       // Differential drive mixing
       target_vel_l = linear - angular;
       target_vel_r = linear + angular;

       // Clamp to [-1.0, 1.0]
       target_vel_l = std::max(-1.0f, std::min(1.0f, target_vel_l));
       target_vel_r = std::max(-1.0f, std::min(1.0f, target_vel_r));
   }

This performs the standard **differential drive mix**:

.. math::

   v_L = v_{\text{linear}} - v_{\text{angular}}, \quad
   v_R = v_{\text{linear}} + v_{\text{angular}}

The resulting ``target_vel_l`` and ``target_vel_r`` are then consumed in the
main loop, converted through ``apply_deadband``, and passed to the ``Motor``
class to drive PWM outputs.

Executor Integration
--------------------

The micro-ROS executor is responsible for polling the transport and calling
callbacks:

.. code-block:: cpp

   rclc_executor_init(&executor, &support.context, 1, &allocator);
   rclc_executor_add_subscription(
       &executor, &cmd_vel_sub, &cmd_vel_msg,
       &cmd_vel_callback, ON_NEW_DATA);

In the main loop:

.. code-block:: cpp

   while (true) {
       // ...
       rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10));
       // ...
   }

This pattern:

- checks for new messages without blocking the rest of the control loop,
- guarantees that ``cmd_vel_callback`` runs frequently enough to track the
  commands from the Raspberry Pi.

Data Flow Summary
-----------------

End-to-end, the system works like this:

.. code-block:: text

   1. High-level controller on the Raspberry Pi publishes /cmd_vel.
   2. micro-ROS on the Pico receives /cmd_vel and updates target_vel_l/r.
   3. The main loop:
      - reads sensors (ultrasonic, encoders),
      - applies safety checks and deadband,
      - calls Motor.move(...) using the latest targets.
   4. Sensor data (range messages and encoder ticks) are published back
      to ROS 2 for monitoring or higher-level algorithms.

Design Note
-----------

This micro-ROS layer is the **contract** between high-level ROS 2 code and
low-level firmware:

- If messages are delayed, dropped, or mis-scaled, the robot will feel laggy
  or unstable even if the motor and sensor code is correct.
- Clear topic names, consistent units, and reliable timestamps make it much
  easier to debug behaviour from the ROS 2 side without touching firmware.

As the project grows, changes to this interface (e.g. new topics, different
units, new frames) should be treated as API changes and documented carefully.