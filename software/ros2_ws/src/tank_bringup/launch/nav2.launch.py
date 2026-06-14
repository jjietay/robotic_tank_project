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

    bringup_share = get_package_share_directory('tank_bringup')

    ydlidar_params    = os.path.join(bringup_share, 'config', 'ydlidar.yaml')
    ekf_params        = os.path.join(bringup_share, 'config', 'ekf.yaml')
    nav2_params       = os.path.join(bringup_share, 'config', 'nav2_params.yaml')
    twist_mux_config  = os.path.join(bringup_share, 'config', 'twist_mux.yaml')
    map_yaml          = '/home/jj/robotic_tank_project/maps/my_map.yaml'

    return LaunchDescription([


        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            name='robot_state_publisher',
            output='screen',
            parameters=[{
                'robot_description': robot_description,
                'use_sim_time': False,
            }],
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
            output='log',
            parameters=[ydlidar_params],
        ),

        Node(
            package='rc_car_teleop',
            executable='bno085_i2c',
            name='bno085_i2c_node',
            output='screen',
        ),

        Node(
            package='rc_car_teleop',
            executable='odometry',
            name='odometry',
            output='screen',
        ),



        Node(
            package='twist_mux',
            executable='twist_mux',
            name='twist_mux',
            output='screen',
            parameters=[twist_mux_config],
            remappings=[('cmd_vel_out', '/cmd_vel')],
        ),


        TimerAction(
            period=3.0,
            actions=[Node(
                package='robot_localization',
                executable='ekf_node',
                name='ekf_filter_node',
                output='screen',
                parameters=[ekf_params],
            )],
        ),


        TimerAction(
            period=8.0,
            actions=[Node(
                package='nav2_map_server',
                executable='map_server',
                name='map_server',
                output='screen',
                parameters=[{
                    'yaml_filename': map_yaml,
                    'use_sim_time': False,
                }],
            )],
        ),

        TimerAction(
            period=8.0,
            actions=[Node(
                package='nav2_amcl',
                executable='amcl',
                name='amcl',
                output='screen',
                parameters=[nav2_params],
            )],
        ),

        TimerAction(
            period=12.0,
            actions=[Node(
                package='nav2_planner',
                executable='planner_server',
                name='planner_server',
                output='screen',
                parameters=[nav2_params],
            )],
        ),

        TimerAction(
            period=12.0,
            actions=[Node(
                package='nav2_controller',
                executable='controller_server',
                name='controller_server',
                output='screen',
                parameters=[nav2_params],
                remappings=[('cmd_vel', '/cmd_vel_raw')],
            )],
        ),

        TimerAction(
            period=12.0,
            actions=[Node(
                package='nav2_velocity_smoother',
                executable='velocity_smoother',
                name='velocity_smoother',
                output='screen',
                parameters=[nav2_params],
                remappings=[
                    ('cmd_vel', '/cmd_vel_raw'),
                    ('cmd_vel_smoothed', '/nav2/cmd_vel'),
                ],
            )],
        ),

        TimerAction(
            period=12.0,
            actions=[Node(
                package='nav2_behaviors',
                executable='behavior_server',
                name='behavior_server',
                output='screen',
                parameters=[nav2_params],
                remappings=[('cmd_vel', '/nav2/cmd_vel')],
            )],
        ),

        Node(
            package='rc_car_teleop',
            executable='goal_pose_relay',
            name='goal_pose_relay',
            output='screen',
        ),

        TimerAction(
            period=15.0,
            actions=[Node(
                package='nav2_bt_navigator',
                executable='bt_navigator',
                name='bt_navigator',
                output='screen',
                parameters=[nav2_params],
                remappings=[('/goal_pose', '/goal_pose_relayed')],
            )],
        ),

        TimerAction(
            period=20.0,
            actions=[Node(
                package='nav2_lifecycle_manager',
                executable='lifecycle_manager',
                name='lifecycle_manager_navigation',
                output='screen',
                parameters=[{
                    'use_sim_time': False,
                    'autostart': True,
                    'node_names': [
                        'map_server',
                        'amcl',
                        'planner_server',
                        'controller_server',
                        'velocity_smoother',
                        'behavior_server',
                        'bt_navigator',
                    ],
                }],
            )],
        ),

    ])
