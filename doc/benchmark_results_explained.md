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
```

The absolute values vary with compiler flags and hardware but provide a
sense of relative performance between scalar implementations and their
SIMD or multi-threaded counterparts.

Additional tools exercised by the script verify NEON and SSE4.1 helpers
such as `assemble_strip_neon_test`, `rgb_pack_neon_test` and
`predictor_sse41_test`.  These programs do not print timings but return
zero on success which is reported as `status: ok` in the summary.
