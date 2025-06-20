# Benchmark Results Overview

The `scripts/run_all_benchmarks.py` helper automates building libtiff,
executing the full test-suite via `ctest` and running a wide range of
benchmark and validation programs. The output collected from each tool
gives a quick view of SIMD performance and thread pool scaling.  Example
run::

    $ python3 scripts/run_all_benchmarks.py

At the end of the run a summary table is printed.  Below is a sample
captured on the Codex container:

```
Benchmark summary:
------------------
tools/bayerbench
  pack (MPix/s): 534.73
  unpack (MPix/s): 544.40
test/swab_benchmark
  TIFFSwabArrayOfShort (ms): 0.181
  scalar_swab_short (ms): 0.158
  TIFFSwabArrayOfLong (ms): 0.256
  scalar_swab_long (ms): 0.256
  TIFFSwabArrayOfLong8 (ms): 0.484
  scalar_swab_long8 (ms): 0.445
  TIFFSwabArrayOfDouble (ms): 0.439
  scalar_swab_double (ms): 0.438
test/bayer_simd_benchmark
  pack (ms): 2.00
  unpack (ms): 2.10
test/predictor_threadpool_benchmark
  predictor+pack (ms): 5.86
test/pack_uring_benchmark
  write (ms): 4.23
  read (ms): 4.10
scripts/raw_to_dng_benchmark.py
  1920x1080
    convert_fps: 813.24
  2560x1440
    convert_fps: 705.10
  3840x2160
    convert_fps: 402.45
  5760x3240
    convert_fps: 210.07
```

The absolute values vary with compiler flags and hardware but provide a
sense of relative performance between scalar implementations and their
SIMD or multi-threaded counterparts.

Additional tools exercised by the script verify NEON and SSE4.1 helpers
such as `assemble_strip_neon_test`, `rgb_pack_neon_test`,
`bayer_neon_test`, `gray_flip_neon_test`, `reverse_bits_neon_test`,
`memmove_simd_test`, `ycbcr_neon_test`, `dng_simd_compare` and
`predictor_sse41_test`. These programs do not print timings but return
zero on success which is reported as `status: ok` in the summary.
Tests for the C++ bindings include `tiffstream_api`, while
`tiff_fdopen_async` exercises `TIFFFdOpen` from multiple threads using
`std::async`.

Other utilities assist with performance analysis and documentation:

- `scripts/cross_simd_test.py` cross-compiles for SSE4.1 and NEON and
  runs representative benchmarks under both builds.
- `scripts/vectorization_audit.py` builds with clang and lists loops the
  vectorizer failed to optimize.
- `scripts/test_doc_coverage.py` checks that newly added tests call only
  documented public APIs.
- `scripts/raw_to_dng_benchmark.py` measures raw to DNG conversion speed using raw2tiff.
  The test can be combined with the HVS path by building with `-Ddrm-hvs=ON`
  and setting `TIFFSetUseHVS(1)` in the conversion tool.
