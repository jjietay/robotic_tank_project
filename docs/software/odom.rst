Odometry
========

This node computes wheel-encoder-based odometry for the differential drive
tank. It subscribes to left and right encoder tick counts, estimates robot
pose and velocity, publishes them on ``/odom``, and broadcasts the transform
from ``odom`` to ``base_link``.

Overview
--------

- Node name: ``odometry``
- Subscribed topics:

  - ``/sensors/encoders/left_ticks`` (type: ``std_msgs/msg/Int32``)
  - ``/sensors/encoders/right_ticks`` (type: ``std_msgs/msg/Int32``)

- Published topic:

  - ``/odom`` (type: ``nav_msgs/msg/Odometry``)

- TF broadcast:

  - ``odom`` → ``base_link``

- Main responsibilities:

  - Read cumulative encoder tick counts from both wheels
  - Compute wheel displacement from tick differences
  - Estimate robot position ``(x, y)`` and heading ``theta``
  - Convert heading into quaternion orientation
  - Estimate linear and angular velocity
  - Publish odometry for localization and navigation
  - Broadcast the robot pose into the TF tree

Module Summary
--------------

The odometry node is intended for a differential drive robot. It uses encoder
feedback from the left and right wheels to estimate how far the robot has
moved and how much it has rotated.

The output is published in two forms:

- ``/odom`` as a ``nav_msgs/msg/Odometry`` message for localization and higher-level navigation
- A TF transform from ``odom`` to ``base_link`` so that other ROS 2 nodes can
  reason about the robot's pose in the frame tree

Imports
-------

.. code-block:: python

   import rclpy
   import math
   import tf2_ros

   from rclpy.node import Node
   from std_msgs.msg import Int32
   from nav_msgs.msg import Odometry
   from geometry_msgs.msg import TransformStamped

- ``Int32`` is used for encoder tick counts.
- ``Odometry`` is the standard ROS 2 message used to publish robot pose and velocity.
- ``TransformStamped`` is used to broadcast the robot pose to the TF tree.
- ``tf2_ros.TransformBroadcaster`` sends the ``odom`` → ``base_link`` transform.

Class Definition
----------------

.. code-block:: python

   class OdometryNode(Node):

This class encapsulates all odometry logic: subscriptions, state tracking,
kinematics, odometry publishing, and TF broadcasting.

Constructor
-----------

.. code-block:: python

   def __init__(self):
       super().__init__("odometry")
       self.left_sub = self.create_subscription(
           Int32,
           '/sensors/encoders/left_ticks',
           self.tick_callback_left,
           10
       )
       self.right_sub = self.create_subscription(
           Int32,
           '/sensors/encoders/right_ticks',
           self.tick_callback_right,
           10
       )
       self.pub = self.create_publisher(Odometry, '/odom', 10)
       self.tf_broadcaster = tf2_ros.TransformBroadcaster(self)
       self.left_ticks = None
       self.right_ticks = None
       self.wheel_circumference = 0.14
       self._wheel_base = 1
       self.last_left_ticks = None
       self.last_right_ticks = None
       self.x = 0.0
       self.y = 0.0
       self.z = 0.0
       self.w = 1.0
       self.theta = 0.0
       self.vel_linear_x = 0.0
       self.vel_angular_z = 0.0
       self.prev_time = self.get_clock().now()

       self.create_timer(0.1, self.UpdateOdometry)

Key setup performed in the constructor:

- Registers the node as ``odometry``
- Subscribes to both encoder tick topics
- Creates a publisher for ``/odom``
- Creates a TF broadcaster for ``odom`` → ``base_link``
- Stores robot state variables such as position, heading, and velocity
- Starts a timer that runs ``UpdateOdometry()`` at 10 Hz

State Variables
---------------

The node keeps track of the following internal state:

- ``left_ticks`` / ``right_ticks``: latest cumulative encoder counts
- ``last_left_ticks`` / ``last_right_ticks``: previous encoder counts used to
  compute incremental motion
- ``x`` / ``y``: robot position in the odom frame
- ``theta``: robot yaw angle in radians
- ``z`` / ``w``: quaternion components derived from yaw
- ``vel_linear_x``: estimated forward linear velocity
- ``vel_angular_z``: estimated yaw angular velocity
- ``prev_time``: previous timestamp used to compute ``dt``

Encoder Callbacks
-----------------

Left encoder callback:

.. code-block:: python

   def tick_callback_left(self, msg: Int32):
       self.left_ticks = msg.data

Right encoder callback:

.. code-block:: python

   def tick_callback_right(self, msg: Int32):
       self.right_ticks = msg.data

These callbacks are simple by design:

- They store the latest cumulative tick counts
- They do not perform the odometry math directly
- The actual computation is done periodically in ``UpdateOdometry()``

This is the right pattern. Mixing heavy math directly into subscription
callbacks makes timing harder to reason about.

Odometry Update Loop
--------------------

.. code-block:: python

   def UpdateOdometry(self):
       if self.left_ticks is None or self.right_ticks is None:
           return
       if self.last_left_ticks is None:
           self.last_left_ticks = self.left_ticks
           self.last_right_ticks = self.right_ticks
           self.prev_time = self.get_clock().now()
           return

       now = self.get_clock().now()
       dt = (now - self.prev_time).nanoseconds / 1e9
       if dt <= 0.0:
           return

This function runs every 0.1 seconds and is the core of the odometry node.

The early checks do two important things:

