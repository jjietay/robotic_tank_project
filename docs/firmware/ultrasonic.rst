Ultrasonic Sensor (HC-SR04)
===========================

See also: :doc:`main.cpp <main_cpp>`

Purpose
-------

The ``Ultrasonic`` class wraps one HC-SR04-style ultrasonic sensor.
Each sensor instance manages one trigger pin and one echo pin, fires
distance measurements on demand, and stores the result for the main
loop to read.

Physical Overview
-----------------

The HC-SR04 module contains two cylindrical transducers side by side:

- **Transmitter** — converts an electrical pulse into a 40 kHz ultrasonic sound burst (inaudible to humans).
- **Receiver** — listens for that burst after it bounces off an object and converts it back into an electrical signal.

The sensor measures distances from approximately **2 cm to 400 cm**
with an accuracy of ±3 mm.

A simplified layout:

.. code-block:: text

        ┌───────────────────── HC-SR04 Module ─────────────────────┐
        │                                                          │
        │        [ TX ]                             [ RX ]         │
        │          ↑                                 ↑             │
        │       Transmit                            Receive        │
        │         burst                              echo          │
        └──────────────────────────────────────────────────────────┘

            TRIG  ─────►  Fire burst
            ECHO  ◄─────  Pulse width encodes round-trip time

How It Works
------------
**1) Timing diagram:**

.. image:: ../_images/ultrasonic_img1.png
    :alt: A leading B
    :width: 500px
    :align: center

**2) Distance measurement follows four steps:**

1. **Trigger pulse:** the microcontroller pulls the ``TRIG`` pin HIGH for at least 10 µs, then LOW. This tells the sensor to fire.
2. **Burst transmission:** the sensor automatically sends out an 8-cycle burst of 40 kHz sound waves through the transmitter.
3. **Echo listening:** the ``ECHO`` pin goes HIGH immediately after the burst is sent. The sensor waits for the reflected sound to return.
4. **Echo received:** when the reflected burst arrives at the receiver, the ``ECHO`` pin goes LOW. The duration the pin stayed HIGH is the round-trip travel time of the sound.


.. note::

    If no object is detected within range, the sensor times out and pulls ``ECHO`` LOW after approximately 38 ms. The code treats this as a special error value.


Distance Calculation
--------------------

Once the echo pulse duration is measured, distance is calculated using:

.. math::

    d = \frac{t \times v_s}{2}

where:

- :math:`t` is the echo pulse duration in seconds,
- :math:`v_s` is the speed of sound (≈ 346 m/s at 25 °C in this implementation),
- the result is divided by **2** because the sound travels to the object
  *and back*, so the measured time covers twice the actual distance.

In this codebase the result is converted to centimetres:

.. code-block:: cpp

   float dt = (float)(fall - rise) * 1e-6f;           // pulse duration in seconds
   last_distance = (dt * sound_vel / 2.0f) * 100.0f;  // convert metres → centimetres

Implementation Notes
--------------------

**1) Blocking measurement**

The ``update()`` method is **blocking** — it actively waits (busy-loops)
for the echo pin to go HIGH, then waits again for it to go LOW.
Each call can hold the CPU for up to 38 ms if no object is present.

Timeout guards are included for both waits::

   if (time_us_64() - t0 > 38000ULL) { last_distance = -1.0f; return; }  // no echo start
   if (time_us_64() - rise > 38000ULL) { last_distance = -2.0f; return; } // no echo end

Return values:

- ``-1.0`` — echo never started (no object or sensor fault).
- ``-2.0`` — echo started but never ended (object too close or sensor fault).
- Any positive value — valid distance in centimetres.

**2) Staggered firing**

All four sensors are fired once per loop tick using a rotating index
(``usrm_index`` in ``main()``). This avoids *crosstalk*, where one
sensor's burst is accidentally received by a neighbouring sensor's
receiver before it fires its own burst. This ensures only one sensor
is actively transmitting at any given tick.

Visualising staggered firing within the main loop:

.. code-block:: text

   Loop ticks:   0      1      2      3      4      5      ...
                 |      |      |      |      |      |
   Sensor used:  Front  Back   Right  Left   Front  Back   ...


**3) Blocking vs. non-blocking tradeoff**

.. list-table::
    :header-rows: 1
    :widths: 20 40 40

    *   - Approach
        - Advantages
        - Disadvantages
    *   - Blocking (current)
        - Simple, easy to debug, no state machine needed
        - Stalls the CPU for up to 38 ms; limits loop rate
    *   - Non-blocking (IRQ-based)
        - CPU free while waiting; scales to many sensors
        - More complex; requires careful interrupt management

Design intuition
~~~~~~~~~~~~~~~~

.. admonition:: Why blocking is "good enough" here

    The tank runs a simple control loop on the Pico and
    the heavy decision-making happens on the Raspberry Pi.
    A 10 ms loop with four staggered sensors is acceptable,
    and the clarity of a blocking implementation makes it
    easier to debug hardware issues. If you later need
    higher-frequency control or more sensors, you can
    refactor to a non-blocking, interrupt-driven design.

Class Reference
---------------

.. cpp:class:: Ultrasonic : public Electronics

    Wraps a single HC-SR04 ultrasonic sensor.

.. cpp:function:: float update()

    Fires the sensor and returns the measured distance in centimetres.
    Blocks for up to 38 ms. Returns ``-1.0`` on start timeout,
    ``-2.0`` on end timeout.

.. cpp:function:: float get_distance() const

    Returns the most recently stored measurement without firing the
    sensor again.