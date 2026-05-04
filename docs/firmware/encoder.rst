Encoder
=======

See also: :doc:`main.cpp <main_cpp>`

Purpose
-------

The ``Encoder`` class reads wheel motion using quadrature encoder signals from
pins A and B. It maintains tick counts using GPIO interrupts and provides
methods for retrieving the current tick count and estimated wheel velocity.

Overall Concept
---------------

The quadrature encoder produces two square-wave signals (**Channel A** and
**Channel B**) offset by 90 degrees in phase. As the shaft rotates, rising
and falling edges are generated on each channel. The **relative phase** of A
and B encodes the direction of rotation.

.. list-table::
    :header-rows: 1
    :widths: 30 30 40

    *   - Event
        - Condition
        - Result
    *   - Pin A rises
        - B is LOW (A ≠ B)
        - ``count++`` (forward)
    *   - Pin A rises
        - B is HIGH (A == B)
        - ``count--`` (backward)
    *   - Pin B rises
        - A is HIGH (A == B)
        - ``count++`` (forward)
    *   - Pin B rises
        - A is LOW (A ≠ B)
        - ``count--`` (backward)

The ``count`` variable is declared ``volatile`` because it is modified inside
an Interrupt Service Routine (ISR) and read in the main loop. Without
``volatile``, the compiler may cache ``count`` in a CPU register and never
re-read it from RAM. (more info at the back)

Scenario 1: A Leading B (Forward)
----------------------------------

.. image:: ../_images/encoder_img1.png
    :alt: A leading B
    :width: 500px
    :align: center

- At t=0, A rises while B is still LOW → ``A ≠ B`` → ``count++``
- At t=1, B rises while A is HIGH → ``A == B`` → ``count++``
- Net result: **+2 counts** per A–B rising pair → forward motion.

Scenario 2: B Leading A (Backward)
------------------------------------

.. image:: ../_images/encoder_img2.png
    :alt: B leading A
    :width: 500px
    :align: center

- At t=0, B rises while A is still LOW → ``A ≠ B`` → ``count--``
- At t=1, A rises while B is HIGH → ``A == B`` → ``count--``
- Net result: **−2 counts** per B–A rising pair → backward motion.

.. note::

    The ``count`` accumulates **displacement**, not distance.
    If the tank moves forward 1 m then reverses 1 m, ``count`` returns to zero.
    This is intentional as ``count`` tracks net signed displacement, not total path length.

Dependencies
------------

.. list-table::
    :header-rows: 1
    :widths: 30 70

    *   - Header
        - Purpose
    *   - ``pico/stdlib.h``
        - Standard device initialisation
    *   - ``hardware/gpio.h``
        - GPIO pin control and interrupt configuration
    *   - ``pico/time.h``
        - High-resolution timer ``time_us_64()`` for velocity calculation

Class Structure
---------------

The ``Encoder`` class inherits from the ``Electronics`` base class, sharing
basic ``name`` and ``status`` properties.

**1) Private Constants**

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Constant
     - Meaning
   * - ``PPR = 11``
     - Pulses Per Revolution — HIGH edges per full *motor shaft* revolution
   * - ``PI``
     - 3.14159…

**2) Static Members**

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Member
     - Role
   * - ``instances[2]``
     - Registry of active encoder pointers; lets the static ISR reach each object
   * - ``instance_count``
     - Tracks how many encoders have been constructed

Initialisation
--------------

**Constructor Signature:**

.. code-block:: cpp

    Encoder(std::string name, std::string status,
            uint _pin_a, uint _pin_b,
            float _reduction_ratio = 100.0f,
            float _diameter = 4.7f)

**Step 1 — Physical Constants**

The wheel circumference is calculated from the wheel diameter :math:`d`:

.. math::

    C = \pi \times d

The total counts per output-shaft revolution accounts for the gearbox and
quadrature decoding (both edges of both channels):

.. math::

    \text{CPR}_{\text{output}} = 4 \times \text{PPR} \times \text{reduction ratio}

For this tank (PPR = 11, reduction = 169):

.. math::

    \text{CPR}_{\text{output}} = 4 \times 11 \times 169 = 7{,}436 \text{ counts/rev}

The factor of 4 arises because a quadrature decoder counts **both** rising
and falling edges on **both** channels, giving four detectable events per
raw pulse.

**Step 2 — GPIO Configuration**

- Pins A and B are set as inputs with **pull-up resistors** enabled.
- Pull-ups hold pins at a stable HIGH when no signal is present, preventing false edge detections from floating inputs.

**Step 3 — Interrupt Registration**

The RP2040 enforces a hardware limit: only **one** GPIO IRQ callback function
can be registered per core. A second call to
``gpio_set_irq_enabled_with_callback()`` would silently overwrite the first.

The workaround:

.. code-block:: cpp

    // First encoder — registers the shared callback
    gpio_set_irq_enabled_with_callback(pin_a, EDGE_RISE | EDGE_FALL,
                                        true, &Encoder::irq_handler);

    // Second encoder — enables pins only, reuses existing callback
    gpio_set_irq_enabled(pin_a, EDGE_RISE | EDGE_FALL, true);
    gpio_set_irq_enabled(pin_b, EDGE_RISE | EDGE_FALL, true);

All GPIO edges on the chip route to the single ``irq_handler``, which
inspects the ``gpio`` argument to dispatch to the correct encoder instance.

Interrupt Handling (The ISR)
----------------------------

Routing is split into two stages because a ``static`` function has no
``this`` pointer and cannot directly access instance variables:

.. code-block:: text

    Any GPIO edge changes
            │
            ▼
    irq_handler (static)          ← hardware entry point (gets called out)
    loops instances[], calls
    handle_irq() on each          ← performs handle_irq for all encoders
            │
            ▼
    handle_irq (instance method)  ← quadrature logic, updates count

``handle_irq`` first filters out pins that do not belong to this instance,
then applies the quadrature rule:

.. code-block:: cpp

    if (gpio == pin_a) {
        if (a == b) count--; else count++;
    } else {
        if (a == b) count++; else count--;
    }

Public Methods
--------------

.. cpp:function:: int get_count()

- Returns the current signed tick count.
- Interrupts are disabled for the duration of the read to guarantee an **atomic** snapshot
- the ISR cannot modify ``count`` between the CPU loading the lower and upper bytes of the integer:

.. code-block:: cpp

    uint32_t saved = save_and_disable_interrupts();
    int c = count;   // guaranteed uninterrupted
    restore_interrupts(saved);

``volatile`` ensures the compiler always re-reads ``count`` from RAM
rather than serving a stale cached register value.

.. cpp:function:: float get_vel()

- Calculates and returns the current linear wheel velocity in m/s.

**Algorithm:**

.. math::

    v = \frac{\Delta\text{ticks}}{\text{CPR}_{\text{output}}} \times \frac{C}{\Delta t}

where :math:`C` is the wheel circumference and :math:`\Delta t` is the
elapsed time since the previous call.

Broken into steps:

.. code-block:: text

    1. dt        = time_us_64() - last_time                 (seconds)
    2. Δticks    = current_count - last_count
    3. Δdistance = (Δticks / cpr_output) * circumference    (metres)
    4. velocity  = Δdistance / dt                           (m/s)

An atomic snapshot of ``count`` is taken at step 2 for the same reason as in ``get_count()``.