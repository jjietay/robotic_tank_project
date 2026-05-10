#!/usr/bin/env python3
import sys
import termios
import tty
import select
import time

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist

HOLD_TIMEOUT_MS = 200.0
WHEEL_BASE_M = 0.356
# Drive setpoint: kept below the firmware's physical V_MAX_MPS (0.14) so the
# PID has duty-cycle headroom to correct left/right wheel asymmetry.
V_MAX_MPS    = 0.13
MAX_ANGULAR = V_MAX_MPS / (WHEEL_BASE_M / 2)

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
        super().__init__('teleop')

        self.cmd_pub = self.create_publisher(Twist, 'cmd_vel', 10)

        self.last_cmd = 'X'
        self.last_key_time = time.time()
        # Cached desired twist — published every tick so the firmware's
        # cmd_vel watchdog (500 ms) never trips while a key is held through
        # the terminal's auto-repeat delay.
        self.current_twist = Twist()

        self.fd, self.original = configure_terminal()
        self.get_logger().info(
            'Hold WASD to drive, release to stop. '
            'Q to quit.'
        )

        self.timer = self.create_timer(0.02, self.timer_callback)

    def update_cmd_label(self, cmd):
        if cmd is not None and cmd != self.last_cmd:
            self.last_cmd = cmd
            sys.stdout.write(f'\rCMD: {cmd}   ')
            sys.stdout.flush()

    def timer_callback(self):
        try:
            key = read_key(self.fd, 0.0)
            cmd = None

            if key is not None:
                self.last_key_time = time.time()

                if key in ('w', 'W'):
                    cmd = 'W'
                    self.current_twist = Twist()
                    self.current_twist.linear.x = V_MAX_MPS
                elif key in ('s', 'S'):
                    cmd = 'S'
                    self.current_twist = Twist()
                    self.current_twist.linear.x = -V_MAX_MPS
                elif key in ('a', 'A'):
                    cmd = 'A'
                    self.current_twist = Twist()
                    self.current_twist.angular.z = -MAX_ANGULAR
                elif key in ('d', 'D'):
                    cmd = 'D'
                    self.current_twist = Twist()
                    self.current_twist.angular.z = MAX_ANGULAR
                elif key == ' ':
                    cmd = 'X'
                    self.current_twist = Twist()
                elif key in ('q', 'Q'):
                    self.current_twist = Twist()
                    self.cmd_pub.publish(self.current_twist)
                    sys.stdout.write('\rCMD: Stop   ')
                    sys.stdout.flush()
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

                self.update_cmd_label(cmd)

            else:
                now = time.time()
                if (now - self.last_key_time) * 1000.0 >= HOLD_TIMEOUT_MS:
                    if self.last_cmd != 'X':
                        self.current_twist = Twist()
                        self.update_cmd_label('X')

            # Always publish the cached twist so the firmware sees a steady
            # stream and its watchdog doesn't zero the setpoint between
            # auto-repeat key events.
            self.cmd_pub.publish(self.current_twist)

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
