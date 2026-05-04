main() Loop
=========

See also: :doc:`main.cpp <main_cpp>`

Startup sequence
----------------

The ``main()`` function performs:

- standard I/O initialisation,
- sensor and actuator object construction,
- micro-ROS transport setup,
- node, publisher, subscriber, and executor setup.

Control loop
------------

After initialisation, the firmware repeatedly:

1. rate-limits execution,
2. processes incoming ROS messages,
3. measures one ultrasonic sensor,
4. computes motor commands,
5. applies directional safety checks,
6. drives the motors,
7. publishes encoder counts.

Why this matters
----------------

This loop is the behavioural heart of the Pico firmware. It is where sensing,
communication, and actuation come together.