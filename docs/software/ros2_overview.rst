ROS 2 Overview
==============

This page gives a short overview of the ROS 2 software that runs on the
Raspberry Pi 4. The Raspberry Pi handles sensing, state estimation, mapping,
navigation, and the commands that drive the tank. The Pico W handles the low
level motor and sensor work and talks to the Pi over a serial micro ROS link.

The system is made up of a few groups of nodes.

Sensing
    The Pico W publishes wheel encoder ticks and ultrasonic ranges. The lidar
    driver publishes laser scans on ``/scan``. The BNO085 IMU node publishes
    orientation and motion on ``/sensors/imu``.

State estimation
    The odometry node turns encoder ticks into a pose estimate on ``/odom``.
    The EKF from robot_localization fuses ``/odom`` and ``/sensors/imu`` into a
    smoother estimate and broadcasts the ``odom`` to ``base_link`` transform.

Mapping and navigation
    SLAM Toolbox builds a map from the laser scans. Nav2 plans a path to a goal
    and follows it, sending velocity commands for the tank to follow.

Commands
    More than one source can ask the tank to move. Teleop sends keyboard
    commands and Nav2 sends its own commands. The twist_mux node picks one
    based on priority and forwards it as ``/cmd_vel``, which the Pico W reads.
