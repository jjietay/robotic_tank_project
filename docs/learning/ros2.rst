ROS 2 Notes
==================

See also: *Reflections*

1. Core Architecture & Concepts
-------------------------------

.. list-table::
    :header-rows: 1
    :widths: 20 80

    * - Term
      - Definition

    * - Nodes
      - A process that performs a specific task. Nodes are child classes inheriting from the ROS parent ``Node`` class. Each node runs in its own isolated runtime environment and can be written in Python, C++, or almost any other supported language.

    * - Packages
      - The basic unit of organization in ROS. It contains code (``package.xml``, ``CMakeLists.txt``, ``setup.py`` source files) needed to run something.``colcon build`` uses ``package.xml`` (similar to a ``requirements.txt``) to build the package.

    * - Messages
      - The strongly typed communication “language” (strings, ints, floats, arrays)
        passed between nodes.

    * - Interfaces
      - The blueprint or format of a message. It defines the programming API for data
        received/sent via topics or services. ROS provides pre-defined interfaces,
        but custom ones can be created.

    * - Topics
      - Named, asynchronous communication channels using a Publish–Subscribe model.
        A node publishes a message to a topic, and all subscribing nodes receive it.

    * - Services
      - Synchronous communication channels using a Request–Response model. A client
        node sends a request to a server node, which executes a task and returns
        a response.

    * - Callbacks
      - Functions that ROS will execute when an event occurs (e.g., message
        received, timer fired). This keeps ROS 2 code modular.

    * - Timers
      - Mechanisms integrated with the ROS executor that run a specific callback
        function at a defined interval (e.g., “run every 10 ms”).

    * - Parameters
      - Named configuration settings stored within a node (int, float, bool, string,
        list). They allow behavior changes without altering code and can be set via
        command line, YAML, or launch files.

    * - Logging
      - Structured node status output (DEBUG, INFO, WARN, ERROR, FATAL) that replaces
        raw ``print()`` calls. Logs can be routed to the terminal, files, or
        ``/rosout`` for centralized filtering and monitoring.

    * - twist_mux
      - A node that subscribes to multiple velocity topics and outputs values based
        on configured priority (typically handled via a YAML file).

2. Coordinate Systems & TF2 (Transformations)
---------------------------------------------

* **Link:** A named, physical fixed body part of your robot (e.g., chassis, wheel, sensor mount).
* **Frame:** A local coordinate system attached to a link, complete with its own origin (0,0,0) and directional axes (x,y,z). Note that abstract frames like ``odom`` and ``map`` do not have physical hardware attached.
* **URDF (Unified Robot Description Format):** An XML file that defines the fixed physical offsets between your robot's frames.

The TF Tree
~~~~~~~~~~~

- TF2 bridges coordinate systems together
- Tt is literally like a tree that connects different branches (links) together
- It chains transforms to translate measurements from one frame to another

.. code-block:: text

  odom (fixed to world at startup)
  └── base_link (moving with the robot)
      ├── lidar_link
      ├── wheel_left_link
      └── wheel_right_link

Key TF2 Components
~~~~~~~~~~~~~~~~~~

* **odom (Node & Frame):** The base reference frame is anchored at the robot's starting position. The ``odom`` node acts as a generic TF2 broadcaster, publishing the transform (position x, y and rotation theta) between ``base_link`` (current position) and ``odom`` (start) to the ``/tf`` topic.
* **base_link:** The child frame moving with the robot's body. Physical sensors (``lidar_link``) are children of ``base_link``. Therefore, lidar_link is the grandchild of base_link.
* **TF2 Library:** Contains broadcasters, a buffer, and listeners. The buffer subscribes to both ``/tf`` (dynamic movement) and ``/tf_static`` (fixed physical offsets).
* **robot_state_publisher**: It does publish the fixed, non-moving offsets (like base_link to lidar_link) to /tf_static once at startup. It then constantly listens to the angles of the moving joints (wheels), uses the URDF to calculate complex 3D math (Forward Kinematics), and continuously publishes the live coordinates of the moving parts to /tf.
* **joint_state_publisher**: It publishes those "The left wheel is currently rotated at 45 degrees" messages continuously to a specific topic called /joint_states. On a real robot, the physical motor encoders do this job. When simulating or testing, this node fakes the encoder data.
* **RViz2**: RViz2 reads the tank.urdf file first so it knows what the physical 3D .stl meshes look like. Then, it listens to the math coming from both /tf (for moving parts) and /tf_static (for fixed parts) to know exactly where in the virtual world to draw those meshes, so that we can visualize it in real-time.

