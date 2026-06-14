#!/usr/bin/env python3
"""BNO085 IMU node over I2C.

Reads orientation, angular velocity, and linear acceleration from a BNO085 IMU
over I2C and publishes them as ``sensor_msgs/msg/Imu`` on ``/sensors/imu``. The
first reading is used to zero the heading, so yaw starts at zero on every boot.
"""

import time
import math

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


def quaternion_multiply(q1, q2):
    """Multiply two quaternions given as (x, y, z, w)."""
    x1, y1, z1, w1 = q1
    x2, y2, z2, w2 = q2
    return (
        w1*x2 + x1*w2 + y1*z2 - z1*y2,
        w1*y2 - x1*z2 + y1*w2 + z1*x2,
        w1*z2 + x1*y2 - y1*x2 + z1*w2,
        w1*w2 - x1*x2 - y1*y2 - z1*z2,
    )


def yaw_from_quaternion(x, y, z, w):
    """Return the yaw angle in radians from a quaternion."""
    return math.atan2(2.0 * (w*z + x*y), 1.0 - 2.0 * (y*y + z*z))


def yaw_offset_quaternion(yaw):
    """Return a quaternion that rotates by the negative of the given yaw."""
    half = -yaw / 2.0
    return (0.0, 0.0, math.sin(half), math.cos(half))


class BNO085I2CNode(Node):
    """ROS 2 node that publishes BNO085 IMU data read over I2C.

    Publishes:
        ``/sensors/imu`` (sensor_msgs/msg/Imu): orientation, angular velocity,
        and linear acceleration.

    Parameters:
        i2c_bus (int): I2C bus number. Default 4.
        frame_id (str): frame id for the IMU messages. Default ``imu_link``.
        publish_rate (double): publish rate in Hz. Default 50.
    """

    def __init__(self):
        """Open the IMU on I2C, enable the reports, and start the timer."""
        super().__init__('bno085_i2c')

        self.declare_parameter('i2c_bus', 4)
        self.declare_parameter('frame_id', 'imu_link')
        self.declare_parameter('publish_rate', 50.0)

        bus_num       = self.get_parameter('i2c_bus').value
        self.frame_id = self.get_parameter('frame_id').value
        rate          = self.get_parameter('publish_rate').value

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
        self.get_logger().info('BNO085 I2C connected, 3 reports enabled.')

        self._yaw_offset_q = None

        self.imu_pub = self.create_publisher(Imu, '/sensors/imu', 10)
        self._consec_errors = 0

        period = 1.0 / rate
        self.timer = self.create_timer(period, self.timer_callback)
        self.get_logger().info(
            f'Publishing sensor_msgs/Imu on /sensors/imu at {rate} Hz'
        )

    def timer_callback(self):
        """Read one IMU sample, apply the heading offset, and publish it."""
        try:
            quat  = self.imu.quaternion
            gyro  = self.imu.gyro
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

        x, y, z, w = quat[0], quat[1], quat[2], quat[3]

        if self._yaw_offset_q is None:
            boot_yaw = yaw_from_quaternion(x, y, z, w)
            self._yaw_offset_q = yaw_offset_quaternion(boot_yaw)
            self.get_logger().info(
                f'IMU heading zeroed, boot yaw was {math.degrees(boot_yaw):.1f}°'
            )

        cx, cy, cz, cw = quaternion_multiply(self._yaw_offset_q, (x, y, z, w))

        msg = Imu()
        msg.header.stamp    = self.get_clock().now().to_msg()
        msg.header.frame_id = self.frame_id

        msg.orientation.x = cx
        msg.orientation.y = cy
        msg.orientation.z = cz
        msg.orientation.w = cw
        msg.orientation_covariance[0] = 0.01
        msg.orientation_covariance[4] = 0.01
        msg.orientation_covariance[8] = 0.01

        msg.angular_velocity.x = gyro[0]
        msg.angular_velocity.y = gyro[1]
        msg.angular_velocity.z = gyro[2]
        msg.angular_velocity_covariance[0] = 0.001
        msg.angular_velocity_covariance[4] = 0.001
        msg.angular_velocity_covariance[8] = 0.001

        msg.linear_acceleration.x = accel[0]
        msg.linear_acceleration.y = accel[1]
        msg.linear_acceleration.z = accel[2]
        msg.linear_acceleration_covariance[0] = 0.1
        msg.linear_acceleration_covariance[4] = 0.1
        msg.linear_acceleration_covariance[8] = 0.1

        self.imu_pub.publish(msg)

    def destroy_node(self):
        """Destroy the node."""
        super().destroy_node()


def main(args=None):
    """Start the BNO085 I2C node and spin until interrupted."""
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
