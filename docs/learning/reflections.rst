Reflections
===========

See also: :doc:`ROS 2 Notes <ros2>`

A personal log of difficulties faced, solutions found, and lessons learned
while building the autonomous tank.

Brutally Honest, Unflitered Background
------------------------------------------------

I started this project because I realised how cooked I was with regards to my future career.

Past 2 years of Uni (Y1 & Y2) was just studying a lot of math and physics subjects. I studied really hard (was still adjusting from NS to Y1), took exams and then called it a day. But in Y3S2 (during my internship at ST Engineering), I realised that the modules taken in Y1-2 did not exactly prepare me enough for what I might face when I actually become an engineer.

This was when I decided to take on this project. I knew I wanted to do something related to my initial specialization, Robotics & Automation (now changed to ML). That's why I thought about a robotic car.

However, after dwelving into agentic AI quite a bit, I wanted to incorporate AI into my project. That's where Computer Vision comes in (I know agents and CV is quite different...). This project is also a very good precursor to my FYP which is incoporating (Vision Transformers + Large Language Models = Vision Language Model) + Action --->  Vision Language Action in robotic systems.

I believe understanding the fundamentals of Ultrasonic Sensors, LIDARS, SLAM, ROS2, Nav2, will certainly give me an edge, especially when I would want to do something related to my FYP for my future job (AI + Robotics).

Therefore, I decided to make Self-driving Robotic Car. Since, I extended my internship such that it ends just before Y4S1 starts, I decided that that should be my deadline. With all that yapping, lets move on with the timelines!

----

26 Feb 26 — Start of Project
------------------------------

**What I did**

After consulting AI what I should purchase, I spend nearly a crazy $200 out of my own pocket to purchase the items, that included Rasberry Pi 4, Pico W, basic arduino car kit (included with motors), H bridge, 18560 batteries. 

**Difficulty faced**

Describe the problem clearly — what broke, what confused you, what didn't work.

**My solution**

How you solved it. Be specific — commands, code snippets, or reasoning.

.. code-block:: python

   # optional: paste a relevant snippet if useful
   example_code = True

**Key takeaway**

One sentence on what you'd do differently or what you now understand better.

**Media**

.. figure:: /_images/reflections_img1.HEIC
   :alt: Description of image
   :width: 80%

   Caption for the image.

.. raw:: html

   <video width="640" height="360" controls>
     <source src="../_static/videos/your-video.mp4" type="video/mp4">
     Your browser does not support the video tag.
   </video>

----


5 May 2026 — System Identification
--------------------------------
- main idea is to get max velocity during full load operation
- run the car over fixed distance
