#!/usr/bin/env python3
# ---------------------------------------------------------------------------
#        bno085_rvc_node.py  —  BNO085 UART-RVC → sensor_msgs/Imu
# ---------------------------------------------------------------------------
#  Reads yaw/pitch/roll + linear acceleration from the BNO085 in UART-RVC
#  mode and publishes sensor_msgs/Imu on /sensors/imu.
#
#  RVC mode gives Euler angles (degrees) and linear acceleration (m/s²).
#  Angular velocity is NOT available in RVC mode, so that field is zeroed
#  and its covariance is set to -1 (unknown).
#
#  Usage:
#    sudo python3 bno085_rvc_node.py
#
#  Or copy to your ROS 2 workspace and run via ros2 run.
# ---------------------------------------------------------------------------

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
    """Kill any leftover process still holding the serial port.

    Crashes and rough Ctrl+C exits can leave a dead process gripping the
    UART, which silently eats the BNO085's bytes so the next run gets
    nothing. fuser -k clears them. We resolve the symlink (e.g.
    /dev/serial0 -> /dev/ttyS0) because fuser needs the real device.
    """
    try:
        real = os.path.realpath(port)
        subprocess.run(['fuser', '-k', real],
                       stdout=subprocess.DEVNULL,
                       stderr=subprocess.DEVNULL,
                       check=False)
        time.sleep(0.5)   # give the OS a moment to release the handle
    except FileNotFoundError:
        # fuser not installed — skip; not fatal.
        pass


def euler_to_quaternion(yaw_deg: float, pitch_deg: float, roll_deg: float):
    """Convert Euler angles (degrees) to quaternion (x, y, z, w).

    Uses ZYX convention (yaw around Z, pitch around Y, roll around X),
    which matches ROS REP-103.
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
    def __init__(self):
        super().__init__('bno085_rvc')

        # -- Parameters (can override via command line) --
        self.declare_parameter('serial_port', '/dev/serial0')
        self.declare_parameter('baudrate', 115200)
        self.declare_parameter('frame_id', 'imu_link')
        self.declare_parameter('publish_rate', 50.0)  # Hz

        port     = self.get_parameter('serial_port').value
        baudrate = self.get_parameter('baudrate').value
        self.frame_id = self.get_parameter('frame_id').value
        rate     = self.get_parameter('publish_rate').value

        # -- Serial + RVC setup --
        # Clear any zombie process holding the port (from a prior crash).
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
        self.uart.reset_input_buffer()   # drop any partial boot data
        self.rvc = BNO08x_RVC(self.uart)
        self.get_logger().info('BNO085 RVC connected.')

        # -- Publisher --
        self.imu_pub = self.create_publisher(Imu, '/sensors/imu', 10)

        # Recovery state: track consecutive read failures so we can reset
        # the input buffer if the stream gets out of sync (e.g. after a
        # brief power glitch on the IMU).
        self._consec_errors = 0

        # -- Timer --
        period = 1.0 / rate
        self.timer = self.create_timer(period, self.timer_callback)

        self.get_logger().info(
            f'Publishing sensor_msgs/Imu on /sensors/imu at {rate} Hz'
        )

    def timer_callback(self):
        try:
            yaw, pitch, roll, x_accel, y_accel, z_accel = self.rvc.heading
            self._consec_errors = 0
        except Exception:
            # Occasional UART hiccup — skip this tick. But if many in a row,
            # the stream is likely out of sync; flush the buffer to resync.
            self._consec_errors += 1
            if self._consec_errors >= 25:   # ~0.5 s of failures at 50 Hz
                try:
                    self.uart.reset_input_buffer()
                except Exception:
                    pass
                self._consec_errors = 0
                self.get_logger().warn('Stream out of sync — flushed buffer.')
            return

        qx, qy, qz, qw = euler_to_quaternion(yaw, pitch, roll)

        msg = Imu()

        # -- Header --
        msg.header.stamp    = self.get_clock().now().to_msg()
        msg.header.frame_id = self.frame_id

        # -- Orientation (from onboard sensor fusion) --
        msg.orientation.x = qx
        msg.orientation.y = qy
        msg.orientation.z = qz
        msg.orientation.w = qw
        # Orientation covariance: small values — BNO085 fusion is good.
        msg.orientation_covariance[0] = 0.01   # roll  variance
        msg.orientation_covariance[4] = 0.01   # pitch variance
        msg.orientation_covariance[8] = 0.01   # yaw   variance

        # -- Angular velocity: NOT available in RVC mode --
        msg.angular_velocity.x = 0.0
        msg.angular_velocity.y = 0.0
        msg.angular_velocity.z = 0.0
        # Set first element to -1 to indicate "unknown" per REP-145.
        msg.angular_velocity_covariance[0] = -1.0

        # -- Linear acceleration (gravity removed by BNO085) --
        msg.linear_acceleration.x = float(x_accel)
        msg.linear_acceleration.y = float(y_accel)
        msg.linear_acceleration.z = float(z_accel)
        msg.linear_acceleration_covariance[0] = 0.1
        msg.linear_acceleration_covariance[4] = 0.1
        msg.linear_acceleration_covariance[8] = 0.1

        self.imu_pub.publish(msg)

    def destroy_node(self):
        try:
            self.uart.close()
        except Exception:
            pass
        super().destroy_node()


def main(args=None):
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