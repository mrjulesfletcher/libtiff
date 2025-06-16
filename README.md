# LibTIFF SIMD Fork

This repository extends **libtiff** with optimized implementations for ARM NEON and x86 SSE4.1.  Additional helpers simplify working with 12‑bit data and assembling in-memory TIFF/DNG strips.  The fork stays API compatible with upstream while delivering substantial speedups on supported CPUs.

## Building

### Dependencies
- CMake 3.10 or later (or GNU Autotools)
- A C compiler such as GCC >=7 or Clang >=6
- Optional: `ninja` or `make` for building

### CMake
```bash
$ mkdir build-release && cd build-release
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ cmake --build . -j$(nproc)
$ ctest
$ cmake --install . --prefix /usr/local
```
SIMD support is detected and selected automatically at runtime.  You may explicitly control it or
override the NEON compiler flags:
```bash
$ cmake -DHAVE_NEON=1 -DHAVE_SSE41=0 ..   # force NEON only
$ cmake -DHAVE_SSE41=1 ..                 # enable SSE4.1
$ cmake -DTIFF_NEON_FLAGS="-march=armv8-a+simd" ..  # custom NEON flags
```
Cross-compiling examples:
```bash
# Raspberry Pi 5 (AArch64 with NEON)
$ cmake -DCMAKE_TOOLCHAIN_FILE=toolchains/rpi5.cmake ..
# Generic AArch64 target
$ cmake -DCMAKE_TOOLCHAIN_FILE=toolchains/aarch64.cmake ..
# Target x86_64 with SSE4.1
$ cmake -DCMAKE_TOOLCHAIN_FILE=toolchains/x86_64.cmake \
        -DCMAKE_C_FLAGS="-msse4.1" -DHAVE_SSE41=1 ..
```

#### Cross-compiling for ARM

Install an AArch64 cross compiler such as `gcc-aarch64-linux-gnu` and use the
`toolchains/aarch64.cmake` or `toolchains/rpi5.cmake` files when building on an
x86 host:

```bash
$ cmake -DCMAKE_TOOLCHAIN_FILE=toolchains/aarch64.cmake ..
```

With Autotools pass the `--host=aarch64-linux-gnu` triplet and appropriate
flags:

```bash
$ ./configure --host=aarch64-linux-gnu CFLAGS="-march=armv8-a+simd" --enable-shared
```


### libdeflate Support
Install libdeflate and enable the option when configuring:
```bash
$ cmake -Dlibdeflate=ON -DCMAKE_BUILD_TYPE=Release ..
$ cmake --build . -j$(nproc)
```
Cross-compiling for Raspberry Pi 5 with libdeflate:
```bash
$ cmake -DCMAKE_TOOLCHAIN_FILE=toolchains/rpi5.cmake \
        -Dlibdeflate=ON -DCMAKE_BUILD_TYPE=Release ..
```
Benchmarking a 10MB image gave about 2x faster compression with `zip:p9:s1` (libdeflate) versus `zip:p9:s0` (zlib).


### Autotools
```bash
$ ./autogen.sh       # when building from git
$ ./configure CFLAGS="-msse4.1" --enable-shared
$ make -j$(nproc)
$ make check
$ make install DESTDIR=/usr/local
```
Use `--disable-sse41` or `--disable-neon` to disable the respective optimizations.
Cross-compiling for NEON or SSE4.1 requires setting appropriate host/CC flags, for example:
```bash
# Cross-build for AArch64 with NEON
$ ./configure --host=aarch64-linux-gnu CFLAGS="-march=armv8-a+simd" --enable-shared
# Cross-build for x86_64 with SSE4.1
$ ./configure --host=x86_64-linux-gnu CFLAGS="-msse4.1" --enable-shared
```

## New Features

### NEON 12‑bit Bayer Packing
The functions `TIFFPackRaw12()` and `TIFFUnpackRaw12()` leverage NEON to process 16 pixels per iteration and fall back to scalar code when NEON is unavailable.  On an RK3588 (ARMv8) system the NEON path provides about **6×** packing and **5×** unpacking speedups【F:doc/bayer12neon.rst†L1-L12】.
The companion routines `TIFFPackRaw16()` and `TIFFUnpackRaw16()` operate on
16-bit samples with a similar automatic fallback【F:doc/bayer16neon.rst†L1-L8】.

