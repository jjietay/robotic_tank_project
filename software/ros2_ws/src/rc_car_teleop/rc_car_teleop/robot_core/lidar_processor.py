"""Lidar processing node.

Subscribes to the laser scan on ``/scan``, drops the invalid readings, and
publishes the distance to the closest valid point as a ``std_msgs/msg/Float32``
on ``/sensors/lidar/closest_point``.
"""

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import LaserScan
from std_msgs.msg import Float32


class LidarProcessorNode(Node):
    """ROS 2 node that finds the nearest obstacle in each laser scan.

    Subscribes:
        ``/scan`` (sensor_msgs/msg/LaserScan): the raw lidar scan.

    Publishes:
        ``/sensors/lidar/closest_point`` (std_msgs/msg/Float32): distance to
        the closest valid point, in metres.
    """

    def __init__(self):
        """Set up the scan subscription and the closest point publisher."""
        super().__init__("lidar_processor")
        self.sub = self.create_subscription(LaserScan, '/scan', self.scan_callback, 10)
        self.get_logger().info("LidarProcessor node started, waiting for /scan...")
        self.pub = self.create_publisher(Float32, '/sensors/lidar/closest_point', 10)

    def scan_callback(self, msg: LaserScan):
        """Keep the readings within sensor range and publish the smallest one.

        Args:
            msg (LaserScan): the incoming scan. Readings outside the sensor
                range are skipped.
        """
        valid_ranges = []
        for r in msg.ranges:
            if msg.range_min < r < msg.range_max:
                valid_ranges.append(r)

        if valid_ranges:
            closest = min(valid_ranges)
            self.get_logger().info(f"Closest object: {closest:.2f}m")
            msg_out = Float32()
            msg_out.data = closest
            self.pub.publish(msg_out)


def main(args=None):
    """Start the lidar processor node and spin until shutdown."""
    rclpy.init(args=args)
    node = LidarProcessorNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
