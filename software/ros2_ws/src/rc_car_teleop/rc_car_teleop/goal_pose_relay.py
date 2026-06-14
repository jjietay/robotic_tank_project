#!/usr/bin/env python3
"""Goal pose relay node.

This node forwards navigation goals with a zeroed timestamp. It subscribes to
``/goal_pose``, sets the message stamp to zero, and republishes the goal on
``/goal_pose_relayed``. A zero stamp tells the planner to use the latest
transform, which avoids a timestamp lookup failure on slower hardware.
"""

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import PoseStamped
from builtin_interfaces.msg import Time


class GoalPoseRelay(Node):
    """ROS 2 node that republishes goal poses with a zero timestamp.

    Subscribes:
        ``/goal_pose`` (geometry_msgs/msg/PoseStamped): the incoming goal.

    Publishes:
        ``/goal_pose_relayed`` (geometry_msgs/msg/PoseStamped): the goal with a
        zeroed stamp.
    """

    def __init__(self):
        """Set up the goal subscription and the relay publisher."""
        super().__init__('goal_pose_relay')
        self.sub = self.create_subscription(
            PoseStamped, '/goal_pose', self.cb, 10)
        self.pub = self.create_publisher(
            PoseStamped, '/goal_pose_relayed', 10)
        self.get_logger().info('Goal pose relay active, zeroing timestamps')

    def cb(self, msg: PoseStamped):
        """Zero the timestamp on a goal and republish it.

        Args:
            msg (PoseStamped): the incoming goal pose.
        """
        msg.header.stamp = Time(sec=0, nanosec=0)
        self.pub.publish(msg)
        self.get_logger().info(
            f'Relayed goal ({msg.pose.position.x:.2f}, '
            f'{msg.pose.position.y:.2f}) with stamp=0')


def main(args=None):
    """Start the goal pose relay node and spin until shutdown."""
    rclpy.init(args=args)
    node = GoalPoseRelay()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
