Reflections
================

See also: :doc:`ROS 2 Notes <ros2>`

A personal log of difficulties faced, solutions found, and lessons learned
while building the autonomous tank.

Background
----------

Past 2 years of Uni (Y1 & Y2) was just studying a lot of math and physics subjects. I studied really hard (was still adjusting from NS to Y1), took exams and then called it a day. But in Y3S2 (during my internship at ST Engineering), I realised that the modules taken in Y1-2 did not exactly prepare me enough for what I might face when I actually become an engineer. Therefore, I needed to take matters in my own hands and actually learn meaningful things that will help me in expanding my knowledge pool and also in my future career. 

I knew I wanted to do something related to my initial specialization, Robotics & Automation (now changed to ML). That's why I thought about a robotic car.

However, after dwelving into agentic AI quite a bit, I wanted to incorporate AI into my project. That's where Computer Vision comes in (I know agents and CV is quite different...). However, this project is a good precursor to my FYP which is incoporating Vision Language Action in robotic systems.

I believe understanding the fundamentals of Ultrasonic Sensors, LIDARS, SLAM, ROS2, Nav2, will certainly give me an edge in robotics, especially when I would want to do something related to my FYP for my future job (AI + Robotics).

Therefore, I decided to make Self-driving Robotic Car. Since, I extended my internship such that it ends just before Y4S1 starts, I decided that that should be my deadline. With all that yapping, lets move on with the timelines!

----

26 Feb 26 — Start of Project
------------------------------

After researching and consulting AI what I should purchase, I spend nearly a crazy ~$200 out of my own pocket to purchase the items that included Rasberry Pi 4, Pico W, Pi Camera Module 2, YDLidar X3 Pro, basic arduino car kit (included with motors), H bridge, 18560 batteries.

.. figure:: ../_images/reflections_img1.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 1: Before building version 1

When the kit in Figure 1 came, I was eager to build it. I followed some YouTube Tutorial on it and started building. I learnt how to write a simple micropython script and then make it such that it recieves the velocity commands over USB to my RPI4. I avoided touching ROS2 to reduce complexity and ensure hardware, wiring is also correct.

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

12 Mar 26 — 1st Real Difficulty
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

26 Mar 26 — Improved Chassis, or is it?
-----------------------------------------

When I fitted the my bigger powerbank, everything changed. The whole system got heavier, and my cheap TT motors are struggling to move the car. When moving forward or backwards, it worked fine. But when it was tank turning left or right, it didn't budge. This is because the TT motors are not producing enough torque force to overcome the static friction.


.. figure:: ../_images/reflections_img5-.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 5: New chassis & motor

From Figure 5, we can see that I am using the TP-101 chassis with 2 provided 33GB-520 motors.

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

9 Apr 26 — Camera problems
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

9 Apr 26 — Lack of Encoder & Motor Driver Upgrade
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

16 Apr 26 — Motor Mismatch + LIDAR's custom fit
-------------------------------------------------

When the motor came, I eagerly tried to yoink it in place. But to my surprise, it didn't fit. I checked the shaft dimensions before purchasing, but turned out the old motor I was using had a different sizing from the spec sheet found online.

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

27 Apr 26 — Acrylic
-----------------------

For acrlyic, my main goal is just to drill holes so that I can fit my LIDAR. 

.. figure:: ../_images/reflections_img9-.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 9: 3D model of chassis

Figure 9 shows how it looked like.

20 Apr 26 — 3D Prints
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

26 Apr 26 — How I learn coding/ROS 2
--------------------------------------
.. figure:: ../_images/reflections_img_holy_trinity.png
   :alt: Before building
   :width: 400px
   :align: center

   ~ holy trinity ~

Throughout the whole process while waiting for shipment to arrive (usually takes a week or two), I would take this time to brush up on my python, learn my C++, and also learn ROS from scratch. I found it a very big challenge at the beginning because this is all completely new to me.

My process was as such. First I used AI to teach me the relevant concepts. I will then ask it to generate some questions/practice for me to do. After that, it will check my code and also my output and give me my score. If I still don't feel confident about that particular exercise, I will ask it generate another one. Only when I understand at least 80% of it, I will ask it to teach me another concept. The exercise portion is so that I can learn actively instead of passively.

Also, I chose python for my codebase for RPI4 because its easier. When the full pipeline works, I might explore C++.

27 Apr 26 — Minor Upgrades: Ultrasonic Sensors and Electrical Schematics
-------------------------------------------------------------------------

I decided to get some new ultrasonic sensors and then decided to plan out the schematics and pin allocations. 

.. figure:: ../_images/reflections_img12-.png
   :alt: Before building
   :width: 1000px
   :align: center

   Figure 12: Finished Hardware

Figure 12 shows the schematics that I have created using Fritzing (an open-soruce application). It shows my main electronics used for connections. Since this is just prototyping, I am using a half sized breadboard and jumper wires for my wiring.

28 Apr 26 — Finished Hardware
-------------------------------

.. figure:: ../_images/reflections_img13.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 13: Finished Hardware

Figure 13 shows my completed hardware. At this point, I already have my main.cpp for my micro-ROS ready to be flashed onto my 

29 Apr 26 — E-Stop Function using Ultrasonic Sensors
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


5 May 2026 — PID Controller
-----------------------------
.. figure:: ../_images/reflections_img14.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 13: Finished Hardware

I decided to first brush up on my concept of PID controller. Having studied this in theory and now using it irl, it was a step up. I knew that i should first begin with P, and gradually add in I and D and tune in base on how the car reacts.

When I calculate the PID values, I had to see the maximum error possible. Assuming maximum error, I can calculate the maximum PID output, and ensure that it doesn't go out of the maximum allowed value, i.e. saturation. Besides having a PID controller, we have a feedforward path that provides 80% of the motion, with PID just affect 20% of the final output by making tiny adjustments to the pwm.

I also added a slew rate that allows for smoother acceleration and deceleration.

6 May 2026 — Disaster #2
-----------------------------
.. figure:: ../_images/reflections_img15.png
   :alt: Before building
   :width: 500px
   :align: center

   Figure 13: Finished Hardware

PID controller works fine, but while moving the car, the coupler **snapped once again**. Since this entire project is self-funded and I have blasted a hole through my wallet, I had to save future costs of remaking and repurchasing this coupler and motor. Therefore, I decided to just get a full tank kit with correctly sized motors with encoders to ensure reliability. This way I can focus on what matters more, which is my SLAM and higher level algorithms. Though, what I 3D designed, the chassis and coupler, I wouldn't say its a waste of time since I did learn quite abit from this.

