Asynchronous IO and Thread Pool Specification
=============================================

The file :file:`uring_threadpool_spec.tla` models the interaction between
libtiff's thread pool workers and the ``io_uring`` submission/completion loop.
It tracks buffers as they move from the work queue to asynchronous writes and
verifies that a buffer is never simultaneously being processed by a worker and
queued for ``io_uring``.

Install the `TLA+ tools <https://github.com/tlaplus/tlaplus/releases>`_
locally and run ``tlc``::

  tlc uring_threadpool_spec.tla -config uring_threadpool_spec.cfg

``TLC`` will exhaustively check that the invariants in the model hold for all
reachable states.
