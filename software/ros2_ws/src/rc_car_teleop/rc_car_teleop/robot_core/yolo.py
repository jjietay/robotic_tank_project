from ultralytics import YOLO
from cv_bridge import CvBridge
from sensor_msgs.msg import Image

# load YOLO in __init__
# subscribe to /camera/image_raw
# in callback: imgmsg_to_cv2 -> model(frame) -> plot() -> cv2_to_imgmsg -> publish

