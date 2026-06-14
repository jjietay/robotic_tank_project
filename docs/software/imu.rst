IMU
===

The tank reads its BNO085 IMU on the Raspberry Pi. There are two nodes for it.
The I2C node is the one used during normal running. The RVC node is an option
that reads the same sensor over a serial connection.

.. automodule:: rc_car_teleop.robot_core.bno085_i2c_node
   :members:

.. automodule:: rc_car_teleop.robot_core.bno085_rvc_node
   :members:
