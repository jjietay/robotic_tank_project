import os
import sys

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
                        'builtin_interfaces',
                        'tf2_ros',
                        'cv2',
                        'cv_bridge',
                        'ultralytics',
                        'serial',
                        'adafruit_bno08x',
                        'adafruit_bno08x_rvc',
                        'adafruit_extended_bus']

autoclass_content = 'both'
autodoc_member_order = 'bysource'

html_theme = "shibuya"
project = 'Robotic Tank Project'
author = 'JJ'
release = '2026.05'
html_title = project
html_theme_options = {
  "accent_color": "pink",
}
templates_path = ["_templates"]
html_static_path = ['_static']

language = 'en'
locale_dirs = []
gettext_compact = True
html_baseurl = 'https://jjietay.github.io/robotic_tank_project/'