### NEON Byte Swapping
Byte-swapping routines accelerate endian conversion with vector instructions:
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
Horizontal differencing used by the predictor tag includes NEON implementations for 8/16/32/64‑bit samples.  When `stride == 1` each loop handles 16 values:
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

### NEON PackBits and Fax3 Speedups
PackBits decoding now uses NEON for both run replication and literal copies.
Fax3 fill runs are also vectorized, providing snappier handling of bilevel
images.

### SSE2 Predictor Optimization
Float32 predictor decoding on x86‑64 interleaves four vectors at once and now prefetches upcoming data. Horizontal predictor functions use SSE4.1 when `TIFFUseSSE41()` is enabled at runtime with an SSE2 fallback. This yields about a **25 %** improvement:
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

### SIMD TIFF Strip Assembly
`TIFFAssembleStripSIMD()` assembles a strip from a 16‑bit buffer and optionally applies the predictor before packing samples to 12‑bit form.  It selects a NEON or SSE4.1 backend at runtime:
```c
uint8_t *TIFFAssembleStripSIMD(TIFF *tif, const uint16_t *src, uint32_t width,
                               uint32_t height, int apply_predictor,
                               int bigendian, size_t *out_size,
                               uint16_t *scratch, uint8_t *out_buf);
```
【F:libtiff/strip_simd.h†L1-L19】
The vector implementations compute differences and pack data efficiently【F:libtiff/tif_strip_neon.c†L1-L63】【F:libtiff/tif_strip_sse41.c†L1-L63】.

### ZIP NEON Decompression
When libdeflate is unavailable the Deflate codec now accelerates small memcpy
and CRC loops with ARM NEON. The path works with multithreaded decoding and
provides around a **20 %** speedup on an RK3588 using zlib 1.3.

### NEON YCbCr Conversion
`TIFFReadRGBAImage()` uses a vectorized path for YCbCr 4:4:4 data. 16 pixels are
converted per iteration, significantly reducing the cost of converting camera
captures to RGBA.

### NEON RGB Packing
The helpers `TIFFPackRGB24`, `TIFFPackRGBA32`, `TIFFPackRGB48` and
`TIFFPackRGBA64` assemble ABGR words from 8‑ or 16‑bit planar channels. They are
exposed in `rgb_neon.h` and called internally by the RGBA reader.

### SIMD Memory Helpers
`tiff_simd.h` defines `tiff_memcpy_u8`, `tiff_memmove_u8`, `tiff_memset_u8` and
`tiff_crc32` which use NEON on ARM. Compression codecs and internal routines now
rely on these for faster memory access.

### SIMD Abstraction Header
`libtiff/tiff_simd.h` exposes a small vector API that maps to NEON, SSE4.1 or plain C:
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
A placeholder codec integrates with CharLS when available. Configure with `-Djpegls=ON` or `--with-jpegls` to experiment.

### Lazy Strile Loading
`TIFFOpen()` accepts `'D'` (defer) and `'O'` (on‑demand) mode flags. Append
them to the mode string like `"rD"` or `"rO"`. `'D'` postpones loading the
`StripOffsets`/`StripByteCounts` or `TileOffsets`/`TileByteCounts` arrays until
first use. `'O'` implies `'D'` and only fetches the requested strip or tile
entry. These flags enable lazy metadata access and can significantly reduce
startup time when files reside on slow storage.

See [doc/functions/TIFFStrileQuery.rst](doc/functions/TIFFStrileQuery.rst) for
details on querying per‑strile information.

### Directory Offset Caching
Directory offsets are cached in memory to speed up `TIFFSetDirectory()` and
avoid expensive scans through large files. The cache is invalidated on write so
that applications always see consistent state.

## How to Use SIMD Routines
Below is a short example that assembles a 12‑bit strip and writes a DNG.
The full program is [`test/assemble_strip_neon_test.c`](test/assemble_strip_neon_test.c).
```c
#include "tiffio.h"
#include "strip_neon.h"

uint32_t width = 64, height = 32;
uint16_t *buf = malloc(width * height * sizeof(uint16_t));
/* fill buf */
size_t strip_size = ((width * height + 1) / 2) * 3;
uint16_t *tmp = malloc(width * height * sizeof(uint16_t));
uint8_t *strip = malloc(strip_size);
TIFFAssembleStripSIMD(NULL, buf, width, height, 1, 1, &strip_size, tmp, strip);

TIFF *tif = TIFFOpen("out.dng", "w");
TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 12);
TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, height);
TIFFWriteRawStrip(tif, 0, strip, strip_size);
TIFFClose(tif);
free(strip);
free(tmp);
free(buf);
```

