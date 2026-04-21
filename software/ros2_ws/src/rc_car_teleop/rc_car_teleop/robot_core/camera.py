import numpy
import cv2
import rclpy
from rclpy.node import Node
from cv_bridge import CvBridge
from sensor_msgs.msg import Image

class ImagePublisher(Node):
    def __init__(self):
        super().__init__("camera")

        # OpenCV camera config
        self.cap = cv2.VideoCapture(0)
        fourcc = cv2.VideoWriter_fourcc(*'MJPG')
        self.cap.set(cv2.CAP_PROP_FOURCC, fourcc)
        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
        self.cap.set(cv2.CAP_PROP_FPS, 30)

        # ROS publisher + bridge
        self.bridge = CvBridge()
        self.publisher_ = self.create_publisher(Image, '/camera/image_raw', 10)
        timer_period = 1/30     # 30 fps
        self.timer = self.create_timer(timer_period, self.timer_callback)
        self.i = 0

    def timer_callback(self):
        ret, frame = self.cap.read()
        if not ret:
            self.get_logger().warn('Failed to read frame')
            return
        
        # Convert and publish
        msg = self.bridge.cv2_to_imgmsg(frame, encoding='bgr8')
        self.publisher_.publish(msg)
        self.get_logger().info('Publishing frame')

    def Kill(self):
        self.cap.release()

def main(args=None):
    rclpy.init(args=args)
    img_publisher = ImagePublisher()
    rclpy.spin(img_publisher)
    img_publisher.destroy_node()
    rclpy.shutdown()
    img_publisher.Kill()

if __name__ == '__main__':
    main()