3. Understanding Packages
--------------------------

Understanding packages and its contents.

* ``colcon build``: colcon is like the main universal way of building packages regardless of the type of source files (python or C++)
* ``ament``: ``ament_python`` or ``ament_cmake`` are build types (specific ways we can build), colcon sees this and understands which ways we want to build our package
* ``ament_python``: always contains python source files and a config.py files, sometimes containing a setup.cfg
* ``ament_cmake``: usually contains C++ source files, but also used for "resource-only" packages that hold models, maps, custom messages, or configuration files



.. code-block:: xml

  <!-- Found in the packages.xml file -->

  <!-- For C++ -->
  <export>
    <build_type>ament_cmake</build_type>
  </export>

  <!-- For python -->
  <export>
  <build_type>ament_python</build_type>
  </export>


1) ``package.xml``

- just a ID card and dependency checklist
- ``<buildtool_depend>`` tells ROS 2 what software is needed to actually compile the package, python (an interpreted language) packages doesn't need this because it doesn't get compiled unlike C++, it simply gets copied over to /install therefore we dont need this, C++ packages explicitly requires this to be stated in packages.xml cos it needs to be compiled into machines binary code
- ``<exec_depend>`` is called *Execution Dependencies* which is what our launch files depend on when we are running it
- ``<test_depend>`` are the dependencies we require to test our code, such as ``pytest``, ``flake8``, and ``pep257`` which are all standard Python code linters (tools that check your code for formatting errors)
- ``<build_type>ament_python</build_type>`` is the tag that ``colcon`` requires to know to look for ``CMakeLists.txt`` or ``config.py`` on how to handle to installation (into /install)


2) ``CMakeLists.txt``

- ``install(DIRECTORY ...)`` This is essentially a "copy-paste" command for the build process. When you run colcon build, ROS 2 doesn't use the files directly from your source code folder. It needs to put them in a central "install space" so the rest of the robot system can find them.
- ``DESTINATION share/${PROJECT_NAME}/)`` This is where those folders get pasted. In ROS 2, description files, launch files, and models are standardly placed in the share/ directory of the installed package.
- ``project(...)`` This gives your package a name. It also creates a handy background variable called ${PROJECT_NAME}


3) ``setup.py``

- ``packages=find_packages(exclude=['test'])`` find packages is a python tool that automatically searches package directory and grabs all the python code so they can be installed
- ``exclude=['test']`` intentionally ignores test/ folders so that testing scripts don't get installed onto robot
- ``data_files=[('share/ament_index/resource_index/packages', ['resource/' + package_name]), ('share/' + package_name, ['package.xml']),],`` first tuple puts a blank marker file deep inside ROS2 system. when we type ``ros2 pkg list``, ROS2 seaches this exact folder to know the existing packages, second tuple copies package.xml file into share/rc_car_teleop installation folder so ROS2 can read the dependencies at runtime
- ``install_requires=['setuptools'], tests_require=['pytest'],`` tells the system it needs ``setuptools`` library to build the package, and the ``pytest`` package to run tests

.. code-block:: python

  entry_points={
      'console_scripts': [
          'teleop          = rc_car_teleop.teleop:main',
          'odometry        = rc_car_teleop.robot_core.odom:main',
          'lidar_processor = rc_car_teleop.robot_core.lidar_processor:main'
      ],
  },

- teleop: This is the name of the executable to create. we use it when we run the command: ``ros2 run rc_car_teleop teleop``.
- rc_car_teleop.teleop:main: This is the path to the code. It tells ROS 2, "Look inside the rc_car_teleop folder, find the teleop.py script, and execute the function called main()."

