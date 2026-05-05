YOLO
==================

This node performs object detection on incoming camera images using an
Ultralytics YOLO model. It subscribes to raw images from the camera,
runs inference, publishes detection results as JSON text, and also
publishes an annotated image showing the detected objects. Ultralytics’
ROS examples use the same general pattern: subscribe to an image topic,
run YOLO inference, and publish structured results or annotated images.

Overview
--------

- Node name: ``yolo``
- Subscribed topic:

  - ``/camera/image_raw`` (type: ``sensor_msgs/msg/Image``)

- Published topics:

  - ``/yolo/detections`` (type: ``std_msgs/msg/String``)
  - ``/yolo/image_annotated`` (type: ``sensor_msgs/msg/Image``)

- Parameters:

  - ``model_path`` (string, default: ``yolov8n.pt``)
  - ``conf_threshold`` (double, default: ``0.5``)
  - ``device`` (string, default: ``cpu``)

- Main responsibilities:

  - Load a YOLO model from disk
  - Convert ROS images into OpenCV format
  - Run object detection on each frame
  - Publish detection results as JSON
  - Publish an annotated visualization image

Pipeline
--------

The processing flow is:

1. Receive a frame from ``/camera/image_raw``
2. Convert the ROS ``Image`` message into an OpenCV image using ``CvBridge``
3. Run YOLO inference on the frame
4. Extract classes, confidence scores, and bounding boxes
5. Publish the detections as JSON on ``/yolo/detections``
6. Draw the detections on the frame and publish the annotated image on
   ``/yolo/image_annotated``

This is a standard perception-node pattern in ROS: image in, inference,
then structured outputs for downstream nodes.

Imports
-------

.. code-block:: python

   import rclpy
   from rclpy.node import Node
   from sensor_msgs.msg import Image
   from std_msgs.msg import String
   from cv_bridge import CvBridge
   from ultralytics import YOLO
   import json

- ``sensor_msgs.msg.Image`` is the standard ROS message type for images.
- ``CvBridge`` converts between ROS image messages and OpenCV images.
- ``ultralytics.YOLO`` loads and runs the trained object detection model.
- ``json`` is used to serialize detections before publishing them as text.

Class Definition
----------------

.. code-block:: python

   class YoloDetectorNode(Node):

This class encapsulates model loading, image subscription, inference,
and result publishing.

Constructor
-----------

.. code-block:: python

   def __init__(self):
       super().__init__('yolo')

       self.declare_parameter('model_path', 'yolov8n.pt')
       self.declare_parameter('conf_threshold', 0.5)
       self.declare_parameter('device', 'cpu')

       model_path = self.get_parameter('model_path').get_parameter_value().string_value
       self.conf = self.get_parameter('conf_threshold').get_parameter_value().double_value
       self.device = self.get_parameter('device').get_parameter_value().string_value

       self.bridge = CvBridge()
       self.model = YOLO(model_path)
       self.get_logger().info(f'Loaded model: {model_path} on {self.device}')
       self.sub = self.create_subscription(Image, '/camera/image_raw', self.image_callback, 10)
       self.pub_detections = self.create_publisher(String, '/yolo/detections', 10)
       self.pub_viz = self.create_publisher(Image, '/yolo/image_annotated', 10)

Key setup performed here:

- Registers the node as ``yolo``
- Declares parameters for model path, confidence threshold, and compute device
- Loads the YOLO model from the configured path
- Creates a subscriber for raw camera images
- Creates publishers for detections and annotated images

Parameters
----------

``model_path``
   Path to the YOLO model weights file, for example ``yolov8n.pt``.

``conf_threshold``
   Confidence threshold passed into YOLO inference. Detections below this value
   are filtered by the model.

``device``
   Compute target such as ``cpu`` or a CUDA device string, depending on your
   system and Ultralytics installation.

Image Callback
--------------

