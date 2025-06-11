# LibTIFF SIMD Fork

This fork of **libtiff** extends the upstream project with optimized code paths for modern SIMD architectures and introduces new helper routines for working with 12‑bit data and TIFF/DNG strips.  It remains fully compatible with existing applications while providing significant speedups on supported hardware.

## Quickstart

```bash
$ mkdir build && cd build
$ cmake ..
$ cmake --build .
$ ctest      # run regression tests
```

By default the build system detects ARM NEON and x86 SSE4.1 support and enables the matching code paths.  Pass `-DHAVE_NEON=0` or `-DHAVE_SSE41=0` to disable the optimizations when cross‑compiling.

## New Features and Enhancements

### NEON 12‑bit Bayer Packing

The functions `TIFFPackRaw12()` and `TIFFUnpackRaw12()` now use NEON when available and fall back to a scalar implementation otherwise.  Processing 16 pixels at a time yields around a 6× packing and 5× unpacking speedup on an RK3588 (ARMv8) system【F:doc/bayer12neon.rst†L1-L12】.

### NEON Byte Swapping

Byte‑swapping helpers accelerate endian conversion of image buffers.  Example implementation:
```c
static void TIFFSwabArrayOfShortNeon(uint16_t *wp, tmsize_t n)
{
    size_t i = 0;
    for (; i + 8 <= (size_t)n; i += 8)
    {
        uint16x8_t v = vld1q_u16(wp + i);
        v = vreinterpretq_u16_u8(vrev16q_u8(vreinterpretq_u8_u16(v)));
        vst1q_u16(wp + i, v);
    }
    if (i < (size_t)n)
        TIFFSwabArrayOfShortScalar(wp + i, n - i);
}
```
【F:libtiff/tif_swab.c†L96-L116】

### NEON Predictor Acceleration

Horizontal differencing used by the predictor tag includes NEON implementations for 8/16/32/64‑bit samples.  When `stride == 1` the optimized loops process 16 values per iteration and fall back to the portable code otherwise:
```c
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    if (stride == 1 && cc > 1)
    {
        uint8_t *p = cp0 + 1;
        tmsize_t remaining = cc - 1;
        while (remaining >= 16)
        {
            uint8x16_t cur = vld1q_u8(p);
            uint8x16_t prev = vld1q_u8(p - 1);
            uint8x16_t diff = vsubq_u8(cur, prev);
            vst1q_u8(p, diff);
            p += 16;
            remaining -= 16;
        }
```
【F:libtiff/tif_predict.c†L1078-L1094】

### SSE2 Predictor Optimization

On x86‑64 systems, Float32 predictor decoding interleaves four vectors at once using SSE2 intrinsics for roughly a 25% speed improvement over the scalar path:
```c
#if defined(__x86_64__) || defined(_M_X64)
    if (bps == 4)
    {
        for (; count + 15 < wc; count += 16)
        {
            __m128i xmm0 = _mm_loadu_si128((const __m128i *)(tmp + count + 3 * wc));
            __m128i xmm1 = _mm_loadu_si128((const __m128i *)(tmp + count + 2 * wc));
            __m128i xmm2 = _mm_loadu_si128((const __m128i *)(tmp + count + 1 * wc));
            __m128i xmm3 = _mm_loadu_si128((const __m128i *)(tmp + count + 0 * wc));
            __m128i tmp0 = _mm_unpacklo_epi8(xmm0, xmm1);
            __m128i tmp1 = _mm_unpackhi_epi8(xmm0, xmm1);
            __m128i tmp2 = _mm_unpacklo_epi8(xmm2, xmm3);
            __m128i tmp3 = _mm_unpackhi_epi8(xmm2, xmm3);
            __m128i tmp2_0 = _mm_unpacklo_epi16(tmp0, tmp2);
            __m128i tmp2_1 = _mm_unpackhi_epi16(tmp0, tmp2);
            __m128i tmp2_2 = _mm_unpacklo_epi16(tmp1, tmp3);
            __m128i tmp2_3 = _mm_unpackhi_epi16(tmp1, tmp3);
            _mm_storeu_si128((__m128i *)(cp + 4 * count + 0 * 16), tmp2_0);
            _mm_storeu_si128((__m128i *)(cp + 4 * count + 1 * 16), tmp2_1);
            _mm_storeu_si128((__m128i *)(cp + 4 * count + 2 * 16), tmp2_2);
            _mm_storeu_si128((__m128i *)(cp + 4 * count + 3 * 16), tmp2_3);
        }
    }
#endif
```
【F:libtiff/tif_predict.c†L596-L626】

