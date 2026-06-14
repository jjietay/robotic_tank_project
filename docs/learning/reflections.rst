Reflections
================================

See also: :doc:`ROS 2 Notes <ros2>`


.. note::
   This is a long reflection, if you wanna just skip to major periods just before the final results, click one of these dates in the right corner table of dates:

   - 30 May 2026 --> RViz2 
   - 7 June 2026 --> Nav2


A personal log of difficulties faced, solutions found, and lessons learned
while building the autonomous tank.

Background
----------
From the two years into uni, I realised I wasn't actually learning the things I was interested in which are AI and robotics. I have always wondered how do robots "think". After learning about neural networks and transformers, I realise I wanted to build a robot. Also, uni didn't teach me any of these. Moreover, I needed to take matters in my own hands and actually learn this such that it will help me in expanding my knowledge pool and also in my future career. 

I figured if I could build something real using sensors, SLAM, Nav2, and actually have a working autonomous robot that I fully understand how its done, then I'd come out the other side actually understanding robotics. Moreover, with my VLA-based FYP for robotics systems in my final year, having this hands-on foundation felt important to me.

I decided to build a self-driving tank. Why a tank you may ask. If I'm being honest, part of it is that I've been fascinated by tanks since I was a kid (spent abit too much time playing world of tanks), so building one felt like the obvious choice. But beyond the nerdy appeal, I also knew that actually building something physical was the only way I'd retain any of this. I needed something I could break, fix, and learn from.

----

26 Feb 26 - Start of Project
------------------------------

After researching and consulting AI what I should purchase, I spend nearly a crazy ~$200 out of my own pocket to purchase the items that included Rasberry Pi 4, Pico W, Pi Camera Module 2, YDLidar X3 Pro, basic arduino car kit (included with motors), H bridge, 18560 batteries.

.. figure:: ../_images/reflections_img1.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 1: Before building version 1

When the kit in Figure 1 came, I couldn't wait to start. I followed some YouTube Tutorial on it and started building. I learnt how to write a simple micropython script and then make it such that it recieves the velocity commands over USB to my RPI4. I've heard that ROS was a pain to set up, so I deliberately kept it out of the picture, so I can focus in ensuring hardware and my python script is correct.

.. figure:: ../_images/reflections_img2.jpg
   :alt: After building
   :width: 500px
   :align: center

   Figure 2: After building version 1

As shown in Figure 2 above, my current build contains RPI4, Pico W, 18650 batteries, the L298N H Bridge that is wired to my motors. The L298N required many connections. For a 4 wheel (4 motor) setup, I had to combine the motor voltage wires together for each side. Dir pins control both motors at each side.

This "simple" setup, took me awhile because of many problems. The table below shows some problems faced and respective solutions.

.. list-table::
   :header-rows: 1
   :widths: 50 50

   *  - Problem
      - Solution

   *  - weak (loose) connection
      - wiggle the wires to make sure they don't just fall out and secure them if they do fall out
   *  - code issues
      - make changes to the code and verify with AI (my project mentor)
   *  - motor forward/backward direction issues
      - switch the pins (best to switch it hardware wise than software which might confuse myself in the future when reviewing the codebase)
   *  - cold solder joints
      - fill edges of soldering iron's tip with solder first, repeat it multiple times, and ensure that soldering joint is hot

This first success building the car made me more motivated and excited to continue on with my project. 

.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls style="display: block; margin: auto;">
      <source src="../_static/reflections_vid1.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 1: Car moving around and spinning</figcaption>
   </figure>

Video 1 above shows my car finally moving, controlled via my keyboard, WASD.

----

12 Mar 26 - 1st Real Difficulty
--------------------------------

.. figure:: ../_images/reflections_img3.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 3: Poor solder joint on my motors

From Figure 3, we can see that I took out the motors. This is because the motors were not reliable due to bad solder joint.

.. figure:: ../_images/reflections_img4.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 4: Poor soldering iron quality

From Figure 4, you can tell that I was using a really cheap soldering iron and the solder connections became so unreliable, sometimes the car won't even move. This was when I knew I had to invest in a good soldering setup to ensure issues like these won't arise in the future again. I bought a new soldering iron (reddit users recommendations) and seperate quality solder with higher flux percentage.

When the new soldering iron came, I soldered on new pair of wires onto the motor flaps and everything worked. I knew the next steps were to add my LIDAR and bigger portable onto my car.

26 Mar 26 - Improved Chassis, or is it?
-----------------------------------------

When I fitted the my bigger powerbank, everything changed. The whole system got heavier, and my cheap TT motors were struggling to move the car. When moving forward or backwards, it worked fine. But when it was tank turning left or right, it didn't budge. This is because the TT motors are not producing enough torque force to overcome the static friction.