When asynchronous I/O is enabled you may queue each strip using
`_tiffUringWriteProc` while preparing the next one. After the thread pool
finishes assembling strips call `_tiffUringWait` to flush outstanding writes:

```c
_tiffUringSetAsync(tif, 1);
size_t buf_sz = ((w * rows + 1) / 2) * 3;
uint16_t *tmp = malloc(w * rows * sizeof(uint16_t));
uint8_t *s = malloc(buf_sz);
for (...) {
    TIFFAssembleStripSIMD(tif, src, w, rows, 1, 1, &sz, tmp, s);
    _tiffUringWriteProc((thandle_t)TIFFFileno(tif), s, (tmsize_t)sz);
}
_TIFFThreadPoolWait(pool);
_tiffUringWait(tif);
free(s);
free(tmp);
```

See [doc/async_dng.rst](doc/async_dng.rst) for a detailed overview of
asynchronous DNG creation and tuning options.

Packing helpers for raw Bayer buffers:
```c
TIFFPackRaw12(src16, packed, count, 0);
TIFFUnpackRaw12(packed, dst16, count, 0);
```
Include `tiff_simd.h` to use vector types that map to NEON, SSE4.1 or scalar C automatically:
```c
#include "tiff_simd.h"
tiff_v16u8 a = tiff_loadu_u8(ptr_a);
tiff_v16u8 b = tiff_loadu_u8(ptr_b);
tiff_storeu_u8(ptr_out, tiff_add_u8(a, b));
```

## SIMD Feature Summary
| Feature | API | Platforms | Typical Speedup |
|---------|-----|-----------|-----------------|
|12‑bit Bayer pack/unpack|`TIFFPackRaw12`, `TIFFUnpackRaw12`|ARM NEON|6× pack, 5× unpack|
|Byte swapping|`TIFFSwabArrayOfShort`, `TIFFSwabArrayOfLong8`|ARM NEON|~3×¹|
|Predictor acceleration|`PredictorDecodeRow`|ARM NEON / SSE4.1 (SSE2 fallback)|up to 25 %|
|Strip assembly|`TIFFAssembleStripSIMD`|ARM NEON / SSE4.1|>5× pack|
|RGB packing|`TIFFPackRGB24` etc.|ARM NEON|>2×|
|YCbCr to RGBA|`TIFFReadRGBAImage`|ARM NEON|~2×|
|SIMD abstraction|`tiff_v16u8` etc.|NEON / SSE4.1 / scalar|N/A|

¹Measured with `swab_benchmark` on an RK3588.

## Benchmarks
Run the provided utilities to reproduce the numbers below.  These were measured
on an Intel Xeon Platinum 8370C with GCC and `-msse4.1`.
```bash
$ ./tools/bayerbench 50
pack:   2177.78 MPix/s
unpack: 1661.60 MPix/s
# with SSE2 only
pack:   757.83 MPix/s
unpack: 1188.53 MPix/s

$ ./test/swab_benchmark
TIFFSwabArrayOfShort: 0.011 ms
scalar_swab_short:    0.004 ms
TIFFSwabArrayOfLong:  0.016 ms
scalar_swab_long:     0.014 ms
TIFFSwabArrayOfLong8: 0.028 ms
scalar_swab_long8:    0.025 ms
```
ARM NEON builds on an RK3588 at 2.4 GHz show roughly 6× improvements for
`TIFFPackRaw12` and 5× for `TIFFUnpackRaw12`. `TIFFSwabArrayOfLong8` is
around 3× faster than the scalar implementation on the same device.

## Thread Pool Usage

