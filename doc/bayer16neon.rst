Bayer 16-bit Vector Optimizations
===============================

libtiff provides helper routines for packing and unpacking 16-bit Bayer
samples to and from byte-aligned buffers. When built on ARM processors
with NEON support, these routines leverage vector instructions to process
eight pixels per iteration internally. The code automatically falls back
to a portable C implementation when NEON is unavailable.

On x86-64 processors the routines also have SSE4.1 versions which use
the same loop structure with SSE instructions. These paths provide
noticeable gains over the SSE2 fallback.