.. figure:: ../_images/reflections_img5-.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 5: New chassis & motor

From Figure 5, we can see that I am using the TP-101 chassis with 2 provided 33GB-520 motors. The tank chassis certainly helped with distribution the weight across the tracks.

The specifications of both are displayed in the table below.

.. list-table::
   :header-rows: 1
   :stub-columns: 1
   :widths: 15 20 20

   *  -
      - Old Chassis w TT Motors
      - New Chassis w 33GB-520 Motors

   *  - Number of Motors
      - 4 Motors
      - 2 Motors

   *  - Voltage
      - 3-6 V
      - 6-12 V

   *  - No-load Speed
      - 90-200 RPM
      - 170-350 RPM

   *  - Max Torque (all motors combined)
      - 3.2 kg cm
      - 5 kg cm

   *  - Gearbox
      - Plastic
      - All metal

   *  - Current (no-load)
      - 170-250 mA
      - 100 mA


.. figure:: ../_images/reflections_img6.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 6: After building

Figure 6 shows the final product after building. Notice the rasberry camera module 2 installed. I tested it by streaming live feed and viewing it on my macbook via VLC player. I noticed that the camera FOV is small. This might impact the robot's ability to see things at corners of the image. Therefore, I have decided to get another camera that have a greater FOV.

9 Apr 26 - Camera problems
---------------------------

.. list-table::
   :header-rows: 1
   :stub-columns: 1
   :widths: 15 20 20

   *  -
      - RPI Camera Module 2
      - OV5647 130 degrees

   *  - Sensor
      - Sony IMX219
      - OmniVision OV5647

   *  - Resolution
      - 8MP
      - 5MP

   *  - Horizontal FOV
      - 62.2 degrees
      - 130 degrees

   *  - Focal Length
      - F2.0
      - F2.9

When I got the OV5647, I tried it and observed a larger FOV. However, when it took it and pulled the latch on the Camera Serial Interface (CSI) to unlock it (so that I can remove the ribbon attached to my OV5637, it **broke**.

.. figure:: ../_images/reflections_img7.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 7: Broken latch

Figure 7 shows my destroyed CSI. I looked up online and realised that the CSI is notorious for getting damaged. I could replace the whole CSI piece and solder on a new one, or I could get a USB camera. I didn't take the risk, and bought a USB camera with similar FOV. 

9 Apr 26 - Lack of Encoder & Motor Driver Upgrade
---------------------------------------------------

I began researching more into SLAM, and realised that I need an encoder for my wheel so that I can calculate the odometry for my base_link.

I had a choice of adding an encoder to my current motor, but it wasn't feasible as it might increase inaccuracies (since there are more hardware building), and I wouldn't wanna mess up the encoder accuracy since that directly affects drift.

Therefore, I decided to purchase the JBG37-520-12V-60RPM with Encoder already attached to it. The table below shows the differences.

.. list-table::
   :header-rows: 1
   :stub-columns: 1
   :widths: 15 20 25

   *  - 
      - 33GB-520
      - JBG37-520-12V-60RPM with Encoder

   *  - Encoder
      - No
      - Yes

   *  - Working Voltage
      - 6-12 V
      - 12 V

   *  - Rated Torque (per motor)
      - 1.6 kg cm
      - up to 30 kg cm (at 37 RPM)

   *  - No-load Speed
      - 170-350 RPM
      - 70 RPM

From the table above, we can see that JBG37-520-12V-60RPM motor gives ALOT of torque, and that is good for my application where speed is not really important, but torque and precision in movement is.

JBG37-520 uses AB-phase Quadrature Hall Encoders. The encoder outputs 11 Pulses per Revolution (PPR). This means that the effective output shaft resolution is ``11 * gear_ratio``. For this 60RPM variant, it is around 1848 counts/rev which is excellent resolution.

Additionally, the existing H-bridge proved to be quite inefficient, dissipating a significant amount of energy as heat, with a voltage drop of nearly 2V across it. To address this, I sourced the Cytron MDD10A Dual Channel 10A DC Motor Driver, which uses a fully NMOS H-Bridge design to achieve significantly lower voltage drop and improved efficiency.

16 Apr 26 - Motor Mismatch + LIDAR's custom fit
-------------------------------------------------

When the motor came, I eagerly tried to yoink it in place. But to my surprise, it didn't fit. I did check the shaft dimensions before purchasing, but turned out the old motor I was using had a different sizing from the spec sheet found online.

.. figure:: ../_images/reflections_img8.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 8: Motor to Chassis Connection

From Figure 8 above, we can see the different parts required to connect the motor shaft to the drive wheel. First, the mini motor housing extrusion of C has to fit through the hole in A. Coupler in D is then attached onto motor shaft, and attached to drive wheel as shown in B.

However, my new motor's housing extrusion can't fit through the hole in A, and the coupler couldn't attach to the motor shaft.

To fix this, I have decided to customize my chassis by:

1) **3D printing** my own custom chassis with custom coupler
2) Mounting my LIDAR on custom **acrylic**

