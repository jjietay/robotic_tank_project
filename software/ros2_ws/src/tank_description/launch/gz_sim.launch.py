"""
Launch file: gz_sim.launch.py
Starts Gz Sim (Ignition) headless, bridges topics to ROS 2, and runs
robot_state_publisher so RViz can visualize everything.

Usage:
    ros2 launch tank_description gz_sim.launch.py
"""

import os
from launch import LaunchDescription
from launch.actions import ExecuteProcess, DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():

    pkg = get_package_share_directory('tank_description')
    world_file = os.path.join(pkg, 'worlds', 'tank_world.sdf')
    urdf_file  = os.path.join(pkg, 'urdf', 'tank.urdf')  # original URDF for TF

    with open(urdf_file, 'r') as f:
        robot_description = f.read()

    # ---- Start Gz Sim server (headless — no GUI on macOS) ----
    gz_sim = ExecuteProcess(
        cmd=['ign', 'gazebo', '-s', '-r', '--verbose', world_file],
        output='screen'
    )

    # ---- Bridge Gz Sim topics ↔ ROS 2 topics ----
    # Each argument is:  gz_topic@ros_msg_type[direction]gz_msg_type
    #   [  = Gz→ROS (subscribe from Gz, publish to ROS)
    #   ]  = ROS→Gz (subscribe from ROS, publish to Gz)
    #   @  = bidirectional
    bridge = Node(
        package='ros_gz_bridge',
        executable='parameter_bridge',
        arguments=[
            # cmd_vel: ROS → Gz  (teleop commands)
            '/cmd_vel@geometry_msgs/msg/Twist]ignition.msgs.Twist',

            # odom: Gz → ROS  (odometry from diff_drive)
            '/odom@nav_msgs/msg/Odometry[ignition.msgs.Odometry',

            # LIDAR scan: Gz → ROS
            '/scan@sensor_msgs/msg/LaserScan[ignition.msgs.LaserScan',

            # IMU: Gz → ROS
            '/imu@sensor_msgs/msg/Imu[ignition.msgs.IMU',

            # TF: Gz → ROS (odom → base_link from diff_drive)
            '/tf@tf2_msgs/msg/TFMessage[ignition.msgs.Pose_V',

            # Clock: Gz → ROS (so RViz timestamps sync)
            '/clock@rosgraph_msgs/msg/Clock[ignition.msgs.Clock',
        ],
        output='screen'
    )

    # ---- Robot state publisher (URDF → TF tree) ----
    # Uses the original URDF (with meshes) for the full TF tree.
    # Gz Sim handles odom→base_link; RSP handles base_link→child links.
    robot_state_pub = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        parameters=[{
            'robot_description': robot_description,
            'use_sim_time': True,
        }],
        output='screen'
    )

    return LaunchDescription([
        gz_sim,
        bridge,
        robot_state_pub,
    ])
