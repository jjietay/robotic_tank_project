"""Wheel odometry node for the differential drive tank.

This node reads the left and right wheel encoder tick counts, works out how
far the robot has moved and how much it has turned, and publishes the result
as a ``nav_msgs/msg/Odometry`` message on ``/odom``. It can also broadcast the
``odom`` to ``base_link`` transform.
"""

import rclpy
import math
import tf2_ros

from rclpy.node import Node
from std_msgs.msg import Int32
from nav_msgs.msg import Odometry
from geometry_msgs.msg import TransformStamped


class OdometryNode(Node):
    """ROS 2 node that turns encoder ticks into an odometry estimate.

    Subscribes:
        ``/sensors/encoders/left_ticks`` (std_msgs/msg/Int32): left wheel ticks.
        ``/sensors/encoders/right_ticks`` (std_msgs/msg/Int32): right wheel ticks.

    Publishes:
        ``/odom`` (nav_msgs/msg/Odometry): estimated pose and velocity.

    Attributes:
        wheel_circumference (float): wheel circumference in metres.
        _wheel_base (float): distance between the two tracks in metres.
        x (float): robot x position in the odom frame, in metres.
        y (float): robot y position in the odom frame, in metres.
        theta (float): robot heading in radians.
    """

    def __init__(self):
        """Set up the subscriptions, publisher, state, and update timer."""
        super().__init__("odometry")
        self.left_sub = self.create_subscription(Int32, '/sensors/encoders/left_ticks', self.tick_callback_left, 10)
        self.right_sub = self.create_subscription(Int32, '/sensors/encoders/right_ticks', self.tick_callback_right, 10)
        self.pub = self.create_publisher(Odometry, '/odom', 10)
        self.tf_broadcaster = tf2_ros.TransformBroadcaster(self)
        self.left_ticks = None
        self.right_ticks = None
        self.wheel_circumference = 0.1381
        self._wheel_base = 0.220
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

        self.create_timer(0.05, self.UpdateOdometry)

    def tick_callback_left(self, msg: Int32):
        """Store the latest left wheel tick count."""
        self.left_ticks = msg.data

    def tick_callback_right(self, msg: Int32):
        """Store the latest right wheel tick count."""
        self.right_ticks = msg.data

    def UpdateOdometry(self):
        """Update the pose and velocity from the newest tick counts.

        Runs on a timer. It waits until both encoders have reported, finds the
        change in ticks since the last run, handles the 32 bit counter
        wraparound, converts ticks into wheel travel, and applies differential
        drive kinematics to update the position, heading, and velocity. The
        result is then published on ``/odom``.
        """
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

        delta_l = self.left_ticks - self.last_left_ticks
        delta_r = self.right_ticks - self.last_right_ticks

        INT32_RANGE = 2**32
        if delta_l > 2**31:
            delta_l -= INT32_RANGE
        elif delta_l < -2**31:
            delta_l += INT32_RANGE

        if delta_r > 2**31:
            delta_r -= INT32_RANGE
        elif delta_r < -2**31:
            delta_r += INT32_RANGE

        dist_change_left = (delta_l/5764) * self.wheel_circumference
        dist_change_right = (delta_r/5764) * self.wheel_circumference

        d_center = (dist_change_left + dist_change_right)/2
        angle_change = (dist_change_right - dist_change_left) / self._wheel_base

        self.x += d_center * math.cos(self.theta)
        self.y += d_center * math.sin(self.theta)
        self.theta += angle_change
        self.z = math.sin(self.theta / 2)
        self.w = math.cos(self.theta / 2)
        self.vel_linear_x = d_center / dt
        self.vel_angular_z = angle_change / dt

        self.last_left_ticks = self.left_ticks
        self.last_right_ticks = self.right_ticks
        self.prev_time = now
        self.Publish(now)

    def Publish(self, stamp):
        """Build and publish the odometry message.

        Args:
            stamp: ROS time used for the message header.
        """
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
        msg.twist.covariance[0]  = 0.01
        msg.twist.covariance[35] = 0.01

        self.pub.publish(msg)

    def _publish_tf(self, stamp):
        """Broadcast the odom to base_link transform.

        Args:
            stamp: ROS time used for the transform header.
        """
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


def main(args=None):
    """Start the odometry node and spin until shutdown."""
    rclpy.init(args=args)
    node = OdometryNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
