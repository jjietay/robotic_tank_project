import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():

    pkg_share = get_package_share_directory('tank_description')
    urdf_path  = os.path.join(pkg_share, 'urdf', 'tank.urdf')
    rviz_config = os.path.join(pkg_share, 'rviz', 'display.rviz')

    # Read URDF file content at launch time and pass it as a string parameter
    with open(urdf_path, 'r') as f:
        robot_description = f.read()

    # Only pass '-d config' to RViz if the config file already exists
    rviz_args = ['-d', rviz_config] if os.path.exists(rviz_config) else []

    return LaunchDescription([
        
        DeclareLaunchArgument(
            'use_rviz',
            default_value='true',
            description='Set to false to skip launching RViz2'
        ),

        # Publishes TF transforms from the URDF joint tree
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            name='robot_state_publisher',
            output='screen',
            parameters=[{'robot_description': robot_description}]
        ),

        # GUI slider panel to manually drive joint positions
        Node(
            package='joint_state_publisher_gui',
            executable='joint_state_publisher_gui',
            name='joint_state_publisher_gui',
            output='screen',
        ),

        # RViz2 — loads saved config if present, otherwise opens with defaults
        Node(
            package='rviz2',
            executable='rviz2',
            name='rviz2',
            output='screen',
            arguments=rviz_args,
            condition=IfCondition(LaunchConfiguration('use_rviz'))
        ),
    ])