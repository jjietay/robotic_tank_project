## activate the main ROS 2 Humble installation
source /opt/ros/humble/setup.bash

## activate specific robotic_tank_project workspace
source ~/robotic_tank_project/software/ros2_ws/install/setup.bash

source ~/microros_ws/install/local_setup.bash

ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyACM0 -v6

ros2 topic echo /sensors/encoders/right_ticks


17s - 0 to 1180044 (front)
14.79s - 118044 to 11104 (back)
13.43s - 11104 to 105034 (front)
21.39s - 88182 to -65088 (CW - 3*360 spins)
22s - -65088 to 81647 (ACW - 3*360 spins)