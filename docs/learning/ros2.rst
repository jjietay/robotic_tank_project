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
     - A process that performs a specific task. Nodes are implemented as subclasses
       inheriting from the ROS ``Node`` class (requiring OOP encapsulation). Each
       runs in its own isolated runtime environment and can be written in Python,
       C++, or almost any other supported language.

   * - Packages
     - The basic unit of organization in ROS. It contains the minimum amount of code
       (``package.xml``, ``CMakeLists.txt``, source files) needed to run something.
       ``colcon build`` uses ``package.xml`` (similar to a ``requirements.txt``)
       to build the package.

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
     - Functions that ROS promises to execute when an event occurs (e.g., message
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

Understanding spatial awareness is critical for robot navigation.

* **Link:** A named, physical rigid body part of your robot (e.g., chassis, wheel, sensor mount).
* **Frame:** A local coordinate system attached to a link, complete with its own origin (0,0,0) and directional axes (x,y,z). Note that abstract frames like ``odom`` and ``map`` do not have physical hardware attached.
* **URDF (Unified Robot Description Format):** An XML file that defines the fixed physical offsets between your robot's frames.

The TF Tree
~~~~~~~~~~~

TF2 bridges coordinate systems together, chaining transforms to translate measurements from one frame to another.

.. code-block:: text

   odom (fixed to world at startup)
   └── base_link (moving with the robot)
       ├── lidar_link
       ├── camera_link
       ├── wheel_left_link
       └── wheel_right_link

Key TF2 Components
~~~~~~~~~~~~~~~~~~

* **odom (Node & Frame):** The base reference frame anchored at the robot's starting position. The ``odom`` node acts as a generic TF2 broadcaster, publishing the transform (position x, y and rotation theta) between ``base_link`` (current position) and ``odom`` (start) to the ``/tf`` topic.
* **base_link:** The child frame moving with the robot's body. Physical sensors (``lidar_link``) are children of ``base_link``.
* **TF2 Library:** Contains broadcasters, a buffer, and listeners. The buffer subscribes to both ``/tf`` (dynamic movement) and ``/tf_static`` (fixed physical offsets).
* **robot_state_publisher:** Reads the URDF and publishes fixed physical offsets (like ``base_link`` to ``lidar_link``) to ``/tf_static`` once at startup. This optimizes bandwidth by keeping fixed data off the high-frequency ``/tf`` topic.

3. Standard Interfaces Reference
--------------------------------

Message interfaces follow the format: ``PackageName/subfolder_type/specific_msg``.

Primitives (``std_msgs``)
~~~~~~~~~~~~~~~~~~~~~~~~~

*Use for simple prototyping; avoid using as fields in higher-level messages.*

======================================== ==================================================
Message                                  Use Case
======================================== ==================================================
``std_msgs/msg/Bool``                    Binary flags, on/off states
``std_msgs/msg/Int32``, ``Int64``        Integer sensor readings, counters
``std_msgs/msg/Float32``, ``Float64``    Raw floating-point values (e.g., voltage)
``std_msgs/msg/String``                  Debug text, status strings
``std_msgs/msg/Header``                  Timestamp + frame ID; embedded in most messages
``std_msgs/msg/Empty``                   Trigger signals with no payload
======================================== ==================================================

Motion & Spatial (``geometry_msgs``)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

*The most commonly used package in mobile robotics.*

========================================== ==================================================
Message                                    Use Case
========================================== ==================================================
``geometry_msgs/msg/Twist``                Velocity commands (linear + angular); ``/cmd_vel``
``geometry_msgs/msg/TwistStamped``         ``Twist`` with a timestamp header
``geometry_msgs/msg/Pose``                 Position (x,y,z) + orientation (quaternion)
``geometry_msgs/msg/PoseStamped``          ``Pose`` with timestamp + frame; Nav2 goals
``geometry_msgs/msg/PoseWithCovariance``   Pose with uncertainty matrix; used in localization
``geometry_msgs/msg/Point``                3D point in space
``geometry_msgs/msg/Vector3``              3D vector; used inside ``Twist``, ``Accel``, etc.
``geometry_msgs/msg/Quaternion``           Rotation in quaternion form
``geometry_msgs/msg/Transform``            Translation + rotation between two frames
``geometry_msgs/msg/TransformStamped``     ``Transform`` with header; used in TF2
========================================== ==================================================

Sensors & Health (``sensor_msgs`` & ``diagnostic_msgs``)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

========================================== ===================================================
Message                                    Use Case
========================================== ===================================================
``sensor_msgs/msg/LaserScan``              2D LIDAR scans
``sensor_msgs/msg/Image``                  Raw camera images
``sensor_msgs/msg/CompressedImage``        JPEG/PNG compressed images; reduces bandwidth
``sensor_msgs/msg/CameraInfo``             Camera calibration parameters
``sensor_msgs/msg/PointCloud2``            3D LIDAR or depth camera point clouds
``sensor_msgs/msg/Imu``                    Accelerometer + gyroscope data
``sensor_msgs/msg/Range``                  Single ultrasonic/IR range reading
``sensor_msgs/msg/BatteryState``           Battery voltage, current, percentage
``sensor_msgs/msg/JointState``             Joint positions/velocities/efforts; for encoders
``sensor_msgs/msg/NavSatFix``              GPS coordinates
``diagnostic_msgs/msg/DiagnosticStatus``   Health report for a single component (OK/WARN/ERR)
========================================== ===================================================

Navigation (``nav_msgs``)
~~~~~~~~~~~~~~~~~~~~~~~~~

================================ ==============================================
Message/Service                  Use Case
================================ ==============================================
``nav_msgs/msg/Odometry``        Robot pose + velocity from encoders/IMU
``nav_msgs/msg/OccupancyGrid``   2D map output from SLAM
``nav_msgs/msg/Path``            Sequence of poses; planned path from Nav2
``nav_msgs/srv/GetMap``          Service to request the current map
================================ ==============================================

Common Services & System (``std_srvs`` & ``rcl_interfaces``)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

======================================= =================================================
Service/Message                         Use Case
======================================= =================================================
``std_srvs/srv/Empty``                  Trigger with no data (e.g., reset, clear costmap)
``std_srvs/srv/SetBool``                Turn something on/off
``std_srvs/srv/Trigger``                Fire an action, get back a success flag + message
``rcl_interfaces/msg/ParameterEvent``   Broadcasts parameter changes across graph
``rcl_interfaces/srv/GetParameters``    Service to read a node's parameters
======================================= =================================================

4. Python Node Templates
------------------------

A. Publisher Node (``cmd_vel_publisher_node.py``)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This template publishes a ``Twist`` message (containing linear and angular ``Vector3`` data) to ``/cmd_vel`` every 0.5 seconds.

.. code-block:: python

   import rclpy
   from rclpy.node import Node
   from geometry_msgs.msg import Twist

   class CmdVelPublisher(Node):
       def __init__(self):
           # 1. Register node with ROS graph
           super().__init__('cmd_vel_publisher')
           
           # 2. Create publisher: Message Type, Topic Name, Queue Size
           self.publisher_ = self.create_publisher(Twist, '/cmd_vel', 10)
           
           # 3. Create timer callback
           timer_period = 0.5  
           self.timer = self.create_timer(timer_period, self.timer_callback)
           self.i = 0

       def timer_callback(self):
           msg = Twist()
           msg.linear.x = 0.1   # Forward speed
           msg.angular.z = 0.0  # Yaw rotation (positive = counterclockwise)
           
           self.publisher_.publish(msg)
           self.get_logger().info(f"Publishing: linear.x={msg.linear.x}, angular.z={msg.angular.z}")
           self.i += 1

   def main(args=None):
       rclpy.init(args=args)                      # Init ROS 2 client library
       cmd_vel_publisher = CmdVelPublisher()      # Instantiate node
       rclpy.spin(cmd_vel_publisher)              # Enter event loop (blocks until Ctrl-C)
       cmd_vel_publisher.destroy_node()           # Clean up node
       rclpy.shutdown()                           # Shut down middleware


B. Subscriber Node (``cmd_vel_subscriber_node.py``)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A subscriber must use the exact same message type (``Twist``) and topic name (``/cmd_vel``) as the publisher to successfully communicate.

.. code-block:: python

   import rclpy
   from rclpy.node import Node
   from geometry_msgs.msg import Twist

   class CmdVelSubscriber(Node):
       def __init__(self):
           super().__init__('cmd_vel_subscriber')
           
           # 1. Create subscription: Type, Topic, Callback Function, Queue Size
           self.subscription = self.create_subscription(
               Twist,
               '/cmd_vel',
               self.cmd_vel_callback,
               10
           )
           self.subscription  # Prevent unused variable warning

       def cmd_vel_callback(self, msg: Twist):
           # 2. Process incoming message data
           linear_x = msg.linear.x
           angular_z = msg.angular.z
           self.get_logger().info(f"Received cmd_vel: linear.x={linear_x}, angular.z={angular_z}")

   def main(args=None):
       rclpy.init(args=args)
       cmd_vel_subscriber = CmdVelSubscriber()
       rclpy.spin(cmd_vel_subscriber)
       cmd_vel_subscriber.destroy_node()
       rclpy.shutdown()

   if __name__ == '__main__':
       main()


C. Hardware Node: Camera Image Publisher
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This node integrates OpenCV hardware capture with the ROS ecosystem using ``CvBridge``. Notice the explicit ``Kill()`` method used to release the hardware *after* ROS shuts down.

.. code-block:: python

   import numpy
   import cv2
   import rclpy
   from rclpy.node import Node
   from cv_bridge import CvBridge
   from sensor_msgs.msg import Image

   class ImagePublisher(Node):
       def __init__(self):
           super().__init__("image_publisher")

           # 1. OpenCV Camera Configuration
           self.cap = cv2.VideoCapture(0) # Open /dev/video0
           fourcc = cv2.VideoWriter_fourcc(*'MJPG') # Reduces USB bandwidth
           self.cap.set(cv2.CAP_PROP_FOURCC, fourcc)
           self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
           self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
           self.cap.set(cv2.CAP_PROP_FPS, 30)

           # 2. ROS Publisher & Bridge Initialization
           self.bridge = CvBridge() # Instantiated once for efficiency 
           self.publisher_ = self.create_publisher(Image, 'camera/image_raw', 10)
           
           # 3. Target 33ms between callbacks for 30 FPS
           self.timer = self.create_timer(1/30, self.timer_callback)

       def timer_callback(self):
           ret, frame = self.cap.read() # frame shape: (720, 1280, 3)
           
           if not ret: # Guard clause prevents crashes if camera disconnects
               self.get_logger().warn('Failed to read frame')
               return

           # Convert numpy array to ROS Image message (bgr8 is OpenCV default)
           msg = self.bridge.cv2_to_imgmsg(frame, encoding='bgr8')
           self.publisher_.publish(msg)
           self.get_logger().info('Publishing frame')

       def Kill(self):
           # Safely free /dev/video0 for other processes
           self.cap.release() 

   def main(args=None):
       rclpy.init(args=args)
       img_publisher = ImagePublisher()
       
       rclpy.spin(img_publisher)
       img_publisher.destroy_node()
       rclpy.shutdown()
       
       # CRITICAL: Call custom kill method after ROS shutdown
       img_publisher.Kill() 

   if __name__ == '__main__':
       main()