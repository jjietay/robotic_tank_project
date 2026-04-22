import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from std_msgs.msg import String
from cv_bridge import CvBridge
from ultralytics import YOLO
import json

class YoloDetectorNode(Node):

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

        self.sub = self.create_subscription(Image, 'camera/image_raw', self.image_callback, 10)

        self.pub_detections = self.publisher_(String, 'yolo/detections', 10)
        self.pub_viz = self.create_publisher(Image, 'yolo/image_annotated', 10)

    def image_callback(self, msg: Image):   # : just means msg should be an Image object
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
                'bbox': box.xyxy[0].tolist()  # [x1, y1, x2, y2]
            })

        self.pub_detections.publish(String(data=json.dumps(detections)))

        annotated = results[0].plot()
        self.pub_viz.publish(self.bridge.cv2_to_imgmsg(annotated, encoding='bgr8'))


def main(args=None):
    rclpy.init(args=args)
    node = YoloDetectorNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()