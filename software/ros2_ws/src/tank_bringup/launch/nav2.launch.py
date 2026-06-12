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

        # ── Base robot stack ────────────────────────────────────────────

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
            output='log',              # suppress "Real points > fixed points" warnings from terminal
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

        # ── Teleop — run separately in its own terminal (needs TTY) ──
        # ros2 run rc_car_teleop teleop

        # ── Twist Mux — arbitrates teleop (P100) > nav2 (P50) ──────

        Node(
            package='twist_mux',
            executable='twist_mux',
            name='twist_mux',
            output='screen',
            parameters=[twist_mux_config],
            remappings=[('cmd_vel_out', '/cmd_vel')],
        ),

        # ── EKF — fuses /odom + /sensors/imu → /odometry/filtered ──

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

        # ── Nav2 stack ──────────────────────────────────────────────

        # Map server — loads your saved map
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

        # AMCL — particle-filter localization on the saved map
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

        # Planner server — global path planning (A*)
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

        # Controller server — Regulated Pure Pursuit
        # Remapped: output goes to /nav2/cmd_vel → twist_mux → /cmd_vel
        TimerAction(
            period=12.0,
            actions=[Node(
                package='nav2_controller',
                executable='controller_server',
                name='controller_server',
                output='screen',
                parameters=[nav2_params],
                remappings=[('cmd_vel', '/nav2/cmd_vel')],
            )],
        ),

        # Behavior server — recovery behaviors (spin, back up, wait)
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

        # Goal pose relay — zeros timestamp to fix TF extrapolation error
        # Foxglove → /goal_pose → relay → /goal_pose_relayed → bt_navigator
        # See: https://github.com/ros-planning/navigation2/issues/3075
        Node(
            package='rc_car_teleop',
            executable='goal_pose_relay',
            name='goal_pose_relay',
            output='screen',
        ),

        # BT navigator — coordinates planning + control + recovery
        # Remapped: reads goals from /goal_pose_relayed (with zeroed timestamps)
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

        # Lifecycle manager — brings all Nav2 nodes through their lifecycle
        # Must start AFTER all Nav2 nodes are up — RPi4 needs extra time
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
                        'behavior_server',
                        'bt_navigator',
                    ],
                }],
            )],
        ),

    ])