27 Apr 26 - Acrylic
-----------------------

For acrlyic, my main goal is just to drill holes so that I can fit my LIDAR. 

.. figure:: ../_images/reflections_img9-.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 9: 3D model of chassis

Figure 9 shows how it looked like.

20 Apr 26 - 3D Prints
-----------------------

As for the 3D printing, I saw this as an opportunity for me to learn 3D design, and I downloaded Autodesk Fusion. I spent the first few hours watching youtube tutorials and then I got started.

.. figure:: ../_images/reflections_img10.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 10: 3D model of chassis

Figure 10 shows the overall 3D view of the chassis. It features motor mounts for me to slide in the motors and secure with screws. At the top are holes for me to add standoffs to secure my components such as powerbank, motor driver, RPI4 and 18650 batteries.

.. figure:: ../_images/reflections_img11.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 11: Details of Chassis & Motor Coupler

Figure 11 shows the details of the designs. The motor housing is intentionally pushed slightly inwards due to its longer extrusion (at the shaft) compared to the previous motor. Moreover, there are also 5 screw holes for secure fit. As for the coupler, it is a simple design where the shaft slides into the bottom and the drive wheel slides in at the top.

Some issues faced with this design was that the shaft turned while the coupler stayed in place. This made me learn about tolerances and how we have to account for that. Also, the length of coupler was not accurate. This meant that my drive wheel and supporting wheel were not aligned with respect to the track. Therefore, I had to re-measure and print a few more with (slight) differing lengths.

26 Apr 26 - How I learn coding/ROS 2
--------------------------------------
.. figure:: ../_images/reflections_img_holy_trinity.png
   :alt: Before building
   :width: 400px
   :align: center

   ~ holy trinity ~

Throughout the whole process while waiting for shipment to arrive (usually takes a week or two), I would take this time to brush up on my python, learn my C++, and also learn ROS from scratch. I found it a very big challenge at the beginning because this is all completely new to me.

My process was as such. First I used AI to teach me the relevant concepts. I will then ask it to generate some questions/practice for me to do. After that, it will check my code and also my output and give me my score. If I still don't feel confident about that particular exercise, I will ask it generate another one. Only when I understand at least 80% of it, I will ask it to teach me another concept. The exercise portion is so that I can learn actively instead of passively.

Also, I chose python for my codebase for RPI4 because its easier. When the full pipeline works, I might explore C++.

27 Apr 26 - Minor Upgrades: Ultrasonic Sensors and Electrical Schematics
-------------------------------------------------------------------------

I decided to get some new ultrasonic sensors and then decided to plan out the schematics and pin allocations. 

.. figure:: ../_images/reflections_img12-.png
   :alt: Before building
   :width: 1000px
   :align: center

   Figure 12: Fritzing Schematics

Figure 12 shows the schematics that I have created using Fritzing (an open-soruce application). It shows my main electronics used for connections. Since this is just prototyping, I am using a half sized breadboard and jumper wires for my wiring.

28 Apr 26 - Finished Hardware
-------------------------------

.. figure:: ../_images/reflections_img13.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 13: Finished Hardware

Figure 13 shows my completed hardware. At this point, I already have my main.cpp for my micro-ROS ready to be flashed onto my 

29 Apr 26 - E-Stop Function using Ultrasonic Sensors
-----------------------------------------------------

.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls style="display: block; margin: auto;">
      <source src="../_static/reflections_vid2.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 2: Car stopping when 15cm from an object</figcaption>
   </figure>

Video 2 shows the simple E-stop function using my ultrasonic sensors. It will stop when it detects an object 15cm in front and behind it. 


5 May 2026 - PID Controller
-----------------------------
.. figure:: ../_images/reflections_img14.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 14: Finished Hardware

I decided to first brush up on my concept of PID controller. Having studied this in theory and now using it irl, it was a step up. I knew that i should first begin with P, and gradually add in I and D and tune in base on how the car reacts.

When I calculate the PID values, I had to see the maximum error possible. Assuming maximum error, I can calculate the maximum PID output, and ensure that it doesn't go out of the maximum allowed value, i.e. saturation. Besides having a PID controller, we have a feedforward path that provides 80% of the motion, with PID just affect 20% of the final output by making tiny adjustments to the pwm.

I also added a slew rate that allows for smoother acceleration and deceleration.

