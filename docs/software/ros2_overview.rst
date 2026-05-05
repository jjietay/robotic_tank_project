ROS 2 Overview
==============

This section gives a high-level overview of the ROS 2 software
architecture used in the tank project.

High-Level Architecture
-----------------------

At a high level, the tank’s ROS 2 system is organised into:

- **Perception nodes** for reading sensors (LIDAR, ultrasonics, encoders,
  camera) and publishing standard ROS 2 messages.
- **State-estimation and TF nodes** for computing odometry, managing frames
  (``map``, ``odom``, ``base_link``), and keeping the robot’s pose consistent.
- **Control and decision nodes** (the “brain”) that take sensor data and
  goals, then publish velocity commands on ``/cmd_vel``.
- **Utility nodes** such as logging, parameter management, and tools like
  ``twist_mux`` that arbitrate between multiple command sources.