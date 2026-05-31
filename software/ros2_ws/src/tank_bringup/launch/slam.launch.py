import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import TimerAction


def generate_launch_description():

    pkg_share = get_package_share_directory('tank_description')
    urdf_path = os.path.join(pkg_share, 'urdf', 'tank.urdf')
    with open(urdf_path, 'r') as f:
        robot_description = f.read()

    ydlidar_params = os.path.join(
        get_package_share_directory('tank_bringup'),
        'config', 'ydlidar.yaml'
    )

    slam_params = os.path.join(
        get_package_share_directory('tank_bringup'),
        'config', 'slam_toolbox.yaml'
    )

    return LaunchDescription([

        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            name='robot_state_publisher',
            output='screen',
            parameters=[{
                'robot_description': robot_description,
                'use_sim_time': False
            }]
        ),

        Node(
            package='joint_state_publisher',
            executable='joint_state_publisher',
            name='joint_state_publisher',
            output='screen',
        ),

        Node(
            package='ydlidar_ros2_driver',
            executable='ydlidar_ros2_driver_node',
            name='ydlidar_ros2_driver_node',
            output='screen',
            parameters=[ydlidar_params],
        ),

        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            name='static_odom_tf',
            arguments=['0', '0', '0', '0', '0', '0', 'odom', 'base_link'],
        ),
        
        TimerAction(
            period=5.0,
            actions=[Node(
                package='slam_toolbox',
                executable='async_slam_toolbox_node',
                name='slam_toolbox',
                output='screen',
                parameters=[slam_params],
            )]
        ),

    ])