4) ``launch.py``

- split into 3 parts: imports, generator function, execution/return
- allows us to ros2 launch my_package my_launch.py

**Full Code Block**

.. code-block:: python

  # ==========================================
  # PART 1: THE IMPORTS
  # ==========================================
  from launch import LaunchDescription
  from launch_ros.actions import Node

  # ==========================================
  # PART 2: THE GENERATOR FUNCTION
  # ==========================================
  def generate_launch_description():
    
    # 1. Define your first node (e.g., the joystick teleop)
    teleop_node = Node(
        package='rc_car_teleop',     # The package name (from package.xml)
        executable='teleop',         # The executable name (from setup.py entry_points)
        name='joystick_teleop',      # What to call this specific instance of the node
        output='screen',             # Print the node's output to this terminal
        parameters=[
            {'max_speed': 2.5}       # Inject parameters directly!
        ]
    )

    # 2. Define your second node (e.g., the lidar processor)
    lidar_node = Node(
        package='rc_car_teleop',
        executable='lidar_processor',
        name='lidar_processor',
        output='screen',
        remappings=[
            ('/scan', '/scan_filtered')  # Change topic names on the fly
        ]
    )

    # ==========================================
    # PART 3: THE EXECUTION / RETURN
    # ==========================================
    # ROS 2 expects this function to return a LaunchDescription object
    # containing a list of all the actions/nodes you want to start.
    return LaunchDescription([
        teleop_node,
        lidar_node
    ])


**Part 1: imports**

.. code-block:: python

  from launch import LaunchDescription
  from launch_ros.actions import Node

Always need to import these.


**Part 2: generator function**

.. code-block:: python

  def generate_launch_description():
    
    # 1. Define your first node (e.g., the joystick teleop)
    teleop_node = Node(
        package='rc_car_teleop',     # The package name (from package.xml)
        executable='teleop',         # The executable name (from setup.py entry_points)
        name='joystick_teleop',      # What to call this specific instance of the node
        output='screen',             # Print the node's output to this terminal
        parameters=[
            {'max_speed': 2.5}       # Inject parameters directly!
        ]
    )

    # 2. Define your second node (e.g., the lidar processor)
    lidar_node = Node(
        package='rc_car_teleop',
        executable='lidar_processor',
        name='lidar_processor',
        output='screen',
        remappings=[
            ('/scan', '/scan_filtered') # Change topic names on the fly
        ]
    )

exact function name is mandatory, when we run ``ros2 launch``, the ros2 system searches the python script specifically for a function called ``generate_launch_description()``. The ``Node`` block references everything from previous files.

**Part 3: return**

.. code-block:: python

  return LaunchDescription([
          teleop_node,
          lidar_node
      ])

bundle all the nodes, scripts, and configs defines into a single list and pass it to ``LaunchDescription([...])``. When this object is returned, ROS2 takes over and starts spinning up the processes.

4. SLAM and Mapping
-------------------

SLAM is Simultaneous Localization and Mapping. The robot builds a map of
an unknown space while working out where it is inside that map at the same time.
On the tank I use the ``slam_toolbox`` package in synchronous mode
(``sync_slam_toolbox_node``).

What slam_toolbox does
~~~~~~~~~~~~~~~~~~~~~~~

* **Input**: the laser scans on ``/scan`` and the robot's rough pose from the ``odom`` to ``base_link`` transform
* **Scan matching**: it lines each new scan up against the map so far to correct the pose
* **Output**: an occupancy grid (the map) on ``/map``, plus the ``map`` to ``odom`` transform

The map, odom and base_link frames
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

While mapping, the transform chain looks like this, with the node that owns each link:

.. code-block:: text

  map        <- slam_toolbox          (scan matching correction)
   |
  odom       <- EKF                   (encoders + IMU fused)
   |
  base_link  <- EKF
   |
  lidar_link, imu_link, ...           <- robot_state_publisher (from the URDF)

