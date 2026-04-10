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
    maintainer_email='jj@todo.todo',
    description='RC car teleoperation node',
    license='Apache-2.0',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'teleop_node = rc_car_teleop.teleop_node:main',
        ],
    },
)
