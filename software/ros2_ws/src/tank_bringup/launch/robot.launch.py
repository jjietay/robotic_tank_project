import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():

    # --- 1. Get the URDF ---
    pkg_share = get_package_share_directory('tank_description')
    urdf_path = os.path.join(pkg_share, 'urdf', 'tank.urdf')
    with open(urdf_path, 'r') as f:
        robot_description = f.read()

    twist_mux_config = os.path.join(
        get_package_share_directory('tank_bringup'),
        'config',
        'twist_mux.yaml'
    )

    ydlidar_params = os.path.join(
    get_package_share_directory('tank_bringup'),
    'config',
    'ydlidar.yaml'
    )

    return LaunchDescription([

        # --- 2. TF Tree ---
        # robot_state_publisher
        # Hint: same as display.launch.py but add use_sim_time: False
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


        # --- 3. Sensing ---
        # ydlidar driver
        # Hint: check your ydlidar package name and executable name
        Node(
            package = 'ydlidar_ros2_driver',
            executable = 'ydlidar_ros2_driver_node',
            name = 'ydlidar_ros2_driver_node',
            output = 'screen',
            parameters=[ydlidar_params],
        ),


        # --- 4. Thinking ---
        # odometry node
        Node(
            package='rc_car_teleop',
            executable='odometry',
            name='odometry',
            output='screen',
        ),

        # lidar_processor node
        Node(
            package='rc_car_teleop',
            executable='lidar_processor',
            name='lidar_processor',
            output='screen',
        ),


        # brain node
        Node(
            package='rc_car_teleop',
            executable='brain',
            name='brain',
            output='screen',
        ),


        # yolo node
        Node(
            package='rc_car_teleop',
            executable='yolo',
            name='yolo',
            output='screen',
        ),



        # --- 5. Arbitration ---
        # twist_mux
        Node(
            package='twist_mux',
            executable='twist_mux',
            name='twist_mux',
            output='screen',
            parameters=[twist_mux_config],      # pass the yaml path directly
            remappings=[('cmd_vel_out', '/cmd_vel')]   # twist_mux output → pico's topic
        ),


        # --- 6. Teleop ---
        Node(
            package='rc_car_teleop',
            executable='teleop',
            name='teleop',
            output='screen',
        ),


    ])