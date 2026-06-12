#!/usr/bin/env python3
"""
Goal Pose Relay — zeros the timestamp on incoming goal poses.

Nav2 Humble bug: the planner keeps transforming the goal at its original
timestamp. During recovery cycles on slow hardware (RPi4), that timestamp
falls out of the TF buffer and replanning fails forever.

Fix: this node sits between Foxglove and bt_navigator.
  Foxglove publishes to /goal_pose_raw  (remapped in launch)
  This node zeros the stamp and republishes to /goal_pose
  bt_navigator picks it up with Time(0) → TF uses latest transform.

See: https://github.com/ros-planning/navigation2/issues/3075
"""

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import PoseStamped
from builtin_interfaces.msg import Time


class GoalPoseRelay(Node):
    def __init__(self):
        super().__init__('goal_pose_relay')
        self.sub = self.create_subscription(
            PoseStamped, '/goal_pose', self.cb, 10)
        self.pub = self.create_publisher(
            PoseStamped, '/goal_pose_relayed', 10)
        self.get_logger().info('Goal pose relay active — zeroing timestamps')

    def cb(self, msg: PoseStamped):
        msg.header.stamp = Time(sec=0, nanosec=0)
        self.pub.publish(msg)
        self.get_logger().info(
            f'Relayed goal ({msg.pose.position.x:.2f}, '
            f'{msg.pose.position.y:.2f}) with stamp=0')


def main(args=None):
    rclpy.init(args=args)
    node = GoalPoseRelay()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
