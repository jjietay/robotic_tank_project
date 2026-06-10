# Commands

## ROS 2 Humble Activation
- source /opt/ros/humble/setup.bash

## activate specific robotic_tank_project workspace
- source ~/robotic_tank_project/software/ros2_ws/install/setup.bash
- source ~/microros_ws/install/local_setup.bash

## Run micro-ros
- ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyACM0 -v6

## Run specific nodes
- ros2 topic echo /sensors/encoders/right_ticks

## Run slam launch for mapping
ros2 launch tank_bringup slam.launch.py

## Open RVIZ
### (A) MAC
- conda activate ros2
- export AMENT_PREFIX_PATH=/Users/jj/robotic_tank_project/software/ros2_ws/install/tank_description:/opt/homebrew/Caskroom/miniforge/base/envs/ros2
- rviz2 -d ~/.rviz2/new_without_encoders.rviz

### (B) RPI4
- cd ~/robotic_tank_project/software/ros2_ws && source install/setup.bash
- ros2 launch tank_description display.launch.py

## Build Sphinx
- sphinx-autobuild -b html docs docs/_build/html 

## Build Mkdocs
- mkdocs serve --dev-addr localhost:8001


## uf2 file from mac to rpi4
### (A) MAC
- scp /Users/jj/robotic_tank_project/firmware/pico_w/micro_ros_ws/build/pico_micro_ros.uf2 jj@192.168.10.37:~/

### (B) RPI4
- sudo picotool reboot -f -u
- sudo picotool load ~/pico_micro_ros.uf2 -f
- sudo picotool reboot


## TMUX
-tmux ls (find existing windows)
-tmux new -As NAME (create permanent new window)
-Ctrl b % (split left and right panes)
-Ctrl b “ (split up and down panes)
-Ctrl b o (switch between panes)
-Ctrl b x (kill pane)
-Ctrl b : “select-layout tiled” (equally spaced panes)




# =========== TESTING SENSORS BEFORE STARTING =============
# ── 1. Are all sensor topics alive? ──
# Expected: left/right ticks ~100 Hz, IMU ~50 Hz, odom ~30-50 Hz, filtered ~30 Hz, scan ~6-8 Hz
ros2 topic hz /sensors/encoders/left_ticks --window 20
ros2 topic hz /sensors/encoders/right_ticks --window 20
ros2 topic hz /sensors/imu --window 20
ros2 topic hz /odom --window 20
ros2 topic hz /odometry/filtered --window 20
ros2 topic hz /scan --window 20

# ── 2. Is the EKF the SOLE owner of odom → base_link? ──
# Expected: exactly ONE broadcaster, from ekf_filter_node
ros2 run tf2_ros tf2_monitor odom base_link

# ── 3. Does the TF actually exist? ──
# Expected: valid translation + rotation, not "does not exist"
ros2 run tf2_ros tf2_echo odom base_link

# ── 4. Quick IMU sanity check (robot stationary on flat surface) ──
# Expected: orientation should be near identity (w≈1.0, x/y/z≈0),
#           gyro should be near 0, linear_accel should be near 0
ros2 topic echo /sensors/imu --once

# ── 5. EKF diagnostic — is it actually fusing BOTH inputs? ──
# This prints what the EKF is receiving and processing
ros2 topic echo /diagnostics --once


# ================== MORE INFO ABOUT TOPIC, HOW MANY SUBSCRIBERS OR PUBLISHING ================
ros2 topic info /odom --verbose
ros2 topic info /sensors/imu --verbose