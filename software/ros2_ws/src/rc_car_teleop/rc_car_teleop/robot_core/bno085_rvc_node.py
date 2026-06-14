#!/usr/bin/env python3
"""BNO085 IMU node over UART in RVC mode.

Reads yaw, pitch, roll, and linear acceleration from a BNO085 IMU in UART RVC
mode and publishes them as ``sensor_msgs/msg/Imu`` on ``/sensors/imu``. RVC
mode does not provide angular velocity, so that field is left at zero and its
covariance is set to negative one to mark it as unknown.
"""

import math
import os
import subprocess
import time

import serial
from adafruit_bno08x_rvc import BNO08x_RVC

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Imu


def free_serial_port(port: str) -> None:
    """Free the serial port by killing any process still holding it.

    Args:
        port (str): the serial port path, such as ``/dev/serial0``.
    """
    try:
        real = os.path.realpath(port)
        subprocess.run(['fuser', '-k', real],
                       stdout=subprocess.DEVNULL,
                       stderr=subprocess.DEVNULL,
                       check=False)
        time.sleep(0.5)
    except FileNotFoundError:
        pass


def euler_to_quaternion(yaw_deg: float, pitch_deg: float, roll_deg: float):
    """Convert Euler angles in degrees to a quaternion (x, y, z, w).

    Uses the ZYX convention (yaw about Z, pitch about Y, roll about X), which
    matches ROS REP 103.

    Args:
        yaw_deg (float): yaw in degrees.
        pitch_deg (float): pitch in degrees.
        roll_deg (float): roll in degrees.

    Returns:
        tuple: the quaternion as (x, y, z, w).
    """
    yaw   = math.radians(yaw_deg)
    pitch = math.radians(pitch_deg)
    roll  = math.radians(roll_deg)

    cy = math.cos(yaw   * 0.5)
    sy = math.sin(yaw   * 0.5)
    cp = math.cos(pitch * 0.5)
    sp = math.sin(pitch * 0.5)
    cr = math.cos(roll  * 0.5)
    sr = math.sin(roll  * 0.5)

    w = cr * cp * cy + sr * sp * sy
    x = sr * cp * cy - cr * sp * sy
    y = cr * sp * cy + sr * cp * sy
    z = cr * cp * sy - sr * sp * cy

    return x, y, z, w


class BNO085RVCNode(Node):
    """ROS 2 node that publishes BNO085 IMU data read over UART in RVC mode.

    Publishes:
        ``/sensors/imu`` (sensor_msgs/msg/Imu): orientation and linear
        acceleration.

    Parameters:
        serial_port (str): serial device path. Default ``/dev/serial0``.
        baudrate (int): serial baud rate. Default 115200.
        frame_id (str): frame id for the IMU messages. Default ``imu_link``.
        publish_rate (double): publish rate in Hz. Default 50.
    """

    def __init__(self):
        """Free the serial port, open it, and start the publish timer."""
        super().__init__('bno085_rvc')

        self.declare_parameter('serial_port', '/dev/serial0')
        self.declare_parameter('baudrate', 115200)
        self.declare_parameter('frame_id', 'imu_link')
        self.declare_parameter('publish_rate', 50.0)

        port     = self.get_parameter('serial_port').value
        baudrate = self.get_parameter('baudrate').value
        self.frame_id = self.get_parameter('frame_id').value
        rate     = self.get_parameter('publish_rate').value

        self.get_logger().info(f'Clearing any stale holders of {port}...')
        free_serial_port(port)

        self.get_logger().info(f'Opening {port} at {baudrate} baud...')
        self.uart = None
        for attempt in range(5):
            try:
                self.uart = serial.Serial(port, baudrate=baudrate, timeout=1.0)
                break
            except serial.SerialException as e:
                self.get_logger().warn(
                    f'Open failed (attempt {attempt + 1}/5): {e}'
                )
                time.sleep(1.0)
        if self.uart is None:
            raise RuntimeError(f'Could not open {port} after 5 attempts')

        time.sleep(1.0)
        self.uart.reset_input_buffer()
        self.rvc = BNO08x_RVC(self.uart)
        self.get_logger().info('BNO085 RVC connected.')

        self.imu_pub = self.create_publisher(Imu, '/sensors/imu', 10)

        self._consec_errors = 0

        period = 1.0 / rate
        self.timer = self.create_timer(period, self.timer_callback)

        self.get_logger().info(
            f'Publishing sensor_msgs/Imu on /sensors/imu at {rate} Hz'
        )

    def timer_callback(self):
        """Read one RVC sample, convert it, and publish it as an Imu message."""
        try:
            yaw, pitch, roll, x_accel, y_accel, z_accel = self.rvc.heading
            self._consec_errors = 0
        except Exception:
            self._consec_errors += 1
            if self._consec_errors >= 25:
                try:
                    self.uart.reset_input_buffer()
                except Exception:
                    pass
                self._consec_errors = 0
                self.get_logger().warn('Stream out of sync, flushed buffer.')
            return

        qx, qy, qz, qw = euler_to_quaternion(yaw, pitch, roll)

        msg = Imu()

        msg.header.stamp    = self.get_clock().now().to_msg()
        msg.header.frame_id = self.frame_id

        msg.orientation.x = qx
        msg.orientation.y = qy
        msg.orientation.z = qz
        msg.orientation.w = qw
        msg.orientation_covariance[0] = 0.01
        msg.orientation_covariance[4] = 0.01
        msg.orientation_covariance[8] = 0.01

        msg.angular_velocity.x = 0.0
        msg.angular_velocity.y = 0.0
        msg.angular_velocity.z = 0.0
        msg.angular_velocity_covariance[0] = -1.0

        msg.linear_acceleration.x = float(x_accel)
        msg.linear_acceleration.y = float(y_accel)
        msg.linear_acceleration.z = float(z_accel)
        msg.linear_acceleration_covariance[0] = 0.1
        msg.linear_acceleration_covariance[4] = 0.1
        msg.linear_acceleration_covariance[8] = 0.1

        self.imu_pub.publish(msg)

    def destroy_node(self):
        """Close the serial port and destroy the node."""
        try:
            self.uart.close()
        except Exception:
            pass
        super().destroy_node()


def main(args=None):
    """Start the BNO085 RVC node and spin until interrupted."""
    rclpy.init(args=args)
    node = BNO085RVCNode()
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
