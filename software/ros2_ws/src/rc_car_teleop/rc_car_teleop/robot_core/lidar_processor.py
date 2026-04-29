import rclpy
from rclpy.node import Node
from sensor_msgs.msg import LaserScan
from std_msgs.msg import Float32

class LidarProcessorNode(Node):

    def __init__(self):
        super().__init__("lidar_processor")
        self.sub = self.create_subscription(LaserScan, '/scan', self.scan_callback, 10)
        self.get_logger().info("LidarProcessor node started, waiting for /scan...")
        self.pub = self.create_publisher(Float32, '/sensors/lidar/closest_point', 10)

    def scan_callback(self, msg: LaserScan):
        # msg.ranges is a list of distances in metres, one per angle step
        # Invalid readings come back as float('inf') or 0.0 — filter them out
        valid_ranges = []
        for r in msg.ranges:
            if msg.range_min < r < msg.range_max:
                valid_ranges.append(r)

        if valid_ranges:    # list is not empty
            closest = min(valid_ranges)
            self.get_logger().info(f"Closest object: {closest:.2f}m")   # log the closest object
            msg_out = Float32()
            msg_out.data = closest
            self.pub.publish(msg_out)

def main(args=None):
    rclpy.init(args=args)
    node = LidarProcessorNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == "__main__":
    main()