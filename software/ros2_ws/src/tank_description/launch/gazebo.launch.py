"""
Launch file: gazebo.launch.py
Starts Gazebo with an empty world, spawns the tank, and runs robot_state_publisher.

Usage:
    ros2 launch tank_description gazebo.launch.py
"""

import os
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():

    pkg = get_package_share_directory('tank_description')
    urdf_file = os.path.join(pkg, 'urdf', 'tank_gazebo.urdf')

    with open(urdf_file, 'r') as f:
        robot_description = f.read()

    # ---- Start Gazebo with empty world ----
    gazebo = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(
                get_package_share_directory('gazebo_ros'),
                'launch', 'gazebo.launch.py'
            )
        )
    )

    # ---- Publish robot description + TF tree ----
    robot_state_pub = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        parameters=[{'robot_description': robot_description}],
        output='screen'
    )

    # ---- Spawn the tank into Gazebo ----
    spawn_entity = Node(
        package='gazebo_ros',
        executable='spawn_entity.py',
        arguments=[
            '-topic', 'robot_description',
            '-entity', 'my_tank',
            '-z', '0.05',       # spawn slightly above ground so it drops cleanly
        ],
        output='screen'
    )

    return LaunchDescription([
        gazebo,
        robot_state_pub,
        spawn_entity,
    ])