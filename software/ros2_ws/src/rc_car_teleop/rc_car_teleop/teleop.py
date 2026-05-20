#!/usr/bin/env python3
# ---------------------------------------------------------------------------
#                   teleop.py  —  Manual WASD driving (ROS 2)
# ---------------------------------------------------------------------------
#  Publishes geometry_msgs/Twist on /cmd_vel.  Sends raw setpoints in m/s 
#  and rad/s — the firmware applies its own slew-rate limiting, so this
#  node does NOT need to ramp on its own side.  When a brain / nav2 node
#  takes over later, it can publish to the same topic with the same
#  conventions and motion will be smooth automatically.
#
#  Controls:
#      W / S    forward / backward   (linear.x = ± V_MAX_MPS)
#      A / D    spin left / right    (angular.z = ± MAX_ANGULAR)
#      Space    explicit stop
#      Q        quit
#
#  Holding a key pulses ~30–50 Hz from the OS key-repeat.  If no key has
#  been seen for HOLD_TIMEOUT_MS, the node publishes a zero Twist so the
#  robot stops on key-release.
# ---------------------------------------------------------------------------

import sys
import termios 
import tty
import select
import time

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist

HOLD_TIMEOUT_MS = 200.0
WHEEL_BASE_M    = 0.168
# Slow cruise speed for manual driving.  Keeping this well above the
# minimum-duty floor (~0.17 m/s at 0.28 duty) prevents the motors from
# stalling in the deadband.  Raise towards 0.60 once the tank is stable.
V_MAX_MPS       = 0.25
MAX_ANGULAR     = V_MAX_MPS / (WHEEL_BASE_M / 2.0)


def configure_terminal():
    fd = sys.stdin.fileno()
    original = termios.tcgetattr(fd)
    tty.setraw(fd)
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

        self.last_cmd      = 'X'
        self.last_key_time = time.time()

        self.fd, self.original = configure_terminal()
        self.get_logger().info(
            'Hold WASD to drive, release to stop. Space = stop, Q = quit.'
        )

        # 50 Hz publish loop is plenty — firmware smooths anyway.
        self.timer = self.create_timer(0.02, self.timer_callback)

    # ------------------------------------------------------------------
    #                          Twist builders
    # ------------------------------------------------------------------
    @staticmethod
    def _twist(linear_x: float = 0.0, angular_z: float = 0.0) -> Twist:
        t = Twist()
        t.linear.x  = float(linear_x)
        t.angular.z = float(angular_z)
        return t

    def _publish(self, cmd_label: str, twist: Twist) -> None:
        if cmd_label != self.last_cmd:
            self.last_cmd = cmd_label
            sys.stdout.write(f'\rCMD: {cmd_label}   ')
            sys.stdout.flush()
        self.cmd_pub.publish(twist)

    def _shutdown(self) -> None:
        # Best-effort: send a zero Twist so the firmware watchdog isn't the
        # only thing stopping the robot.
        try:
            self.cmd_pub.publish(self._twist())
        except Exception:
            pass
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

    # ------------------------------------------------------------------
    #                          Main timer
    # ------------------------------------------------------------------
    def timer_callback(self):
        try:
            key = read_key(self.fd, 0.0)

            if key is not None:
                self.last_key_time = time.time()

                if key in ('w', 'W'):
                    self._publish('W', self._twist(linear_x=V_MAX_MPS))
                elif key in ('s', 'S'):
                    self._publish('S', self._twist(linear_x=-V_MAX_MPS))
                elif key in ('a', 'A'):
                    self._publish('A', self._twist(angular_z=MAX_ANGULAR))
                elif key in ('d', 'D'):
                    self._publish('D', self._twist(angular_z=-MAX_ANGULAR))
                elif key == ' ':
                    self._publish('X', self._twist())
                elif key in ('q', 'Q'):
                    self._shutdown()
                    return
            else:
                # No key seen this tick — if it's been long enough since the
                # last keypress, treat the key as released and stop.
                now_s = time.time()
                if (now_s - self.last_key_time) * 1000.0 >= HOLD_TIMEOUT_MS:
                    if self.last_cmd != 'X':
                        self._publish('X', self._twist())

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