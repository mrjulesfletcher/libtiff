ZIP NEON Optimizations
======================

When libdeflate is unavailable libtiff falls back to zlib for Deflate
compression.  Small memcpy and CRC32 loops now optionally use ARM NEON
and the ARMv8 CRC instructions.  The routines integrate with the
existing `ZIPDecode` thread-pool path automatically.

On an RK3588 (ARMv8) using zlib 1.3 we measured around a 20% speedup
when decoding a 50 MB image with `TIFF_THREAD_COUNT=8` compared with
the scalar implementation.