Build with `-Dthreadpool=ON` (CMake) or `--enable-multithreading` (Autotools)
to enable the internal worker pool. By default the number of threads matches
the available processors. Override this with the `TIFF_THREAD_COUNT`
environment variable (set to an integer or `auto`) or by calling
`TIFFSetThreadCount()`. The task queue limit may be changed via
`TIFFSetThreadPoolSize()`. Passing `0` to `TIFFSetThreadPoolSize()` sizes the
pool automatically based on the queued tasks. Strip and tile decoding both
submit work items to this pool when more than one thread is available.

## io_uring Configuration

io_uring support requires Linux 5.1 and liburing. Enable it with
`-Dio-uring=ON` (CMake) or `--enable-io-uring` (Autotools). When unavailable,
the library falls back to a portable thread-based helper that shares the same
API. The queue depth defaults to 8 but may be tuned through the
`TIFF_URING_DEPTH` environment variable, via
`TIFFOpenOptionsSetURingQueueDepth()` before opening a file, or at runtime with
`TIFFSetURingQueueDepth()`. Any positive value accepted by the kernel is
allowed.

## Memory Mapping Tuning

Mapped file regions are aligned to `sysconf(_SC_PAGESIZE)` and the library
calls `posix_fadvise` and `madvise` with `POSIX_FADV_SEQUENTIAL` and
`MADV_SEQUENTIAL` by default. Applications may adjust the window size or
advice flags with `TIFFSetMapSize()` and `TIFFSetMapAdvice()`.


## Testing and Validation
Configure with testing enabled and run the full suite:
```bash
$ cmake -DBUILD_TESTING=ON ..
$ cmake --build .
$ ctest       # or: make check
```
Additional programs validate SIMD helpers individually:
- `assemble_strip_neon_test` writes and reads a small image to verify strip assembly【F:test/assemble_strip_neon_test.c†L8-L68】.
- `swab_neon_test` checks byte swapping correctness.
- `swab_benchmark` reports timing for NEON vs. scalar swapping.
All SIMD helpers must pass the same tests as the scalar implementations.

## Contributing
See [CONTRIBUTING.md](CONTRIBUTING.md) for details on code style and workflow.  The project uses `clang-format` and `pre-commit` hooks to enforce formatting【F:CONTRIBUTING.md†L1-L13】.  When adding SIMD code:
1. Provide a scalar fallback for every routine.
2. Ensure the code builds on both ARM (NEON) and x86 (SSE4.1).
3. Add regression tests and benchmark programs under `test/` or `tools/`.
4. Document new code paths and follow the existing naming conventions.
5. Run `pre-commit` and `ctest` before submitting a pull request.

## FAQ
**Q: CMake fails to detect NEON or SSE.**
Use `-DHAVE_NEON=1` or `-DHAVE_SSE41=1` to force detection. If the compiler
requires custom flags, set them via `-DTIFF_NEON_FLAGS="-march=armv8-a+simd"`.

**Q: How do I enable JPEG‑LS support?**
Install CharLS and configure with `-Djpegls=ON` (CMake) or `--with-jpegls` (Autotools).

**Q: Can I disable SIMD at runtime?**
Yes. SIMD paths are chosen after runtime CPU detection. Use `TIFFSetUseNEON(0)` or `TIFFSetUseSSE41(0)` to disable a vector implementation.

**Q: The program crashes with "illegal instruction". What can I do?**
Your CPU might not support the required SIMD level. Rebuild libtiff with
`-DHAVE_NEON=0` or `-DHAVE_SSE41=0` and run `ctest` to confirm the scalar path.

**Q: How do I know which implementation is active?**
Compile your program including `tiff_simd.h` and check the macros
`TIFF_SIMD_NEON` and `TIFF_SIMD_SSE41`. They are set to `1` when that code path
was compiled in.

## Fallback Behaviour
SIMD code is compiled when `HAVE_NEON` or `HAVE_SSE41` are enabled.
The library checks CPU capabilities at startup and uses the best available
implementation. Call `TIFFUseNEON()` or `TIFFUseSSE41()` to query the active
path and `TIFFSetUseNEON(0)` / `TIFFSetUseSSE41(0)` to force scalar routines.
Including `tiff_simd.h` exposes `TIFF_SIMD_ENABLED`, `TIFF_SIMD_NEON` and
`TIFF_SIMD_SSE41` so applications can log which path was compiled.

## License
This fork inherits the original [libtiff license](LICENSE.md).
