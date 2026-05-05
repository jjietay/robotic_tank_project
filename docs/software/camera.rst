Camera
=============================

This node captures frames from a USB/CSI camera using OpenCV and publishes
them as ROS 2 ``sensor_msgs/msg/Image`` messages on ``/camera/image_raw``.

Overview
--------

- Node name: ``camera``
- Topic published: ``/camera/image_raw`` (type: ``sensor_msgs/msg/Image``)
- Parameter:
  
  - ``fps`` (double, default: 30.0) – target frames per second

- Main responsibilities:

  - Configure and open the camera using OpenCV
  - Capture frames at the requested FPS
  - Convert OpenCV images to ROS ``Image`` messages via ``CvBridge``
  - Publish images on ``/camera/image_raw``
  - Log warnings if frames cannot be read

Node Structure
--------------

Imports
~~~~~~~

.. code-block:: python

   import cv2
   import rclpy
   from rclpy.node import Node
   from cv_bridge import CvBridge
   from sensor_msgs.msg import Image

- ``cv2`` is OpenCV, used to talk to the camera hardware and grab frames.
- ``CvBridge`` converts between OpenCV ``numpy`` arrays and ROS ``Image`` messages.
- ``sensor_msgs.msg.Image`` is the standard ROS 2 message type for raw images.

Class Definition
~~~~~~~~~~~~~~~~

.. code-block:: python

   class ImagePublisher(Node):

- The node is implemented as a subclass of ``Node``.
- This class encapsulates all camera configuration, publishing logic,
  and cleanup.

Constructor
~~~~~~~~~~~

.. code-block:: python

   def __init__(self):
       super().__init__("camera")
       self.declare_parameter('fps', 30)
       self.fps = self.get_parameter('fps').get_parameter_value().double_value
       if fps <= 0.0:
           self.get_logger().warn(f'Invalid fps={fps}, defaulting to 30.0')
           fps = 30.0
       timer_period = 1.0/fps

- ``super().__init__("camera")`` registers the node with the name ``camera``.
- ``fps`` is declared as a ROS 2 parameter with default ``30``.
- The parameter is read and used to compute ``timer_period = 1.0 / fps`` so
  that the timer fires at approximately the requested frame rate.
- If an invalid ``fps`` (``<= 0.0``) is provided, the node logs a warning
  and falls back to ``30.0`` FPS.

Camera Configuration
~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   # OpenCV camera config
   self.cap = cv2.VideoCapture(0)
   fourcc = cv2.VideoWriter_fourcc(*'MJPG')
   self.cap.set(cv2.CAP_PROP_FOURCC, fourcc)
   self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
   self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
   self.cap.set(cv2.CAP_PROP_FPS, fps)

- ``cv2.VideoCapture(0)`` opens the first camera device (usually ``/dev/video0``).
- The MJPEG codec is selected via ``VideoWriter_fourcc(*'MJPG')`` to reduce
  USB bandwidth and CPU load.
- Resolution is set to 1280x720 at the requested FPS.

Publisher, Bridge, and Timer
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   # ROS publisher + bridge
   self.bridge = CvBridge()
   self.publisher_ = self.create_publisher(Image, '/camera/image_raw', 10)
   self.timer = self.create_timer(timer_period, self.timer_callback)
   self.i = 0

- ``CvBridge()`` is used to convert OpenCV frames into ROS ``Image`` messages.
- ``create_publisher(Image, '/camera/image_raw', 10)`` creates a publisher
  on ``/camera/image_raw`` with queue size 10.
- ``create_timer(timer_period, self.timer_callback)`` schedules
  ``timer_callback`` to run at the configured FPS.

Timer Callback
--------------

.. code-block:: python

   def timer_callback(self):
       ret, frame = self.cap.read()
       if not ret:
           self.get_logger().warn('Failed to read frame')
           return
       
       # Convert and publish
       msg = self.bridge.cv2_to_imgmsg(frame, encoding='bgr8')
       self.publisher_.publish(msg)
       self.get_logger().info('Publishing frame')

- ``self.cap.read()`` grabs the next frame from the camera.
- If frame capture fails (``ret`` is ``False``), the node logs a warning and
  skips this cycle instead of crashing.
- ``cv2_to_imgmsg(frame, encoding='bgr8')`` converts the OpenCV BGR image into
  a ROS 2 ``Image`` message.
- The message is published on ``/camera/image_raw``, and a log entry is written.

Cleanup
-------

.. code-block:: python

   def Kill(self):
       self.cap.release()

- ``Kill()`` explicitly releases the camera device so other processes can
  use it after the node shuts down.

Main Function
-------------

.. code-block:: python

   def main(args=None):
       rclpy.init(args=args)
       img_publisher = ImagePublisher()
       rclpy.spin(img_publisher)
       img_publisher.destroy_node()
       rclpy.shutdown()
       img_publisher.Kill()


   if __name__ == '__main__':
       main()

- ``rclpy.init()`` initializes the ROS 2 client library.
- ``ImagePublisher()`` creates and configures the node.
- ``rclpy.spin(img_publisher)`` runs the event loop and keeps the node alive
  while the timer callback publishes frames.
- After shutdown, the node is destroyed and the camera resource is released
  by calling ``Kill()``.

Usage Notes
-----------

- Configure FPS at launch time, for example:

  .. code-block:: bash

     ros2 run your_package camera --ros-args -p fps:=15.0

- Subscribe to the image stream with tools like ``rqt_image_view`` or a
  custom node that consumes ``sensor_msgs/msg/Image``.
