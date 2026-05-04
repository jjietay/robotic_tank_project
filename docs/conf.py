import sys, os
sys.path.insert(0, os.path.abspath(''))

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.napoleon',   # for Google-style docstrings
    
]
autodoc_mock_imports = ['rclpy', 'geometry_msgs', 'nav_msgs', 'sensor_msgs', 'std_msgs', 'tf2_ros']
html_theme = "shibuya"