6 May 2026 - Disaster #2
-----------------------------
.. figure:: ../_images/reflections_img15.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 15: Finished Hardware

PID controller works fine, but while moving the car, the coupler **snapped once again**. Since this entire project is self-funded and I have blasted a hole through my wallet, I had to save future costs of remaking and repurchasing this coupler and motor. Therefore, I decided to just get a full tank kit with correctly sized motors with encoders to ensure reliability. This way I can focus on what matters more, which is my SLAM and higher level algorithms. Though, what I 3D designed, the chassis and coupler, I wouldn't say its a waste of time since I did learn quite abit from this.


12 May 2026 - New Kit
-----------------------------
.. figure:: ../_images/reflections_img16.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 16: In progress

Figure 16 shows my new chassis halfway built. Had to think about placement of sensors, how to fit my lidar on top, ensuring maximum usage of space in this smaller chassis. Also had to ensure that weight is at best equally distributed.

.. figure:: ../_images/reflections_img17-.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 17: New IMU

Figure 17 shows a new IMU I bought. It is the BN0085 which has 9 degree of freedom. During implementation, I ensured that it is in the geometrical center of my build with z axis facing upwards, etc.

.. figure:: ../_images/reflections_img18.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 18: Soldering

Figure 18 shows some soldering work that I also did with this new kit. Mainly for the motor's connections.

.. figure:: ../_images/reflections_img19.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 19: URDF

Figure 19 shows the new model that i built as a urdf file and visualized using urdf-visualizer extension in vscode. I used provided chassis and ultrasonic sensors stl files in this design. I also added frame links in relation to base link.

.. figure:: ../_images/reflections_img20.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 20: Testing Notes

From Figure 20, we can see my testing notes. I tested the max speed and minimum duty cycle using debug code. I used encoder counts to estimate the distance and calculated the speed at max duty cycle (using timer from my stopwatch). As for minimum duty cycle, I create a debug code that steps down the duty cycle every 3 seconds, and I can manually count and see at which duty cycle the car can no longer move. However, from all these testing, I noticed the somehow the right motor is significantly weaker than the left.


20 May 2026 - Integration hell
-------------------------------

.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls style="display: block; margin: auto;">
      <source src="../_static/reflections_vid3.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 3: Tank forward movement biasing right</figcaption>
   </figure>

From Video 3 above, we can see that the tank is moving to the right significantly. This meant that the left motor could be stronger than the right. After checking against the published encoder ticks, I could verify this as the ticks were off by about 25%.

I thoroughly checked through my code because I thought it was a software issue, but there were nothing that caught my attention or could cause alarms.

To minimize possibility of PID implementation issue, I ran a quick debug code to test the car via the same teleop node but without PID control (so purely open loop), but the same issue arised. Nevertheless, I went straight to my Motor Driver board to test. The Cytron MDD10A has 4 built-in testing buttons that allow for current to flow directly to the motors. 

.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls style="display: block; margin: auto;">
      <source src="../_static/reflections_vid4.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 4: Difference in sound between left and right motor</figcaption>
   </figure>

From Video 4, we can clearly hear the difference is sound of motor when its running at max speed. This is using the bypass test buttons built-in on the motor driver. My worry is that there is a manufacturing defact of the right motor. However to be very sure, I decided to switch the motor's Output A and B on the motor driver. Meaning left motor's wiring to driver is replaced with right motor's wiring to driver, and vice versa. This is to see if the motor driver is the issue. However, after switching same thing happened, meaning motor driver was never the issue. 

.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls style="display: block; margin: auto;">
      <source src="../_static/reflections_vid5.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 5: Clanking sound when shaking right motor</figcaption>
   </figure>

Video 5 represents a clanking sound when shaking the right motor after removing it. This was my last straw. I decided that it was a faulty motor.

.. figure:: ../_images/reflections_img21.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 21: Motor Close Up

Figure 21 shows the motor when I removed the gear cover. I didn't see anything odd, just poor lubrication. When I shake the motor I could still hear the clanging, and I knew it was something to do with the interal windings of the actual motor itself. But I decided not to pursue this any further and just purchase new motors (with spares cos I can't afford waiting another 1.5 weeks whenever this happens).

23 May 2026 - RViz & Gazebo
---------------------------

.. figure:: ../_images/reflections_img22.png
   :alt: Before building
   :width: 800px
   :align: center

   Figure 22: RViz

Figure 22 shows my RViz model. Managed to set it up on my mac with much difficulty (thanks to apple sillicon).

.. figure:: ../_images/reflections_img23.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 23: Gazebo