.. code-block:: python

   def image_callback(self, msg: Image):
       try:
           frame = self.bridge.imgmsg_to_cv2(msg, desired_encoding='bgr8')
       except Exception as e:
           self.get_logger().error(f'cv_bridge error: {e}')
           return

       results = self.model(frame, conf=self.conf, device=self.device, verbose=False)

       detections = []
       for box in results[0].boxes:
           detections.append({
               'class': self.model.names[int(box.cls)],
               'confidence': float(box.conf),
               'bbox': box.xyxy[0].tolist()
           })

       self.pub_detections.publish(String(data=json.dumps(detections)))

       annotated = results[0].plot()
       self.pub_viz.publish(self.bridge.cv2_to_imgmsg(annotated, encoding='bgr8'))

This callback does all perception work for each incoming frame.

Step 1: Convert ROS image to OpenCV
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   frame = self.bridge.imgmsg_to_cv2(msg, desired_encoding='bgr8')

The ROS ``Image`` message is converted into an OpenCV-compatible BGR image.
That conversion pattern is standard when integrating OpenCV processing into ROS
Python nodes.

Step 2: Run YOLO inference
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   results = self.model(frame, conf=self.conf, device=self.device, verbose=False)

The frame is passed through the YOLO model with the configured confidence
threshold and device selection. Ultralytics’ ROS integration examples use the
same type of direct inference call on incoming image frames
Step 3: Build detection output
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   detections = []
   for box in results[0].boxes:
       detections.append({
           'class': self.model.names[int(box.cls)],
           'confidence': float(box.conf),
           'bbox': box.xyxy[0].tolist()
       })

Each detection contains:

- ``class``: the detected object class name
- ``confidence``: the model confidence score
- ``bbox``: bounding box coordinates in ``[x1, y1, x2, y2]`` format

Step 4: Publish detections
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   self.pub_detections.publish(String(data=json.dumps(detections)))

The list of detections is serialized into JSON and published as a
``std_msgs/msg/String``.

Step 5: Publish annotated image
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   annotated = results[0].plot()
   self.pub_viz.publish(self.bridge.cv2_to_imgmsg(annotated, encoding='bgr8'))

The node renders bounding boxes and labels onto the image and publishes the
annotated result as another ROS ``Image`` message.

This is useful for:

- Visual debugging
- Demo videos
- Confirming that detections line up with objects correctly

Main Function
-------------

.. code-block:: python

   def main(args=None):
       rclpy.init(args=args)
       node = YoloDetectorNode()
       rclpy.spin(node)
       node.destroy_node()
       rclpy.shutdown()

   if __name__ == '__main__':
       main()

- ``rclpy.init()`` initializes the ROS 2 client library
- ``YoloDetectorNode()`` creates the node, loads the model, and sets up topics
- ``rclpy.spin(node)`` keeps the node alive and processes incoming frames.
- ``destroy_node()`` and ``shutdown()`` stop the node cleanly

Example Output Format
---------------------

An example detection message published on ``/yolo/detections`` might look like:

.. code-block:: json

   [
     {
       "class": "person",
       "confidence": 0.87,
       "bbox": [120.4, 55.2, 300.8, 410.6]
     },
     {
       "class": "bottle",
       "confidence": 0.64,
       "bbox": [420.1, 180.7, 470.2, 320.5]
     }
   ]

Practical Notes
---------------

- ``yolov8n.pt`` is a sensible starting model because it is lightweight, but
  smaller models trade accuracy for speed.
- Running on ``cpu`` is simpler on a Raspberry Pi, but inference speed may be
  limited depending on image size and model choice.
- Publishing full JSON strings is fine for experiments, but a proper detection
  message should come later if this node becomes part of a larger pipeline.
- The node currently has no frame-skipping, rate limiting, or back-pressure
  control. If the camera publishes faster than inference can run, performance
  may become unstable.


Possible Improvements
---------------------

- Replace ``std_msgs/msg/String`` with a custom detection message
- Add timing logs for inference latency
- Add frame headers/timestamps to outputs
- Add class filtering so only selected object types are published
- Add image resizing before inference for faster runtime