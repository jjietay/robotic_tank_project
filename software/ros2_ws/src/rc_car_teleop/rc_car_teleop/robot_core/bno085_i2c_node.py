#!/usr/bin/env python3
# ---------------------------------------------------------------------------
#        bno085_i2c_node.py  —  BNO085 I2C → sensor_msgs/Imu
# ---------------------------------------------------------------------------
#  Reads quaternion, gyroscope, and linear acceleration from the BNO085
#  over I2C (software bit-banged bus on /dev/i2c-4) and publishes
#  sensor_msgs/Imu on /sensors/imu.
#
#  Unlike UART-RVC mode, I2C gives us calibrated gyroscope data, which
#  is critical for EKF sensor fusion.
# ---------------------------------------------------------------------------

import time

from adafruit_extended_bus import ExtendedI2C as I2C
from adafruit_bno08x.i2c import BNO08X_I2C
from adafruit_bno08x import (
    BNO_REPORT_ROTATION_VECTOR,
    BNO_REPORT_GYROSCOPE,
    BNO_REPORT_LINEAR_ACCELERATION,
)

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Imu


class BNO085I2CNode(Node):
    def __init__(self):
        super().__init__('bno085_i2c')

        self.declare_parameter('i2c_bus', 4)
        self.declare_parameter('frame_id', 'imu_link')
        self.declare_parameter('publish_rate', 50.0)

        bus_num  = self.get_parameter('i2c_bus').value
        self.frame_id = self.get_parameter('frame_id').value
        rate     = self.get_parameter('publish_rate').value

        self.get_logger().info(f'Opening I2C bus {bus_num}...')
        self.imu = None
        for attempt in range(5):
            try:
                i2c = I2C(bus_num)
                self.imu = BNO08X_I2C(i2c)
                break
            except Exception as e:
                self.get_logger().warn(
                    f'Init failed (attempt {attempt + 1}/5): {e}'
                )
                time.sleep(1.0)
        if self.imu is None:
            raise RuntimeError(f'Could not init BNO085 on i2c-{bus_num} after 5 attempts')

        self.imu.enable_feature(BNO_REPORT_ROTATION_VECTOR)
        self.imu.enable_feature(BNO_REPORT_GYROSCOPE)
        self.imu.enable_feature(BNO_REPORT_LINEAR_ACCELERATION)
        self.get_logger().info('BNO085 I2C connected — 3 reports enabled.')

        self.imu_pub = self.create_publisher(Imu, '/sensors/imu', 10)
        self._consec_errors = 0

        period = 1.0 / rate
        self.timer = self.create_timer(period, self.timer_callback)
        self.get_logger().info(
            f'Publishing sensor_msgs/Imu on /sensors/imu at {rate} Hz'
        )

    def timer_callback(self):
        try:
            quat = self.imu.quaternion
            gyro = self.imu.gyro
            accel = self.imu.linear_acceleration
            self._consec_errors = 0
        except Exception:
            self._consec_errors += 1
            if self._consec_errors >= 25:
                self.get_logger().warn('25 consecutive read failures')
                self._consec_errors = 0
            return

        if quat is None or gyro is None or accel is None:
            return

        msg = Imu()

        msg.header.stamp    = self.get_clock().now().to_msg()
        msg.header.frame_id = self.frame_id

        # Orientation (rotation vector, sensor-fused)
        msg.orientation.x = quat[0]
        msg.orientation.y = quat[1]
        msg.orientation.z = quat[2]
        msg.orientation.w = quat[3]
        msg.orientation_covariance[0] = 0.01
        msg.orientation_covariance[4] = 0.01
        msg.orientation_covariance[8] = 0.01

        # Angular velocity (calibrated gyroscope, rad/s)
        msg.angular_velocity.x = gyro[0]
        msg.angular_velocity.y = gyro[1]
        msg.angular_velocity.z = gyro[2]
        msg.angular_velocity_covariance[0] = 0.001
        msg.angular_velocity_covariance[4] = 0.001
        msg.angular_velocity_covariance[8] = 0.001

        # Linear acceleration (gravity removed, m/s²)
        msg.linear_acceleration.x = accel[0]
        msg.linear_acceleration.y = accel[1]
        msg.linear_acceleration.z = accel[2]
        msg.linear_acceleration_covariance[0] = 0.1
        msg.linear_acceleration_covariance[4] = 0.1
        msg.linear_acceleration_covariance[8] = 0.1

        self.imu_pub.publish(msg)

    def destroy_node(self):
        super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = BNO085I2CNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        try:
            rclpy.shutdown()
        except Exception:
            pass


if __name__ == '__main__':
    main()