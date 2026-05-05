Lidar Processor
===============

This node subscribes to raw LIDAR scans on ``/scan``, filters out invalid
readings, and publishes the distance to the closest valid obstacle as a
single ``Float32`` value.

Overview
--------

- Node name: ``lidar_processor``
- Subscribed topic: ``/scan`` (type: ``sensor_msgs/msg/LaserScan``)
- Published topic: ``/sensors/lidar/closest_point`` (type: ``std_msgs/msg/Float32``)
- Main responsibilities:

  - Receive raw LIDAR ranges from ``/scan``
  - Filter out invalid readings outside ``(range_min, range_max)``
  - Compute the minimum valid distance
  - Publish the closest distance as a single scalar value

Node Structure
--------------

Imports
~~~~~~~

.. code-block:: python

   import rclpy
   from rclpy.node import Node
   from sensor_msgs.msg import LaserScan
   from std_msgs.msg import Float32

- ``LaserScan`` represents a full 2D LIDAR scan (array of ranges + metadata).
- ``Float32`` is used to publish just one number: the closest valid distance.

Class Definition
~~~~~~~~~~~~~~~~

.. code-block:: python

   class LidarProcessorNode(Node):

- The node is implemented as a subclass of ``Node``.
- Encapsulates subscription, processing logic, and publishing.

Constructor
~~~~~~~~~~~

.. code-block:: python

   def __init__(self):
       super().__init__("lidar_processor")
       self.sub = self.create_subscription(
           LaserScan,
           '/scan',
           self.scan_callback,
           10
       )
       self.get_logger().info("LidarProcessor node started, waiting for /scan...")
       self.pub = self.create_publisher(
           Float32,
           '/sensors/lidar/closest_point',
           10
       )

- ``super().__init__("lidar_processor")`` registers the node with the name
  ``lidar_processor``.
- ``create_subscription(...)`` subscribes to the LIDAR topic ``/scan`` with
  message type ``LaserScan`` and callback ``scan_callback``.
- A log message confirms the node has started and is waiting for data.
- ``create_publisher(...)`` sets up a publisher on
  ``/sensors/lidar/closest_point`` to output the closest distance as a
  ``Float32``.

Scan Callback
-------------

.. code-block:: python

   def scan_callback(self, msg: LaserScan):
       # msg.ranges is a list of distances in metres, one per angle step
       # Invalid readings come back as float('inf') or 0.0 — filter them out
       valid_ranges = []
       for r in msg.ranges:
           if msg.range_min < r < msg.range_max:
               valid_ranges.append(r)

       if valid_ranges:    # list is not empty
           closest = min(valid_ranges)
           self.get_logger().info(f"Closest object: {closest:.2f}m")
           msg_out = Float32()
           msg_out.data = closest
           self.pub.publish(msg_out)

- ``msg.ranges`` is a list of distance readings (in metres), one per angle.
- Readings outside the valid window ``(msg.range_min, msg.range_max)`` are
  treated as invalid and skipped.
- If any valid readings remain, the node:

  - Computes ``closest = min(valid_ranges)``
  - Logs the closest distance to two decimal places
  - Constructs a ``Float32`` message and sets ``data`` to the closest value
  - Publishes the result on ``/sensors/lidar/closest_point``

Main Function
-------------

.. code-block:: python

   def main(args=None):
       rclpy.init(args=args)
       node = LidarProcessorNode()
       rclpy.spin(node)
       node.destroy_node()
       rclpy.shutdown()

   if __name__ == "__main__":
       main()

- ``rclpy.init()`` initializes the ROS 2 client library.
- ``LidarProcessorNode()`` creates the node and sets up subscriptions and
  publishers.
- ``rclpy.spin(node)`` enters the event loop and processes incoming ``/scan``
  messages until shutdown.
- On exit, the node is destroyed and the ROS 2 client library is shut down.

Usage Notes
-----------

- This node expects a standard ``/scan`` topic from a LIDAR driver (publishing
  ``sensor_msgs/msg/LaserScan``).
- The output topic ``/sensors/lidar/closest_point`` can be used by higher-level
  safety or navigation nodes that only need the nearest obstacle distance
  instead of the full scan.