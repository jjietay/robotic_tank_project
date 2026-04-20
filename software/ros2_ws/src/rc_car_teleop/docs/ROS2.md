# <center>__ROS 2 Fundamentals__<center>

## <center>_(A) Basic Buidling Blocks_<center>
### (1) nodes
- process that performs some task
- implemented as subclass that inherites from ROS node class
- requires oops' encapsulation and abstraction
- each nodes run in their own runtime environement
- each node is written in its own file such as python and cpp
- can be written in almost any programming language

### (2) Messages
- communication "language" between nodes
- contain 1 or more fields with strongly-typed data such as string int floating point arrays etc
- each Message must be defined by an interface

### (3) Interface
- format of messsage
- descrption of what kind of data the message should contain
- programming api which defines what kind of data should be recieved or sent by a node either in a topic or a service
- ROS comes with a number of pre-defined interfaces, you can also define your own
- nodes can instantiate 1 or more of these messaging types (interfaces)
- nodes can have more that 1 publishers, subscribers, clients and servers

### (4) Method of Communication between nodes:
#### (a) Topics
- named communication channel for publish-subscribe model
- nodes can subscribe to 1 or more topics and when a message is published to that topic, all subscribing nodes will recieve it

#### (b) Service (Synchronous Communication method)
- Client will send a request to a server
- Server is expected to reply with a response
- Requests-Response are specific type of messages

### (5) Package
- basic unit of organization in ROS
- minimum amount of code with all the supporting files needed to make something run in ROS

### (6) Logging
- is how a node prints structured messages about what it's doing (status, warnings, errors) instead of random `print()`s
- Each node has a logger with levels like DEBUG, INFO, WARN, ERROR, FATAL (lowest to highest in severity level)
- INFO is for normal status, ERROR/FATAL for serious problems
- logs can go into terminal, to files, and to `/rosout` so you can monitor many nodes at once
- Tools can filter logs by node or level, which is impossible with raw `print()` spam

### (7) Callbacks
- a callback is just a function that ROS promises to call when some event happens: such as message received, timer flies, parameter changes, etc
- this is the main thing that keep ROS 2 code modular
- one callback handles LIDAR messages, another handles /cmd_vel, another runs every 10ms as a control loop

### (8) Timers
- timer is ROS's way of saying run this function every X seconds
- we need to give ROS a --> period and a callback
- ROS will then call that function on schedule
- Timers integrate into ROS executor, so it play nicely with other callbacks (subscriptions, services)

### (9) Parameters
- named configuration value stored inside a node (like a setting)
- it can be an int, float, bool, string or list
- they let you change behaviour without editing code
- You can set them via command line, YAML, or launch files
- Each node has its own parameters set

---
## <center>_(B) Publisher Format: (Example: cmd_vel_publisher_node.py)_<center>
### (1) Imports
```python
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
```
- twist is a message type
- twist's interface is as such:
     - linear velocity (x, y, z) vector
     - angular velocity (x, y, z) vector
- For differential drive/tank, we use linear.x (forward/backward) and angular.z (yaw rotation)

### (2) Create Class
```python
class CmdVelPublisher(Node):
```
- declares a python class
- this class inherits from Node
- Node gives this class all the built-in ROS 2 node features: logging, creating publishers/suscribers, timers, parameters, etc

```python
def __init__(self):
    super().__init__('minimal_publisher')
    self.publisher_ = self.create_publisher(Twist, '/cmd_vel', 10)
    timer_period = 0.5  # seconds
    self.timer = self.create_timer(timer_period, self.timer_callback)
    self.i = 0
```
- `super().__init__('cmd_vel_publisher_node')` just creates a node titled: cmd_vel_publisher_node, this is what actually registers the node with the ROS graph, without this, its just a python object not a ROS node
- `self.publisher_ = self.create_publisher(Twist, '/cmd_vel', 10)` creates a *publisher* and stores it in self.publisher_
- Twist is the message type, every message published through this publisher will be a  `Twist`
- `/cmd_vel` = the topic name
- any subscriber listening on `/cmd_vel` with type `Twist` can receive these messages
- `10` = queue size, ROS will buffer up to 10 messages if subscribers are slow
older ones get dropped (FIFO)
- `timer_period` = 0.5 seconds is a plain python variable which sets the time interval between timer callbacks to 0.5 seconds

```self.timer = self.create_timer(timer_period, self.timer_callback)```
- creates a *ROS 2 Timer*
- every `timer_period` seconds (0.5 seconds), ROS calls self.timer_callback()
- `self.timer` holds the timer object so it doesn't get garbade-collected

`self.i` = 0
- initialize a counter, often used to count how messages have been sent


```python    
def timer_callback(self):
    msg = Twist()
    msg.linear.x = 0.1   # forward speed
    msg.angular.z = 0.0  # no rotation
    self.publisher_.publish(msg)
    self.get_logger().info(f"Publishing: linear.x={msg.linear.x}, angular.z={msg.angular.z}")
    self.i += 1
```
- this function is called every 0.5 seconds by timer above
- `msg = Twist()` creates a new empty `Twist` message object
- `msg.linear` is a `Vector3` with default 0 values
- `msg.angular` is another `Vector3` with default 0
- `msg.linear.x = 0.1` sets the forward speed to 0.1 units
- `msg.angular.z = 0.0` means no rotation/go straight. For ground robot, +ve means ACW
- `self.publisher_.publish(msg)` sends the `Twist` message our on the `/cmd_vel` topic
- any receivers that subscribe to the topic will receive it and act on it
`self.get_logger().info(f"Publishing: linear.x={msg.linear.x}, angular.z={msg.angular.z}")`
- uses node's built in logger to print an INFO-level log message
- printed log messsage as shown below:
`Publishing: linear.x=0.1, angular.z=0.0`

