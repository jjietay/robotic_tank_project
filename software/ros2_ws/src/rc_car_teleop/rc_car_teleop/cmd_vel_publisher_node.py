import rclpy
from rclpy.node import Node

from geometry_msgs.msg import Twist


class CmdVelPublisher(Node): # Inherits from Node

    def __init__(self):
        super().__init__('minimal_publisher')
        self.publisher_ = self.create_publisher(Twist, '/cmd_vel', 10)
        timer_period = 0.5  # seconds
        self.timer = self.create_timer(timer_period, self.timer_callback)
        self.i = 0

    def timer_callback(self):
        msg = Twist()
        msg.linear.x = 0.1   # forward speed
        msg.angular.z = 0.0  # no rotation
        self.publisher_.publish(msg)
        self.get_logger().info(f"Publishing: linear.x={msg.linear.x}, angular.z={msg.angular.z}")
        self.i += 1


def main(args=None):
    rclpy.init(args=args)

    cmd_vel_publisher = CmdVelPublisher()

    rclpy.spin(cmd_vel_publisher)

    # Destroy the node explicitly
    # (optional - otherwise it will be done automatically
    # when the garbage collector destroys the node object)
    cmd_vel_publisher.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()