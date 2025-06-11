NEON Predictors
===============

libtiff includes optional NEON implementations of the horizontal
differencing and median predictors for 16-bit scanlines.  When built on
ARM processors with NEON support, these routines use vector intrinsics to
process eight pixels per iteration.  The functions fall back to the
portable C versions if NEON is unavailable.

We observed around a 2x speedup on a Cortex-A53 when encoding
4K images using the horizontal differencing predictor.