- `self.i += 1` increments internal counter after each publish

```python
def main(args=None):
    rclpy.init(args=args)
    cmd_vel_publisher = CmdVelPublisher()
    rclpy.spin(cmd_vel_publisher)
    # Destroy the node explicitly
    # (optional - otherwise it will be done automatically
    # when the garbage collector destroys the node object)
    cmd_vel_publisher.destroy_node()
    rclpy.shutdown()
```
- `def main(args=None):` runs when we execute this script directly or via `ros2 run`
- `rclpy.init(args=args)` init ROS 2 client library for this process, parsed ROS-related command line arguements and gets the middleware ready, NOTE: MUST BE CREATED BEFORE CREATING ANY NODES
- `cmd_vel_publisher = CmdVelPublisher()`:
    - creates an insteance of class
    - calls `CmdVelPublisher.__init__`
    - calls `Node.__init__`
    - registers the node in the ROS graph
    - Creates publisher and timer
- `rclpy.spin(cmd_vel_publisher)` Enters the ROS 2 event loop for this node, while `spin` is running:
    - the node stays alive
    - ROS calls callbacks when events happen (timers, subscriptions, services)
    - In this case, timer fires every 0.5s, `timer_callback` gets called, messages are published
- `spin` blocks until you shut down (Ctrl-C)
- `cmd_vel_publisher.destroy_node()` explicitly destroys the node instance 
- `rclpy.shutdown()` shutdowns rclpy library 

---
## <center>_(C) Subscriber Format: (Example: cmd_vel_subscriber_node.py)_<center>

### (1) Imports
```python
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
```

- `Twist` is the same message type as the publisher.
- The subscriber **must** use the same message type and topic name as the publisher to communicate.
- `Node` and `rclpy` are used the same way as in the publisher: to define a ROS 2 node class and manage its lifecycle.


### (2) Create Class
```python
class CmdVelSubscriber(Node):
```

- Declares a Python class named `CmdVelSubscriber`.
- Inherits from `Node`, so it has ROS 2 features: logging, subscriptions, timers, parameters, etc.
- Conceptually: this node **listens** to `/cmd_vel` and reacts to incoming `Twist` messages.


### (3) Constructor
```python
    def __init__(self):
        super().__init__('cmd_vel_subscriber')
        self.subscription = self.create_subscription(
            Twist,
            '/cmd_vel',
            self.cmd_vel_callback,
            10)
        self.subscription  # prevent unused variable warning
```

- `def __init__(self):`  
  Constructor; runs once when you create a `CmdVelSubscriber()` instance.

- `super().__init__('cmd_vel_subscriber')`  
  - Creates a ROS 2 node titled `cmd_vel_subscriber`.  
  - Registers the node with the ROS graph so it appears in `ros2 node list` and can connect to topics.

- `self.subscription = self.create_subscription(...)` creates a **subscription** and stores it in `self.subscription`:

  - `Twist`: message type this subscription expects (must match the publisher on `/cmd_vel`).
  - `'/cmd_vel'`: topic name to listen on (must match the publisher exactly).
  - `self.cmd_vel_callback`: callback function that will be called whenever a `Twist` arrives.
  - `10`: queue size (buffer up to 10 incoming messages if the callback is a bit slow).

- `self.subscription` keeps a reference so the subscription object isn’t garbage-collected.


### (4) Callback
```python
    def cmd_vel_callback(self, msg: Twist):
        linear_x = msg.linear.x
        angular_z = msg.angular.z
        self.get_logger().info(
            f"Received cmd_vel: linear.x={linear_x}, angular.z={angular_z}"
        )
```

- `def cmd_vel_callback(self, msg: Twist):`  
  Defines the **subscription callback**.  
  - `msg` is the `Twist` message received from the publisher, filled in by ROS when a message arrives.

- `linear_x = msg.linear.x` reads the forward/backward velocity from the message.

- `angular_z = msg.angular.z` reads the yaw rotation (turning rate) from the message.

- `self.get_logger().info(...)`  
  Logs the received values so you can see what is coming in on `/cmd_vel`.  
  In a real robot, this is where you could convert the velocities to wheel commands and send them to motors.


### (5) `main()` and Program Entry
```python
def main(args=None):
    rclpy.init(args=args)

    cmd_vel_subscriber = CmdVelSubscriber()

    rclpy.spin(cmd_vel_subscriber)

    cmd_vel_subscriber.destroy_node()
    rclpy.shutdown()
```

- `def main(args=None):`  
  Entry point; runs when the script is executed.

- `rclpy.init(args=args)`  
  Initializes the ROS 2 client library; must be called before any nodes are created.

- `cmd_vel_subscriber = CmdVelSubscriber()`  
  Creates an instance of the subscriber node class and runs its `__init__`, which creates the subscription.

- `rclpy.spin(cmd_vel_subscriber)`  
  Enters the ROS 2 event loop.  
  While spinning:
  - The node stays alive.
  - Whenever a `Twist` message arrives on `/cmd_vel`, ROS calls `cmd_vel_callback`.

- `cmd_vel_subscriber.destroy_node()`  
  Explicitly destroys the node instance and unregisters it from the ROS graph.

- `rclpy.shutdown()`  
  Shuts down the ROS 2 client library when you’re done.


```python
if __name__ == '__main__':
    main()
```

- Standard Python boilerplate to call `main()` when the file is run as a script.