## activate the main ROS 2 Humble installation
source /opt/ros/humble/setup.bash

## activate specific robotic_tank_project workspace
source ~/robotic_tank_project/software/ros2_ws/install/setup.bash

source ~/microros_ws/install/local_setup.bash

ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyACM0 -v6

ros2 topic echo /sensors/encoders/right_ticks
