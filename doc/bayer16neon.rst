Bayer 16-bit NEON Optimizations
===============================

libtiff provides helper routines for packing and unpacking 16-bit Bayer
samples to and from byte-aligned buffers. When built on ARM processors
with NEON support, these routines leverage vector instructions to process
eight pixels per iteration internally. The code automatically falls back
to a portable C implementation when NEON is unavailable.

