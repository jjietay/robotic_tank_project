Teleoperation
=============

This node provides keyboard-based teleoperation for the tank. It reads key
presses directly from the terminal, converts them into ``geometry_msgs/msg/Twist``
messages, and publishes velocity commands on ``cmd_vel``. Keyboard teleop nodes
in ROS commonly publish ``Twist`` commands to a velocity topic such as
``cmd_vel``.

Overview
--------

- Node name: ``teleop``
- Published topic:

  - ``cmd_vel`` (type: ``geometry_msgs/msg/Twist``)

- Main responsibilities:

  - Read keyboard input in raw terminal mode
  - Convert valid keys into velocity commands
  - Continuously publish movement commands while keys are being pressed
  - Stop the robot automatically when input is released for too long
  - Restore terminal settings on shutdown or error

Controls
--------

The current implementation supports these keys:

- ``W``: move forward
- ``S``: move backward
- ``A``: rotate left
- ``D``: rotate right
- ``Space``: stop
- ``Q``: quit the teleop node


Imports
-------

.. code-block:: python

   import sys
   import termios
   import tty
   import select
   import time

   import rclpy
   from rclpy.node import Node
   from geometry_msgs.msg import Twist

- ``termios`` and ``tty`` are used to switch the terminal into raw mode so key
  presses can be read immediately.
- ``select`` is used to check for available keyboard input without blocking.
- ``time`` is used to detect when the key has been released long enough to stop
  the robot.
- ``Twist`` is the standard ROS message for velocity commands, with linear and
  angular components.

Terminal Helper Functions
-------------------------

Terminal configuration:

.. code-block:: python

   def configure_terminal():
       fd = sys.stdin.fileno()
       original = termios.tcgetattr(fd)
       tty.setraw(fd)
       return fd, original

- Saves the current terminal settings
- Switches the terminal into raw mode
- Returns the file descriptor and original settings so they can be restored later

Terminal restoration:

.. code-block:: python

   def restore_terminal(original):
       fd = sys.stdin.fileno()
       termios.tcsetattr(fd, termios.TCSADRAIN, original)

- Restores the terminal back to its normal state
- This is essential because raw mode can leave the terminal unusable if the
  program exits badly

Non-blocking key read:

.. code-block:: python

   def read_key(fd, timeout_ms):
       rlist, _, _ = select.select([fd], [], [], timeout_ms / 1000.0)
       if rlist:
           return sys.stdin.read(1)
       return None

- Waits for keyboard input for up to ``timeout_ms``
- Returns one character if input is available
- Returns ``None`` if no key was pressed during the timeout window

Class Definition
----------------

.. code-block:: python

   class TeleopNode(Node):

This class encapsulates terminal handling, keyboard input, command generation,
and ROS publishing.

Constructor
-----------

.. code-block:: python

   def __init__(self):
       super().__init__('teleop')

       self.cmd_pub = self.create_publisher(Twist, 'cmd_vel', 10)

       self.last_cmd = 'X'
       self.last_key_time = time.time()

       self.fd, self.original = configure_terminal()
       self.get_logger().info(
           'Hold WASD to drive, release to stop. '
           'U/I = fwd-left/right, O/P = bwd-left/right. Q to quit.'
       )

       self.timer = self.create_timer(0.02, self.timer_callback)

Key initialization steps:

- Registers the node as ``teleop``
- Creates a publisher on ``cmd_vel``
- Stores the last command sent so duplicate command prints can be avoided
- Stores the time of the last key press for hold-to-drive behaviour
- Configures the terminal into raw mode
- Starts a timer that runs every 0.02 seconds, which is 50 Hz

The 50 Hz timer allows the node to feel responsive while still using a clean
event-loop design instead of a busy blocking loop.

Publishing Commands
-------------------

.. code-block:: python

   def send_cmd_and_twist(self, cmd, twist):
       if cmd is not None and cmd != self.last_cmd:
           self.last_cmd = cmd
           sys.stdout.write(f'\\rCMD: {cmd}   ')
           sys.stdout.flush()
       if twist is not None:
           self.cmd_pub.publish(twist)

This helper function does two things:

- Updates the terminal display when the command changes
- Publishes the corresponding ``Twist`` message if one exists

This is a good design choice because it keeps output logic separate from
keyboard interpretation.

Main Teleop Loop
----------------

.. code-block:: python

   def timer_callback(self):
       try:
           key = read_key(self.fd, 0.0)
           cmd = None
           twist = None

           if key is not None:
               self.last_key_time = time.time()

               if key in ('w', 'W'):
                   cmd = 'W'
                   twist = Twist()
                   twist.linear.x = 1.0
               elif key in ('s', 'S'):
                   cmd = 'S'
                   twist = Twist()
                   twist.linear.x = -1.0
               elif key in ('a', 'A'):
                   cmd = 'A'
                   twist = Twist()
                   twist.angular.z = 1.0
               elif key in ('d', 'D'):
                   cmd = 'D'
                   twist = Twist()
                   twist.angular.z = -1.0
               elif key == ' ':
                   cmd = 'X'
                   twist = Twist()
               elif key in ('q', 'Q'):
                   ...
               self.send_cmd_and_twist(cmd, twist)

This callback runs every timer tick and checks for key input.

Command mapping:

- ``W`` sets ``twist.linear.x = 1.0`` for forward motion
- ``S`` sets ``twist.linear.x = -1.0`` for backward motion
- ``A`` sets ``twist.angular.z = 1.0`` for left rotation
- ``D`` sets ``twist.angular.z = -1.0`` for right rotation
- ``Space`` sends a zero ``Twist`` to stop the robot

This matches the standard ROS idea of using ``Twist`` linear and angular fields
for velocity control.

Hold-to-Drive Safety Behaviour
------------------------------

.. code-block:: python

   HOLD_TIMEOUT_MS = 200.0

   ...
   else:
       now = time.time()
       if (now - self.last_key_time) * 1000.0 >= HOLD_TIMEOUT_MS:
           if self.last_cmd != 'X':
               cmd = 'X'
               twist = Twist()
               self.send_cmd_and_twist(cmd, twist)

If no key has been received for 200 ms, the node automatically publishes a
zero-velocity command.

This is the right behaviour for keyboard teleop on a robot. It reduces the risk
of the robot continuing to move after the operator releases the key or loses
terminal focus.

Quit Handling
-------------

.. code-block:: python

   elif key in ('q', 'Q'):
       try:
           self.ser.write('X')
           sys.stdout.write('\\rCMD: Stop   ')
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

When ``Q`` is pressed, the node attempts to:

- Stop the robot
- Restore the terminal
- Cancel the timer
- Shut down ROS 2

