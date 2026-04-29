"""
ROS 2 Odometry Node for a differential drive robotoic tank.

This node subscribes to the left and right encoder tick topics ()
"""




import rclpy
import math
import tf2_ros

from rclpy.node import Node
from std_msgs.msg import Int32
from nav_msgs.msg import Odometry
from geometry_msgs.msg import TransformStamped


class OdometryNode(Node):
    def __init__(self):
        super().__init__("odometry")
        self.left_sub = self.create_subscription(Int32, '/sensors/encoders/left_ticks', self.tick_callback_left, 10)
        self.right_sub = self.create_subscription(Int32, '/sensors/encoders/right_ticks', self.tick_callback_right, 10)
        self.pub = self.create_publisher(Odometry, '/odom', 10)
        self.tf_broadcaster = tf2_ros.TransformBroadcaster(self)
        self.left_ticks = None
        self.right_ticks = None
        self.wheel_circumference = 0.14          # change accordingly - in metres
        self._wheel_base = 1                 # change accordingly - in metres
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

        # Timer runs the maths at 10Hz
        self.create_timer(0.1, self.UpdateOdometry)

    def tick_callback_left(self, msg: Int32):
        self.left_ticks = msg.data

    def tick_callback_right(self, msg: Int32):
        self.right_ticks = msg.data

    def UpdateOdometry(self):
        if self.left_ticks is None or self.right_ticks is None:
            return                                  # wait until both have arrived
        if self.last_left_ticks is None:
            self.last_left_ticks = self.left_ticks  # initialise from first real values
            self.last_right_ticks = self.right_ticks
            self.prev_time = self.get_clock().now()
            return
        
        now = self.get_clock().now()
        dt = (now - self.prev_time).nanoseconds / 1e9
        if dt <= 0.0:
            return

        delta_l = self.left_ticks  - self.last_left_ticks
        delta_r = self.right_ticks - self.last_right_ticks

        # wraparound in either direction
        INT32_RANGE = 2**32   # full range of Int32
        if delta_l > 2**31:
            delta_l -= INT32_RANGE
        elif delta_l < -2**31:
            delta_l += INT32_RANGE

        if delta_r > 2**31:
            delta_r -= INT32_RANGE
        elif delta_r < -2**31:
            delta_r += INT32_RANGE

        dist_change_left = (delta_l/4400) * self.wheel_circumference
        dist_change_right = (delta_r/4400) * self.wheel_circumference

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
        self._publish_tf(now) 
    
    # publishes to /odom --> publishes to /odom (for Nav2's EKF / robot_localization)
    def Publish(self, stamp):
        msg = Odometry()
        msg.header.stamp = stamp.to_msg()
        msg.header.frame_id = 'odom'            # fixed world frame
        msg.child_frame_id = 'base_link'        # robot's base frame
        msg.pose.pose.position.x = self.x
        msg.pose.pose.position.y = self.y
        msg.pose.pose.orientation.x = 0.0
        msg.pose.pose.orientation.y = 0.0
        msg.pose.pose.orientation.z = self.z
        msg.pose.pose.orientation.w = self.w
        msg.pose.covariance[0]  = 0.01          # x variance
        msg.pose.covariance[7]  = 0.01          # y variance
        msg.pose.covariance[35] = 0.01          # yaw variance
        msg.twist.twist.linear.x  = self.vel_linear_x
        msg.twist.twist.angular.z = self.vel_angular_z

        self.get_logger().info(f"Current Position --> x: {self.x}, y: {self.y}, theta: {self.theta}")
        self.pub.publish(msg)

    # For TF Tree
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


def main(args=None):
    rclpy.init(args=args)
    node = OdometryNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()