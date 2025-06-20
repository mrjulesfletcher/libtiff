Thread Pool Formal Specification
================================

The file :file:`threadpool_spec.tla` contains a simplified TLA+ specification of
``libtiff``'s internal thread pool.  The model captures task submission and
execution with a bounded queue.  An invariant checks that the queue does not
exceed ``MaxQueue``.

To explore the model, install the `TLA\+ tools <https://github.com/tlaplus/tlaplus/releases>`_
locally and run ``tlc``::

  tlc threadpool_spec.tla -config threadpool_spec.cfg

The ``TLC`` model checker will verify that the invariant holds for all
reachable states.
