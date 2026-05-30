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
      ├── camera_link
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
          'odometry        = rc_car_teleop.odom:main',
          'lidar_processor = rc_car_teleop.lidar_processor:main',
          'yolo            = rc_car_teleop.yolo:main',
          'camera          = rc_car_teleop.camera:main',
          'brain           = rc_car_teleop.brain:main',
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

    # 2. Define your second node (e.g., the camera)
    camera_node = Node(
        package='rc_car_teleop',
        executable='camera',
        name='front_camera',
        output='screen',
        remappings=[
            ('/image_raw', '/camera/front/image_raw') # Change topic names on the fly
        ]
    )

    # ==========================================
    # PART 3: THE EXECUTION / RETURN
    # ==========================================
    # ROS 2 expects this function to return a LaunchDescription object
    # containing a list of all the actions/nodes you want to start.
    return LaunchDescription([
        teleop_node,
        camera_node
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

    # 2. Define your second node (e.g., the camera)
    camera_node = Node(
        package='rc_car_teleop',
        executable='camera',
        name='front_camera',
        output='screen',
        remappings=[
            ('/image_raw', '/camera/front/image_raw') # Change topic names on the fly
        ]
    )

exact function name is mandatory, when we run ``ros2 launch``, the ros2 system searches the python script specifically for a function called ``generate_launch_description()``. The ``Node`` block references everything from previous files.

**Part 3: return**

.. code-block:: python

  return LaunchDescription([
          teleop_node,
          camera_node
      ])

bundle all the nodes, scripts, and configs defines into a single list and pass it to ``LaunchDescription([...])``. When this object is returned, ROS2 takes over and starts spinning up the processes.