Figure 23 shows Gazebo setup on my mac (also a pain due to apple's sillicon). 

29 May 2026 - IMU
------------------
When i tried running the BNO085 IMU, the ROS 2 topic ``/sensors/imu`` appeared, but no data was being published to that topic. I thought it was my firmware issue, because I had to create the fully written BNO085 driver on the pico W built around the SH2 library using I2C. So I used the built-in I2C GPIO pins on the Rasberry Pi 4 for testing. 

**Here are some things I did to debug this:**

- swapped SDA/SCL pair in case cables were reversed
- confirmed 3.29V with multimeter at sensor and a lit up IMU board
- tried 2 different STEMMA QT cables
- continuity tested the blue (SDA) cable
- pulsed RST pin to force hardware reset
- used rpi4 to test if imu can be detected using ``i2c detect``

**Problem 1: I2C Timing Violation**

- SCL and SDA are both open-drain, meaning devices can only pull a line *low*
- SDA needs to be stable before SCL risess
- BNO085 uses **clock stretching** which holds SCL low to signal that its not ready yet
- the problem comes when it releases SCL after stretching, it sometimes haven't finished settling SDA
- so SDA changes too close to, or even after the clock edge
- the data is sampled mid-transition and the byte is garbage or the device never appears to acknowledge at all

**Problem 2: Pi's I2C clock-stretching bug**

- i found online that Rasberry Pi's hardware I2C has a long-standing bug where it does not honour clock stretching properly

**Solution: Swap to UART-RVC Mode**

- BNO085 supports I2C, SPI, and UART
- UART-RVC stands for Robotics Vehicle Control
- a simplified, continuous serial communication protocol designed for hardware like the BNO085/BNO086 IMUs and robot vacuum cleaners
- UART is asynchronous and does not require clock wire to get out of sync with data
- they simply agree on a baud rate (115200)
- the sensor pushes a fixed-format packet out its TX line on a timer and the Pi listens on its RX line
- RCV gives Yaw, pitch, roll, linear acc automatic streaming at a fixed rate

**I found an alternative (Software bit-banged I2C on rpi4)**

- Eventually I came back and got I2C working on the rpi4
- Im using a software bit-banged I2C bus on a free GPIO pair instead of the hardware I2C peripheral (which im been using)
- Bit-banging means the Pi manually toggles the pins in software, which correctly honours clock stretching because I'm in full control of the timing

29 May 2026 - LIDAR
--------------------
.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls style="display: block; margin: auto;">
      <source src="../_static/reflections_vid6.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 6: LIDAR spinning</figcaption>
   </figure>

This took me long enough, but finally I have started setting up the LIDAR and its spinning. Managed to make the LIDAR node work and I was able to see the msg being published on my lidar topic. Next was setting it up so that I can see the laser scan on rviz.


30 May 2026 - RViz2
--------------------

.. figure:: ../_images/reflections_img24.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 24: lidar scans in rviz

Figure 24 shows my live lidar scans in my living room while my tank is stationary. The amount of satisfaction and dopamine I got from reaching this and actually seeing my lidar scan cannot be described by words. Month of grinding hardware, firmware and finally it paid off. Now I understand laying that base hardware foundation for any product/project is so important to ensure the later stages won't fall short. 

.. figure:: ../_images/reflections_img25.png
   :alt: Before building
   :width: 900px
   :align: center

   Figure 25: added map in rviz

Figure 25 shows my live lidar scans + my map in my living room, while stationary too.


.. figure:: ../_images/reflections_img26.png
   :alt: Before building
   :width: 900px
   :align: center

   Figure 26: alot of drift

In Figure 26, I was just driving around my living room, and it shows the drift between lidar and map. I noticed that when I move forwards in real life, the tank moved backwards in RViz. This was very odd. I went to check the axis and i realised that the red axis (+x) was facing backwards in RViz.This didn't make sense because I checked my urdf file and there was no problem.

.. figure:: ../_images/reflections_img27.png
   :alt: Before building
   :width: 900px
   :align: center

   Figure 27: fixed axis

Figure 27 shows the problem finally fixed. The red axis which represents the positive x direction is finally pointing in the same direction as in real life. Even though the problem occured just a few lines before, I actually spent a whole gruesome day trying to debug... running through my codebase and finding anything that is causing this issue. I went through my urdf joint origins, checked my mesh files orientation, verified tf tree in rviz, re-examined how ``robot_state_publisher`` was broadcasting transforms. The whole time the answer was sitting right there. The embarrassing error was that I didn't include a 0.000 in my rpy variable ``Rotation: in RPY (degree) [0.000, -0.000, 180.000]``. This made me realised that I need to pay more attention to the little details.

.. figure:: ../_images/reflections_img28.png
   :alt: Before building
   :width: 900px
   :align: center

   Figure 28: drift

Figure 28 shows another problem of drift. I verified that ``/odom``, ``/scan``, and ``sensors/imu`` were all actively publishing using ``ros2 topic hz``, and confirmed the EKF was outputting /odometry/filtered. I also suspected that my odom was not doing so well, at this point I tuned the EKF to weight odometry more heavily for linear velocity, since my IMU integration tends to drift on that axis. 

I also understand that my LIDAR may be giving poor results due to the characteristics of my living room, such as many dark coloured objects (sofa, piano), which abosrbs infrared light rather than reflecting it back, so the LIDAR struggles to detct them reliably. On top of that, my living room has long flat walls with no distinctive features, which makes it hard for the scan-matcher to localise accurately. I also found another issue where, my map doesn't show the rear of my bin even after moving behind it.

.. figure:: ../_images/reflections_img29.png
   :alt: Before building
   :width: 900px
   :align: center

   Figure 29: Before (left) and After (right) of mapping a smaller area

Figure 29 shows the before and after of mapping a smaller area i created. I realised that this drastically helps with the mapping of rear of objects, since the rear of the bin is now properly shown as a fully round object. However, the drift is still a problem.

5 June 2026 - Fixing Drift
---------------------------

This drift issue is a problem. I changed maps many times and remapped them but the laser scan was always drifting. After many hours of debugging, here's what I found.

1) EKF Config File (``ekf.yaml``)

   - i disabled ``odom0 yaw`` which is encoder's angular velocity
   - because my robot is tracked, the tracks will slip when doing turns
   - adding on my tracks are plastic... so the slippage will be really bad cos the grip with the floor is really bad
   - therefore we ignore the angular velocity
   - our encoders now handle the linear velocity
   - my BNO085 gyro now handles all the rotation
   - I also ensured that ``imu0_differential: false`` is set
   - this drastically improved my drift over time since the rotation vector already has magnetometer correction baked in
   - this was because BNO085 already has its own proprietery sensor fusion

