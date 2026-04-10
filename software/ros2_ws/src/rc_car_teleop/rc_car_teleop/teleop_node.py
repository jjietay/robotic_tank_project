#!/usr/bin/env python3
import sys
import termios
import tty
import select
import time
import serial

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist


SERIAL_PORT = "/dev/ttyACM0"
BAUD_RATE = 115200
HOLD_TIMEOUT_MS = 200.0


def configure_terminal():
    fd = sys.stdin.fileno()
    original = termios.tcgetattr(fd)
    tty.setraw(fd)  # FIX 1: was tty.setraw(fda)
    return fd, original


def restore_terminal(original):
    fd = sys.stdin.fileno()
    termios.tcsetattr(fd, termios.TCSADRAIN, original)


def read_key(fd, timeout_ms):
    rlist, _, _ = select.select([fd], [], [], timeout_ms / 1000.0)
    if rlist:
        return sys.stdin.read(1)
    return None


class TeleopNode(Node):
    def __init__(self):
        super().__init__('teleop_node')

        self.get_logger().info(f'Opening serial port {SERIAL_PORT} @ {BAUD_RATE}')
        self.ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)

        self.cmd_pub = self.create_publisher(Twist, 'cmd_vel', 10)

        self.last_cmd = b'X'  # FIX 6: Pico stop = 'X', not 'S'
        self.last_key_time = time.time()

        self.fd, self.original = configure_terminal()
        self.get_logger().info(
            'Hold WASD to drive, release to stop. '
            'U/I = fwd-left/right, O/P = bwd-left/right. Q to quit.'
        )

        self.timer = self.create_timer(0.02, self.timer_callback)

    def send_cmd_and_twist(self, cmd, twist):
        if cmd is not None and cmd != self.last_cmd:
            self.ser.write(cmd)
            self.last_cmd = cmd
            sys.stdout.write(f'\rCMD: {cmd.decode("ascii")}   ')
            sys.stdout.flush()
        if twist is not None:
            self.cmd_pub.publish(twist)

    def timer_callback(self):
        try:
            key = read_key(self.fd, 0.0)
            cmd = None
            twist = None

            if key is not None:
                self.last_key_time = time.time()

                if key in ('w', 'W'):
                    cmd = b'W'            # FIX 2, 4: correct Pico cmd + forward = +1.0
                    twist = Twist()
                    twist.linear.x = 1.0
                elif key in ('s', 'S'):
                    cmd = b'S'            # FIX 2, 4: correct Pico cmd + backward = -1.0
                    twist = Twist()
                    twist.linear.x = -1.0
                elif key in ('a', 'A'):
                    cmd = b'A'            # FIX 2: correct Pico cmd
                    twist = Twist()
                    twist.angular.z = 1.0   # CCW = left in ROS convention
                elif key in ('d', 'D'):
                    cmd = b'D'            # FIX 2: correct Pico cmd
                    twist = Twist()
                    twist.angular.z = -1.0  # CW = right in ROS convention
                elif key in ('u', 'U'):   # FIX 5: diagonal support
                    cmd = b'U'
                    twist = Twist()
                    twist.linear.x = 1.0
                    twist.angular.z = 1.0
                elif key in ('i', 'I'):   # FIX 5
                    cmd = b'I'
                    twist = Twist()
                    twist.linear.x = 1.0
                    twist.angular.z = -1.0
                elif key in ('o', 'O'):   # FIX 5
                    cmd = b'O'
                    twist = Twist()
                    twist.linear.x = -1.0
                    twist.angular.z = 1.0
                elif key in ('p', 'P'):   # FIX 5
                    cmd = b'P'
                    twist = Twist()
                    twist.linear.x = -1.0
                    twist.angular.z = -1.0
                elif key == ' ':
                    cmd = b'X'            # FIX 3: stop = 'X'
                    twist = Twist()
                elif key in ('q', 'Q'):
                    try:
                        self.ser.write(b'X')  # FIX 3
                        sys.stdout.write('\rCMD: Stop   ')
                        sys.stdout.flush()
                    except Exception:
                        pass
                    self.get_logger().info('Q pressed, shutting down teleop node.')
                    try:
                        restore_terminal(self.original)
                    except Exception:
                        pass
                    try:
                        self.timer.cancel()
                    except Exception:
                        pass
                    rclpy.shutdown()
                    return

                self.send_cmd_and_twist(cmd, twist)

            else:
                now = time.time()
                if (now - self.last_key_time) * 1000.0 >= HOLD_TIMEOUT_MS:
                    if self.last_cmd != b'X':   # FIX 3, 6: sentinel is 'X'
                        cmd = b'X'
                        twist = Twist()
                        self.send_cmd_and_twist(cmd, twist)

        except Exception as e:
            self.get_logger().error(f'Error in teleop loop: {e}')
            try:
                restore_terminal(self.original)
            except Exception:
                pass
            try:
                self.timer.cancel()
            except Exception:
                pass
            rclpy.shutdown()

    def destroy_node(self):
        try:
            restore_terminal(self.original)
        except Exception:
            pass
        try:
            self.ser.write(b'X')  # FIX 3: stop = 'X'
            self.ser.close()
        except Exception:
            pass
        super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = TeleopNode()
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
