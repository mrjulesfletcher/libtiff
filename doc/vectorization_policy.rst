Vectorization Policy
====================

LibTIFF aims to keep performance‑critical loops vectorized on all supported
architectures. Continuous integration runs ``scripts/vectorization_audit.py``
which builds the library with ``clang`` and reports any loops missed by the
vectorizer.

Policy
------

* CI fails when ``vectorization_audit.py`` reports missed vectorization in
  ``libtiff`` or ``tools`` sources.
* Loops that cannot be vectorized must be annotated with
  ``#pragma clang loop vectorize(disable)`` and a brief justification.
* Refactor hot loops by hoisting invariants, simplifying bounds, and keeping
  memory contiguous. The audit script uses ``-Rpass`` flags so developers can
  spot missed opportunities locally.