2) ``odom`` to ``base_link`` transform

   - my oodm and EKF was both trying to publish
   - I had to comment out the ``self._publish_tf(now)`` in my odom node
   - TF chain was stabilized
   - But because of this error, SLAM Toolbox fell to using pure scan matching which accumulates error overtime

3) Odometry wheelbase calibration

   - I realised after reading some random online forum that for tracked vehicles, the wheelbase is larger than the physical track to track distance
   - Therefore I had to perform the rotation test

4) IMU Heading Zeroing

   - my BNO085 boots with whatever heading it powers up in
   - it doesnt zero it, which means IMU booted facing -47 degrees and then my EKF will start with this offset
   - my ``bno085_i2c_node.py`` recorrds the first valid quartenion's yaw and applies the inverse as an offset to all subsequent readings so yaw will always start with 0

.. figure:: ../_images/reflections_img30.png
   :alt: Before building
   :width: 900px
   :align: center

   Figure 30: Laser scans matching the walls after running around mapping the new map

Figure 30 shows the final result of a fully mapped out map with lidar scans still sticking to the walls after driving around.


.. figure:: ../_images/reflections_img100-.png
   :alt: Before building
   :width: 400px
   :align: center

Also forgot to include my live setup.


6 June 2026 - DDS Issues + Mapping
-----------------------------------

I was initially using fastDDS which is default for ROS 2 Humble. However, im using Pi as the robot, and my M2 mac for viz. FastDDS wasn't reliable with multi-machine discovery over WiFi and maybe its a skill issue, but this issue is known on reddit and also ROS community.

The community recommended CylconeDDS so i switched and I could see improvements. But after some sessions, the same issues happened again. At this point I feel like its a Robostack issue (m2 mac problems). I was thinking it was something to do with protocol level incompatibilities since rpi4's cylcloneDDS was apt installed, but mac's one was robotstack built so it was still kinda sus.

So I decided to use Foxglove Bridge. It worked wonders that's all i can say. No more issues of not finding the ROS topics anymore. The rpi4 runs a websocket server that exposes all the ROS topics over a standard wesocket protocol. On mac, I connect foxglove studio to ws://123.123.123.123:1234 using foxglove websocket protocol.

.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls style="display: block; margin: auto;">
      <source src="../_static/reflections_vid7-.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 7: Mapping</figcaption>
   </figure>

This is my mapping in Foxglove UI.


7 June 2026 - Nav2
---------------------------

.. figure:: ../_images/reflections_img31.png
   :alt: Before building
   :width: 900px
   :align: center

   Figure 31: Nav2 setup in Foxglove

Figure 31 shows my setup. The first issue i faced was when i selected my goal pose and i saw the planned path. When the robot rotated, only the map would stay in place while the robot and both the goal pose and planned path would rotate together with the robot. This didn't make sense because the goal pose and planned path should be fixed to the map. 