### NEON TIFF Strip Assembly

`TIFFAssembleStripNEON()` assembles a TIFF/DNG strip from a 16‑bit buffer and optionally applies the horizontal predictor before packing the samples to 12‑bit form:
```c
uint8_t *TIFFAssembleStripNEON(const uint16_t *src, uint32_t width,
                               uint32_t height, int apply_predictor,
                               int bigendian, size_t *out_size);
```
【F:libtiff/strip_neon.h†L1-L19】

The helper uses NEON in `tif_strip_neon.c` to compute differences and pack data efficiently【F:libtiff/tif_strip_neon.c†L1-L63】.

### SIMD Abstraction Header

`libtiff/tiff_simd.h` exposes a small vector API that maps to NEON, SSE4.1 or plain C depending on the build environment.  Example definitions:
```c
#if defined(HAVE_NEON) && defined(__ARM_NEON)
#include <arm_neon.h>
#define TIFF_SIMD_NEON 1
#define TIFF_SIMD_SSE41 0
#define TIFF_SIMD_ENABLED 1
typedef uint8x16_t tiff_v16u8;
```
【F:libtiff/tiff_simd.h†L11-L18】

### JPEG‑LS Codec Stub

A placeholder codec integrates with CharLS when available.  Enable it with `-Djpegls=ON` to experiment with JPEG‑LS compression.

## Verifying the Optimizations

The regular test suite can be run with `ctest` or `make test`.  Additional checks include `assemble_strip_neon_test`, which validates the NEON strip assembler by writing and reading a sample image【F:test/assemble_strip_neon_test.c†L8-L68】.

## Fallback Behaviour

When SIMD instructions are not detected, all routines transparently use portable C implementations.  The same API is available regardless of the hardware capabilities.

## Contributing NEON/SIMD Code

1. Implement new vector routines using the helpers in `tiff_simd.h`.
2. Always provide a scalar fallback so the library builds on all systems.
3. Add a regression or unit test under `test/`.
4. Run `cmake` and `ctest` to verify that the full suite passes.

## FAQ

**Q: CMake fails to detect NEON or SSE. How do I force it?**

Use `-DHAVE_NEON=1` or `-DHAVE_SSE41=1` when invoking CMake. Cross‑compile toolchains may require additional compiler flags.

**Q: How do I enable JPEG‑LS support?**

Install CharLS and configure with `-Djpegls=ON` (or `--with-jpegls` when using Autotools).

**Q: Tests fail on my platform.**

Ensure your compiler supports the chosen SIMD features and rerun `ctest -V` to obtain verbose logs.

## License

This project inherits libtiff's original license.  Silicon Graphics has allowed this work to be distributed for free with no warranty.  See below for the full text.

```
Copyright (c) 1988-1997 Sam Leffler
Copyright (c) 1991-1997 Silicon Graphics, Inc.

Permission to use, copy, modify, distribute, and sell this software and
its documentation for any purpose is hereby granted without fee, provided
that (i) the above copyright notices and this permission notice appear in
all copies of the software and related documentation, and (ii) the names of
Sam Leffler and Silicon Graphics may not be used in any advertising or
publicity relating to the software without the specific, prior written
permission of Sam Leffler and Silicon Graphics.

THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.

IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
OF THIS SOFTWARE.
```
