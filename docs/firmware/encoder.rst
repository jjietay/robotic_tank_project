Encoder
=======

See also: :doc:`main.cpp <main_cpp>`

Purpose
-------

The ``Encoder`` class reads wheel motion using quadrature encoder signals from
pins A and B. It maintains tick counts using GPIO interrupts and provides
methods for retrieving count and estimated wheel velocity.

Key ideas
---------

- Uses interrupts to react to encoder edge changes.
- Stores counts in a ``volatile`` member because updates happen inside an ISR.
- Supports two encoder instances through a static instance registry.
- Provides atomic count reads by disabling interrupts briefly.

Important methods
-----------------

``get_count()``
    Returns the current encoder tick count safely.

``get_vel()``
    Estimates wheel velocity from count difference over elapsed time.

Implementation notes
--------------------

The class computes output counts based on pulses per revolution and reduction
ratio. This is enough for basic velocity estimation, but it depends heavily on
accurate mechanical constants and good encoder signal quality.

Learning note
-------------

This is a good place to document the difference between:

- raw encoder ticks,
- wheel angular displacement,
- wheel linear displacement,
- estimated wheel velocity.