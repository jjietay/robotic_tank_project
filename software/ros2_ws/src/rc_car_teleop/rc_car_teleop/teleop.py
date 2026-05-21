#!/usr/bin/env python3
# ---------------------------------------------------------------------------
#                   teleop.py  —  Manual WASD driving (ROS 2)
# ---------------------------------------------------------------------------
#  Controls:
#      W / S    forward / backward   (linear.x = ± V_MAX_MPS)
#      A / D    spin left / right    (angular.z = ± MAX_ANGULAR)
#      Space    explicit stop
#      Q        quit
#
#  Design: tracks an ACTIVE command as state and publishes it every timer
#  tick at 50 Hz, regardless of whether a key event arrived this tick.
#  This avoids jitter from the OS key-repeat initial delay (~500 ms on
#  macOS), which would otherwise trigger the hold-timeout and send a
#  spurious STOP before repeat events begin.
#
#  HOLD_TIMEOUT_MS must be > the OS initial key-repeat delay so the robot
#  doesn't stop during that window.  600 ms works for the macOS default.
#  After releasing a key, the robot stops within one key-repeat interval
#  (≈33 ms) + HOLD_TIMEOUT_MS.
# ---------------------------------------------------------------------------

import sys
import termios
import tty
import select
import time

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist

# Must be longer than the OS initial key-repeat delay (≈500 ms on macOS).
HOLD_TIMEOUT_MS = 250.0

WHEEL_BASE_M = 0.1488
V_MAX_MPS    = 0.25 # slow cruise; raise once stable
MAX_ANGULAR  = V_MAX_MPS / (WHEEL_BASE_M / 2.0) - 0.20


def configure_terminal():
    fd = sys.stdin.fileno()
    original = termios.tcgetattr(fd)
    tty.setraw(fd)
    return fd, original


def restore_terminal(original):
    fd = sys.stdin.fileno()
    termios.tcsetattr(fd, termios.TCSADRAIN, original)


def read_key(fd):
    """Non-blocking single-character read. Returns None if no key available."""
    rlist, _, _ = select.select([fd], [], [], 0.0)
    if rlist:
        return sys.stdin.read(1)
    return None


class TeleopNode(Node):
    def __init__(self):
        super().__init__('teleop')
        self.cmd_pub = self.create_publisher(Twist, 'cmd_vel', 10)

        # Active command state — published every tick until hold-timeout clears it.
        self.active_linear  = 0.0
        self.active_angular = 0.0
        self.active_label   = 'X'
        self.last_key_time  = time.time()

        self.fd, self.original = configure_terminal()
        self.get_logger().info(
            'Hold WASD to drive, release to stop. Space = stop, Q = quit.'
        )

        # Publish at 50 Hz — active command is sent every tick.
        self.timer = self.create_timer(0.02, self.timer_callback)

    # ------------------------------------------------------------------
    #                          Helpers
    # ------------------------------------------------------------------
    @staticmethod
    def _make_twist(linear_x: float = 0.0, angular_z: float = 0.0) -> Twist:
        t = Twist()
        t.linear.x  = float(linear_x)
        t.angular.z = float(angular_z)
        return t

    def _set_active(self, label: str, linear: float, angular: float) -> None:
        """Update active command state and log on change."""
        if label != self.active_label:
            self.active_label = label
            sys.stdout.write(f'\rCMD: {label}   ')
            sys.stdout.flush()
        self.active_linear  = linear
        self.active_angular = angular

    def _shutdown(self) -> None:
        try:
            self.cmd_pub.publish(self._make_twist())
        except Exception:
            pass
        sys.stdout.write('\rCMD: Stop   \n')
        sys.stdout.flush()
        self.get_logger().info('Q pressed — shutting down.')
        try:
            restore_terminal(self.original)
        except Exception:
            pass
        try:
            self.timer.cancel()
        except Exception:
            pass
        rclpy.shutdown()

    # ------------------------------------------------------------------
    #                          Main timer (50 Hz)
    # ------------------------------------------------------------------
    def timer_callback(self):
        try:
            key = read_key(self.fd)

            if key is not None:
                # A key event arrived — update active command and reset timeout.
                self.last_key_time = time.time()

                if key in ('w', 'W'):
                    self._set_active('W',  V_MAX_MPS,  0.0)
                elif key in ('s', 'S'):
                    self._set_active('S', -V_MAX_MPS,  0.0)
                elif key in ('a', 'A'):
                    self._set_active('A',  0.0,  MAX_ANGULAR)
                elif key in ('d', 'D'):
                    self._set_active('D',  0.0, -MAX_ANGULAR)
                elif key == ' ':
                    self._set_active('X',  0.0,  0.0)
                elif key in ('q', 'Q'):
                    self._shutdown()
                    return
            else:
                # No key this tick — check hold-timeout for key-release detection.
                elapsed_ms = (time.time() - self.last_key_time) * 1000.0
                if elapsed_ms >= HOLD_TIMEOUT_MS:
                    self._set_active('X', 0.0, 0.0)

            # Always publish the active command every tick.
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