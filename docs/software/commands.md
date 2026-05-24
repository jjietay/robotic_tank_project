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

## Open RVIZ
### (A) MAC
- conda activate ros2
- export AMENT_PREFIX_PATH=/Users/jj/robotic_tank_project/software/ros2_ws/install/tank_description:/opt/homebrew/Caskroom/miniforge/base/envs/ros2

### (B) RPI4
- cd ~/robotic_tank_project/software/ros2_ws && source install/setup.bash
- ros2 launch tank_description display.launch.py

## Build Sphinx
- sphinx-autobuild -b html docs docs/_build/html 