* **map to odom**: the slow correction from scan matching, which cancels long term drift
* **odom to base_link**: the fast, smooth motion estimate from the EKF
* Only one node should publish a given transform. Both my odom node and the EKF were publishing ``odom`` to ``base_link``, so I let the EKF own it and turned off the odom node's broadcast

The EKF (robot_localization)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The EKF fuses ``/odom`` (encoders) and ``/sensors/imu`` (BNO085) into a smoother
``/odometry/filtered`` and publishes the ``odom`` to ``base_link`` transform. Key
choices in ``ekf.yaml``:

* **Encoders for linear velocity only**: the plastic tracks slip when turning, so the encoder yaw is not trustworthy, and I stop fusing the encoder angular velocity
* **IMU for all rotation**: the BNO085 rotation vector is already fused with the magnetometer on the chip, so I trust it for yaw and set ``imu0_differential: false`` to use its absolute heading
* **Heading zeroing**: the BNO085 boots facing a random direction, so ``bno085_i2c_node`` saves the first yaw and subtracts it from every later reading, which makes yaw always start at zero

Talking to the laptop (DDS)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The Pi runs the robot and the laptop runs the viewer, so the two have to talk over WiFi.

* **Fast DDS**: the ROS 2 Humble default, which kept dropping topics between machines over WiFi
* **Cyclone DDS**: more reliable for a while, then the same trouble came back
* **Foxglove Bridge**: the fix I settled on, where the Pi serves every topic over a websocket and Foxglove Studio connects to it from the laptop

5. Nav2 and Navigation
----------------------

Nav2 is the ROS 2 navigation stack. Give it a map and a goal pose, and it plans a
path and drives the robot there while avoiding obstacles.

The Nav2 nodes
~~~~~~~~~~~~~~

* **map_server**: loads the saved map
* **amcl**: localizes the robot on the map with a particle filter
* **planner_server**: plans the global path to the goal
* **controller_server**: follows the path and puts out the velocity command
* **velocity_smoother**: smooths the controller output before it goes out
* **behavior_server**: runs the recovery moves like spin and back up
* **bt_navigator**: the behaviour tree that ties planning, following and recovery together
* **lifecycle_manager**: brings all the Nav2 nodes up in order, which the Pi needs extra time for

How a goal flows
~~~~~~~~~~~~~~~~

.. code-block:: text

  goal in Foxglove
    -> /goal_pose
    -> goal_pose_relay        (zeroes the timestamp)
    -> /goal_pose_relayed
    -> bt_navigator -> planner_server + controller_server
    -> /cmd_vel_raw -> velocity_smoother
    -> /nav2/cmd_vel
    -> twist_mux -> /cmd_vel
    -> the Pico

* **goal_pose_relay**: Nav2 kept transforming the goal at its original timestamp, which fell out of the TF buffer during slow recoveries and broke replanning, so this node zeroes the stamp and the planner then uses the latest transform
* **twist_mux**: picks one velocity source by priority, where teleop (100) beats Nav2 (50) so I can always take over

Controllers: RPP vs DWB
~~~~~~~~~~~~~~~~~~~~~~~~

* **DWB (Dynamic Window Approach)**: samples many possible trajectories each cycle and scores them with critics. Powerful but heavy, and too expensive for the Pi 4. Near the goal its ``RotateToGoal`` and ``GoalAlign`` critics pushed each cycle past its deadline, so the robot stalled and dropped into recovery
* **RPP (Regulated Pure Pursuit)**: no trajectory sampling, it just follows the planned path. Light enough to run smoothly with the planner held at 2 Hz, so this is the one I use
* **use_regulated_linear_velocity_scaling**: slows the robot near tight spots, which is why it eases past the pillars
* **progress checker timeout**: the robot has to fail to make progress for a set time before recovery kicks in, so it does not give up too early

Obstacle avoidance
~~~~~~~~~~~~~~~~~~

* **Far range**: the obstacle is placed early, so there is time to plan a fresh route around it
* **Close range**: the obstacle is dropped right in front, so the robot has to react at the last second
* On the Pi 4, RPP with the NavFn planner handled both cases far better than DWB, drawing a new path almost instantly when the path was blocked
