import sys, os
sys.path.insert(0, os.path.abspath('../software/ros2_ws/src/rc_car_teleop'))

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.napoleon',   # for Google-style docstrings
    'sphinxcontrib.images'
]

autodoc_mock_imports = ['rclpy',
                        'geometry_msgs',
                        'nav_msgs', 
                        'sensor_msgs',
                        'std_msgs',
                        'tf2_ros',
                        'cv2',
                        'cv_bridge']

html_theme = "shibuya"
project = 'Robotic Tank Project'
author = 'JJ'
release = '2026.05'
html_title = project