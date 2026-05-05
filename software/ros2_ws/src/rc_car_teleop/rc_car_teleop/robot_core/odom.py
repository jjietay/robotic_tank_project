"""
ROS 2 Odometry Node for a differential drive robotoic tank.

This node subscribes to the left and right encoder tick topics (sensors/encoders/right_ticks & sensors/encoders/left_ticks). It recieves raw int32 which is the raw tick counts,
and computes wheel displacement + angle of rotation (pose) in the form of quaternions (x,y,z,rotation_angle) and velocity.

It publishes to /odom (for Nav2's EKF/localization) and /tf (to TF tree, representing transforms of base_link)
"""

import rclpy
import math
import tf2_ros

from rclpy.node import Node
from std_msgs.msg import Int32
from nav_msgs.msg import Odometry
from geometry_msgs.msg import TransformStamped


class OdometryNode(Node):
    """
    Computes and publishes wheel encoder-based calculated odometry.

    Subscriptions:
        /sensors/encoders/left_ticks (sensor_msgs.msg.Int32)  : Left encoder ticks
        /sensors/encoders/right_ticks (sensor_msgs.msg.Int32) : Right encoder ticks

    Publishers:
        /odom (std_msgs.msg.Odometry) : Estimated robot's pose and velocity
    
    Broadcaster to TF:
        Broadcast the transform of base_link with respect to odom frame or /odom

    Notes:
        This implementation assumes a differential drive robot. 
        Wheel circumference, wheel base, and encoder ticks per rev are used to estimate motion.
    """

    def __init__(self):
        """
        Initialize:
            odometry        : Name of the node

        Local variable:
            left_sub        : Subscriber to left ticks of left motor's encoder
            right_sub       : Subscriber to right ticks of right motor's encoder
            pub             : Publisher of robot's pose and velocity
            tf_broadcaster  : Broadcaster of transforms between /odom frame and base_link's frame
        """

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
        """
        Get the total number of left ticks and stores it in 'left_ticks', that is available throughout entire class. msg: Int32 just tells us that we should expect Int32 type for the msg.
        Does not change how the code is run, purely for documentation.
        """
        self.left_ticks = msg.data

    def tick_callback_right(self, msg: Int32):
        """
        Get the total number of right ticks and stores it in 'right_ticks', that is available throughout entire class. msg: Int32 just tells us that we should expect Int32 type for the msg.
        Does not change how the code is run, purely for documentation.
        """
        self.right_ticks = msg.data

    def UpdateOdometry(self):
        """
        First we check presence of left_ticks and right_ticks. If none we get out of this function. If present, we store it as last_X_ticks.
        Next, we check if there is data in last_left_ticks. If none, we store last_left_ticks using current left_ticks value, vice versa, and start timer as prev_time
        We then start a timer using get_clock (used for velocity calculation).
        
        Helper calculations:
            dt:             represents change in time --> velocity calculation
            delta_X:        schange in ticks from then and now
            INT32_RANGE:    represents the full range of Int32 values (-2,147,483,648 to 2,147,483,647)
            wraparound:     this helps to wrap around so we don't get negative tick values
            4400:           encoder ticks per rev == encoder_pulses_per_rev (PPR -> 11) * gear_ratio (100:1) * quadrature
            now:            this represents current time that is passed to functions Publish and _publish_tf as timestamps
        """

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