.. figure:: ../_images/reflections_img32.png
   :alt: Before building
   :width: 900px
   :align: center

   Figure 32: Nav2 setup in Foxglove

This was fixed by changing fixed frame AND display frame to map. Only fixed frame to map didn't work.

8 June 2026 - Past biting you
------------------------------

This was a massive issue, spent days fixing. Btw the timeline is abit off i spent many days fixing bugs between those timeline days. Please watch this video to understand the issue. The robot is the video is already trying autonomous navigation.

.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls style="display: block; margin: auto;">
      <source src="../_static/reflections_vid8.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 8: Fast forwarded video of Robot failing to turn the correct direction in Nav2</figcaption>
   </figure>

   This is a new remapped map so it looks slightly different. But now u can see that i selected the goal pose and the planned path is supposed to let the robot turn right, but it turned left instead. I finally found the issue after hours of debugging. I looked at my imu to make sure its pointing the right way, checking if CW rotation produced positive yaw in my topic and it did. Had to verify the differntial drive math... implementation hell. After debugging, the issue was my motor pin swapped. Hardware came back from the past to haunt me... All along my teleop node wasn't following the ROS-convention commands. However, Nav2 was sending the right commands. After swapping the motor and encoder pins in my pico firmware code and rebuilding and reflashing, it finally worked.

Another issue was my robot kept jittering, like move stop move stop at a very high frequency. I later found out its because i had my teleop node on on my phone which the ``/cmd_vel`` kept bouncing between my static (no) command and the Nav2's continuous command. It worked once i switched my teleop off.

12 June 2026 - Navigation
----------------------------

Finally, navigation worked! I have structured this in terms of nav from point A to B, and then B back to A. I noticed that B back to A takes longer because the starting pose at B is facing towards the wall so it needs to u turn which the robot finds it challenging. For the below navigation, im using Regulated Pure Pursuit (RPP) controller. I initially used Dynamic Window Approach (DWB) but its too computationally expensive (more about it in next timestep).

*-- Part 1: A to B --*
~~~~~~~~~~~~~~~~~~~~~~

.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls style="display: block; margin: auto;">
      <source src="../_static/reflections_vid11.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 9: A to B (Attempt 1)</figcaption>
   </figure>

Video 9 shows navigation from point A to point B (attempt 1). Notice there is slowing down near the 2 pillars.

.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls style="display: block; margin: auto;">
      <source src="../_static/reflections_vid10.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 10: A to B (Attempt 1 - LIVE)</figcaption>
   </figure>

Video 10 shows the same footage but live version of Video 9, from A to B (attempt 1). Same no slowing at the 2 pillars.

Below is attempt 2 from A to B.

.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls style="display: block; margin: auto;">
      <source src="../_static/reflections_vid9-.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 11: A to B (Attempt 2)</figcaption>
   </figure>

Video 11 shows navigation from point A to point B (attempt 2). Notice how it slows down near the pillars.


.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls style="display: block; margin: auto;">
      <source src="../_static/reflections_vid12.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 12: A to B (Attempt 2 - LIVE)</figcaption>
   </figure>

Video 12 shows navigation from point A to point B (attempt 2). Likewise, you can see how it slows down near the pillars. This is because I used ``use_regulated_linear_velocity_scaling: true``.

Now, lets look at the B to A motion.

*-- Part 2: B to A --*
~~~~~~~~~~~~~~~~~~~~~~~

.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls style="display: block; margin: auto;">
      <source src="../_static/reflections_vid15.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 13: B to A (Attempt 1)</figcaption>
   </figure>

Video 13 shows successful navigation from point B to point A but it took really long for it to get out. 

.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls style="display: block; margin: auto;">
      <source src="../_static/reflections_vid14.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 14: B to A (Attempt 1 - LIVE)</figcaption>
   </figure>

Video 14 shows the same point B to A as video 13 but live, and u can see how it really moves and "thinks" to get out of the sticky situation. I find this so intriguing.


Now move on to after optimization.

.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls style="display: block; margin: auto;">
      <source src="../_static/reflections_vid16.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 15: B to A (Attempt 2)</figcaption>
   </figure>

Video 15 shows successful navigation from point B to point A and it took way lesser time to get out. 

.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls style="display: block; margin: auto;">
      <source src="../_static/reflections_vid17.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 16: B to A (Attempt 2 - LIVE)</figcaption>
   </figure>

Video 16 shows the same point B to A as video 13 but live. You can see how it got out of the situation quicker and in a different way. This is because of a particular change i made in config, which was ``movement_time_allowance:``, this is a progress checker timeout where robot must fail to make progress for 20 seconds before Nav2 triggers recovery behaviours like spin and backup which was exactly what we can see.


