from setuptools import find_packages, setup

package_name = 'rc_car_teleop'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='jj',
    maintainer_email='tayjuenjie.tjj@gmail.com',
    description='RC car teleoperation node',
    license='Apache-2.0',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'teleop          = rc_car_teleop.teleop:main',
            'odometry        = rc_car_teleop.robot_core.odom:main',
            'bno085_i2c      = rc_car_teleop.robot_core.bno085_i2c_node:main',
            'lidar_processor = rc_car_teleop.robot_core.lidar_processor:main',
            'brain           = rc_car_teleop.robot_core.brain:main',
            'goal_pose_relay = rc_car_teleop.goal_pose_relay:main',
        ],
    },
)
