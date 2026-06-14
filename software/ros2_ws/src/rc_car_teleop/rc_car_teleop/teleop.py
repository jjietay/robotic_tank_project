#!/usr/bin/env python3
"""Keyboard teleop node.

Reads WASD key presses from the terminal and publishes velocity commands as
``geometry_msgs/msg/Twist`` on ``/teleop/cmd_vel``. W and S drive forward and
back, A and D turn, Space stops, and Q quits. The active command is sent on a
50 Hz timer, so the robot keeps moving while a key is held and stops shortly
after it is released.
"""

import sys
import termios
import tty
import select
import time

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist

HOLD_TIMEOUT_MS = 500.0

WHEEL_BASE_M = 0.135
V_MAX_MPS    = 0.175
MAX_ANGULAR  = V_MAX_MPS / (WHEEL_BASE_M / 2.0) - 0.20


def configure_terminal():
    """Put the terminal into raw mode and return its previous settings.

    Returns:
        tuple: the stdin file descriptor and the original terminal settings.
    """
    fd = sys.stdin.fileno()
    original = termios.tcgetattr(fd)
    tty.setraw(fd)
    return fd, original


def restore_terminal(original):
    """Restore the terminal to its original settings.

    Args:
        original: the settings returned by :func:`configure_terminal`.
    """
    fd = sys.stdin.fileno()
    termios.tcsetattr(fd, termios.TCSADRAIN, original)


def read_key(fd):
    """Read a single key without blocking.

    Args:
        fd: the stdin file descriptor.

    Returns:
        str or None: the key that was pressed, or None if no key is waiting.
    """
    rlist, _, _ = select.select([fd], [], [], 0.0)
    if rlist:
        return sys.stdin.read(1)
    return None


class TeleopNode(Node):
    """ROS 2 node that turns WASD key presses into velocity commands.

    Publishes:
        ``/teleop/cmd_vel`` (geometry_msgs/msg/Twist): the velocity command.
    """

    def __init__(self):
        """Set up the publisher, the command state, and the 50 Hz timer."""
        super().__init__('teleop')
        self.cmd_pub = self.create_publisher(Twist, '/teleop/cmd_vel', 10)

        self.active_linear  = 0.0
        self.active_angular = 0.0
        self.active_label   = 'X'
        self.last_key_time  = time.time()

        self.fd, self.original = configure_terminal()
        self.get_logger().info(
            'Hold WASD to drive, release to stop. Space = stop, Q = quit.'
        )

        self.timer = self.create_timer(0.02, self.timer_callback)

    @staticmethod
    def _make_twist(linear_x: float = 0.0, angular_z: float = 0.0) -> Twist:
        """Build a Twist with the given linear and angular values.

        Args:
            linear_x (float): forward velocity in metres per second.
            angular_z (float): turn rate in radians per second.

        Returns:
            Twist: the velocity message.
        """
        t = Twist()
        t.linear.x  = float(linear_x)
        t.angular.z = float(angular_z)
        return t

    def _set_active(self, label: str, linear: float, angular: float) -> None:
        """Set the active command and print the label when it changes.

        Args:
            label (str): short label for the current command.
            linear (float): forward velocity in metres per second.
            angular (float): turn rate in radians per second.
        """
        if label != self.active_label:
            self.active_label = label
            sys.stdout.write(f'\rCMD: {label}   ')
            sys.stdout.flush()
        self.active_linear  = linear
        self.active_angular = angular

    def _shutdown(self) -> None:
        """Stop the robot, restore the terminal, and shut down ROS."""
        try:
            self.cmd_pub.publish(self._make_twist())
        except Exception:
            pass
        sys.stdout.write('\rCMD: Stop   \n')
        sys.stdout.flush()
        self.get_logger().info('Q pressed, shutting down.')
        try:
            restore_terminal(self.original)
        except Exception:
            pass
        try:
            self.timer.cancel()
        except Exception:
            pass
        rclpy.shutdown()

    def timer_callback(self):
        """Read a key, update the command, and publish it every tick.

        If a key arrived, the matching command is set. If no key has arrived
        for longer than ``HOLD_TIMEOUT_MS``, the command is reset to stop. The
        active command is published on every tick.
        """
        try:
            key = read_key(self.fd)

            if key is not None:
                self.last_key_time = time.time()

                if key in ('w', 'W'):
                    self._set_active('W',  V_MAX_MPS,  0.0)
                elif key in ('s', 'S'):
                    self._set_active('S', -V_MAX_MPS,  0.0)
                elif key in ('d', 'D'):
                    self._set_active('D',  0.0,  MAX_ANGULAR)
                elif key in ('a', 'A'):
                    self._set_active('A',  0.0, -MAX_ANGULAR)
                elif key == ' ':
                    self._set_active('X',  0.0,  0.0)
                elif key in ('q', 'Q'):
                    self._shutdown()
                    return
            else:
                elapsed_ms = (time.time() - self.last_key_time) * 1000.0
                if elapsed_ms >= HOLD_TIMEOUT_MS:
                    self._set_active('X', 0.0, 0.0)

            self.cmd_pub.publish(
                self._make_twist(self.active_linear, self.active_angular)
            )

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
        """Restore the terminal and then destroy the node."""
        try:
            restore_terminal(self.original)
        except Exception:
            pass
        super().destroy_node()


def main(args=None):
    """Start the teleop node and spin until interrupted."""
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