- Wait until both encoder streams have provided real data
- Initialize previous tick values on the first valid cycle
- Protect against invalid or zero elapsed time

Tick Difference Calculation
---------------------------

.. code-block:: python

   delta_l = self.left_ticks  - self.last_left_ticks
   delta_r = self.right_ticks - self.last_right_ticks

These differences represent how many ticks each wheel has moved since the
previous update.

Wraparound Handling
-------------------

.. code-block:: python

   INT32_RANGE = 2**32
   if delta_l > 2**31:
       delta_l -= INT32_RANGE
   elif delta_l < -2**31:
       delta_l += INT32_RANGE

   if delta_r > 2**31:
       delta_r -= INT32_RANGE
   elif delta_r < -2**31:
       delta_r += INT32_RANGE

This logic handles encoder counter wraparound when the raw tick count crosses
the signed 32-bit integer limit. Without this, a valid rollover could appear
as a huge false jump in wheel motion.

That is a good defensive design choice. Raw encoder counters eventually roll
over, and ignoring that will silently corrupt odometry.

Wheel Distance Calculation
--------------------------

.. code-block:: python

   dist_change_left = (delta_l / 4400) * self.wheel_circumference
   dist_change_right = (delta_r / 4400) * self.wheel_circumference

Here:

- ``4400`` is the effective ticks per wheel revolution
- ``wheel_circumference`` converts revolutions into linear travel distance

This gives the left and right wheel travel distances in metres for the current
update step.

Differential Drive Kinematics
-----------------------------

.. code-block:: python

   d_center = (dist_change_left + dist_change_right) / 2
   angle_change = (dist_change_right - dist_change_left) / self._wheel_base

- ``d_center`` is the robot's forward displacement
- ``angle_change`` is the change in heading

This is the standard differential drive approximation: the average of both
wheel distances gives forward motion, while their difference divided by wheel
base gives rotation.

Pose and Velocity Update
------------------------

.. code-block:: python

   self.x += d_center * math.cos(self.theta)
   self.y += d_center * math.sin(self.theta)
   self.theta += angle_change
   self.z = math.sin(self.theta / 2)
   self.w = math.cos(self.theta / 2)
   self.vel_linear_x = d_center / dt
   self.vel_angular_z = angle_change / dt

This updates:

- Position ``(x, y)`` in the odom frame
- Heading ``theta``
- Quaternion orientation components ``z`` and ``w`` for planar yaw
- Linear and angular velocity estimates

A useful detail: for a planar ground robot, only the yaw component changes, so
quaternion ``x`` and ``y`` stay at zero while ``z`` and ``w`` encode the
rotation.

Publishing /odom
----------------

.. code-block:: python

   def Publish(self, stamp):
       msg = Odometry()
       msg.header.stamp = stamp.to_msg()
       msg.header.frame_id = 'odom'
       msg.child_frame_id = 'base_link'
       msg.pose.pose.position.x = self.x
       msg.pose.pose.position.y = self.y
       msg.pose.pose.orientation.x = 0.0
       msg.pose.pose.orientation.y = 0.0
       msg.pose.pose.orientation.z = self.z
       msg.pose.pose.orientation.w = self.w
       msg.pose.covariance[0]  = 0.01
       msg.pose.covariance[7]  = 0.01
       msg.pose.covariance[35] = 0.01
       msg.twist.twist.linear.x  = self.vel_linear_x
       msg.twist.twist.angular.z = self.vel_angular_z
       self.pub.publish(msg)

The ``/odom`` message contains:

- Frame relationship: ``odom`` as parent and ``base_link`` as child
- Pose estimate: position + quaternion orientation
- Velocity estimate: linear x and angular z
- Basic covariance entries for x, y, and yaw

This is the topic that localization and navigation systems typically consume. ROS 2 navigation stacks commonly use ``nav_msgs/msg/Odometry`` on ``/odom`` as part of the robot state estimation pipeline.

Broadcasting TF
---------------

.. code-block:: python

   def _publish_tf(self, stamp):
       t = TransformStamped()
       t.header.stamp = stamp.to_msg()
       t.header.frame_id = 'odom'
       t.child_frame_id = 'base_link'
       t.transform.translation.x = self.x
       t.transform.translation.y = self.y
       t.transform.translation.z = 0.0
       t.transform.rotation.x = 0.0
       t.transform.rotation.y = 0.0
       t.transform.rotation.z = self.z
       t.transform.rotation.w = self.w
       self.tf_broadcaster.sendTransform(t)

This publishes the robot pose into the TF tree so other nodes can transform
data between frames. TF broadcasters send ``TransformStamped`` messages that
include timestamp, parent frame, child frame, translation, and rotation.

For this node, the transform is:

- Parent frame: ``odom``
- Child frame: ``base_link``

That means the node is telling the rest of the system where the robot base is
relative to the odom frame.

Program Entry
-------------

.. code-block:: python

   def main(args=None):
       rclpy.init(args=args)
       node = OdometryNode()
       rclpy.spin(node)
       node.destroy_node()
       rclpy.shutdown()

   if __name__ == '__main__':
       main()

- ``rclpy.init()`` initializes the ROS 2 client library
- ``OdometryNode()`` constructs the node and starts its subscriptions, publisher,
  broadcaster, and timer
- ``rclpy.spin(node)`` keeps the node alive and processes callbacks
- ``destroy_node()`` and ``shutdown()`` cleanly stop the node
