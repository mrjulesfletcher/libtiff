Bayer 12-bit Vector Optimizations
===============================

libtiff provides helper routines for packing and unpacking 12-bit Bayer
samples to and from byte-aligned buffers.  When built on ARM processors
with NEON support, these routines leverage vector instructions to process
16 pixels per iteration internally.  The code automatically falls back to
a portable C implementation when NEON is unavailable.

On an RK3588 (ARMv8) system running at 2.4 GHz we measured a speedup of
around 6x for packing and 5x for unpacking compared with the scalar
reference implementation when processing large buffers.

SSE4.1 implementations handle the same 16 pixels per iteration and fall
back to SSE2 when unavailable. Running ``bayerbench`` on an Intel Xeon
Platinum 8370C showed about 2.9x faster packing and 1.4x faster
unpacking compared with SSE2.