14 June 2026 - Obstacle Avoidance
---------------------------------

For this part, I used both DWB and RPP and we can see how the per-cycle compute demands of DWB really couldn't be met by my rasberry pi 4. RPP on the other hand doesn't need to do trajectory sampling, that's why its so efficient. Planner also kept at 2Hz and it ran stably with RPP. For obstacle avoidance, I split it up into Far Range and Close Range. Close range being adding the obstacle last minute right in front of the robot. Far Range is adding the obstacle when the robot is still far away, so it has time to replan its route.

*-- DWB (Far Range) --*
~~~~~~~~~~~~~~~~~~~~~~~

.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls muted style="display: block; margin: auto;">
      <source src="../_static/dwb_far_range.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 17: DWB, Far range, Foxglove Studio</figcaption>
   </figure>

Video 17 shows foxglove view of me placing the obstacle early on. Using DWB for this and we can see at the end the robot stopped. Near the goal, the ``RotateToGoal`` and ``GoalAlign`` critics kick in. The additional scoring when the robot is trying to start orienting to the final heading, caused it to miss the cycle deadline and the recovery comes in and the loop continues. This is why the robot appears to stop nearing the end because it requires additional orienting and heading towards final pose. This is consistent.

.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls muted style="display: block; margin: auto;">
      <source src="../_static/dwb_far_range_real.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 18: DWB, Far range, Real life</figcaption>
   </figure>

Video 18 shows the live version of video 17.

*-- DWB (Close Range) --*
~~~~~~~~~~~~~~~~~~~~~~~~~

.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls muted style="display: block; margin: auto;">
      <source src="../_static/dwb_close_range.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 19: DWB, Close range, Foxglove Studio</figcaption>
   </figure>

Here in Video 19, we can see it behaves in close object avoidance. Same issue when nearing the end.

.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls muted style="display: block; margin: auto;">
      <source src="../_static/dwb_close_range_real.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 20: DWB, Close range, Real life</figcaption>
   </figure>

Video 20 shows the live version of video 19.

*-- RPP (Far Range) --*
~~~~~~~~~~~~~~~~~~~~~~~

.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls muted style="display: block; margin: auto;">
      <source src="../_static/rpp_far_range.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 21: RPP, Far range, Foxglove Studio</figcaption>
   </figure>

Now with RPP, we can see the stark difference in Video 21. The moment I placed the object when its far away, the new path was calculated and planned almost instantly. RPP just follows that new path. No issue at the end.

.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls muted style="display: block; margin: auto;">
      <source src="../_static/rpp_far_range_real.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 22: RPP, Far range, Real life</figcaption>
   </figure>

Video 22 shows the live version of Video 21.


*-- RPP (Close Range) --*
~~~~~~~~~~~~~~~~~~~~~~~~~

.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls muted style="display: block; margin: auto;">
      <source src="../_static/rpp_close_range.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 23: RPP, Close range, Foxglove Studio</figcaption>
   </figure>

Here in Video 23, we can see a close range quick object avoidance. See Video 24 for better understanding of what's happening.

.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls muted style="display: block; margin: auto;">
      <source src="../_static/rpp_close_range_real.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 24: RPP, Far range, Real life</figcaption>
   </figure>

Video 24 shows the live version of video 23. We can see how the robot is reacting fast to my attempts to block its path.

*-- Stress Test --*
~~~~~~~~~~~~~~~~~~~

.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls muted style="display: block; margin: auto;">
      <source src="../_static/stress_test.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 25: Stress Test</figcaption>
   </figure>

This stress test is my exaggerated attempt of trying to block the robot, but the RPP + NavFn combo is just so much better, at least on the CPU starved RPI4. Watch Video 26 to see my pov.


.. raw:: html

   <figure style="text-align: center;">
   <video width="640" height="360" controls muted style="display: block; margin: auto;">
      <source src="../_static/stress_test_real.mp4" type="video/mp4">
      Your browser does not support the video tag.
   </video>
   <figcaption style="color: gray; margin-top: 0.8em;">Video 26: Stress Test, Real Life</figcaption>
   </figure>

Video 26 is the live version of video 25.


Conclusion
-----------

This journey was surely a long one. I managed to learn so much, from C++, python, to Robotics fundamentals, math algorithms, ROS 2, slam, navigation, obstacle avoidance, electronics, 3d printing, modelling using fusion 360 and computer networks, it was surely a fruitful one indeed. Now I will be focusing on VLA since thats where my FYP and current trend is heading for Modern Robotics. This gave me a glimpse of what the mature age of "old-school" robotics was like, at least for autonomous navigation, obstacle avoidance and localization. I still having much to learn, anyways on to the next chapter!