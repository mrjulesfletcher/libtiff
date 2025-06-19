# Benchmark Environment

The following details capture the system information used when running the pipeline tests.

```
Linux 2b2c5f81b66a 6.12.13 #1 SMP Thu Mar 13 11:34:50 UTC 2025 x86_64 x86_64 x86_64 GNU/Linux
Architecture:                         x86_64
CPU op-mode(s):                       32-bit, 64-bit
Address sizes:                        46 bits physical, 48 bits virtual
Byte Order:                           Little Endian
CPU(s):                               5
On-line CPU(s) list:                  0-4
Vendor ID:                            GenuineIntel
Model name:                           Intel(R) Xeon(R) Platinum 8370C CPU @ 2.80GHz
BIOS Model name:                        CPU @ 0.0GHz
BIOS CPU family:                      0
CPU family:                           6
Model:                                106
Thread(s) per core:                   1
Core(s) per socket:                   5
Socket(s):                            1
Stepping:                             6
BogoMIPS:                             5586.87
Flags:                                fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ss ht syscall nx pdpe1gb rdtscp lm constant_tsc rep_good nopl xtopology cpuid tsc_known_freq pni pclmulqdq ssse3 fma cx16 pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand hypervisor lahf_lm abm 3dnowprefetch cpuid_fault pti fsgsbase tsc_adjust bmi1 hle avx2 smep bmi2 erms invpcid rtm avx512f avx512dq rdseed adx smap clflushopt avx512cd avx512bw avx512vl xsaveopt xsavec xsaves arat umip md_clear arch_capabilities
Hypervisor vendor:                    KVM
Virtualization type:                  full
               total        used        free      shared  buff/cache   available
Mem:           9.9Gi       318Mi       8.8Gi        44Ki       921Mi       9.6Gi
Swap:             0B          0B          0B
               total        used        free      shared  buff/cache   available
Mem:           9.9Gi       318Mi       8.8Gi        44Ki       921Mi       9.6Gi
Swap:             0B          0B          0B
```

Pipeline test command:

```
cmake -Dthreadpool=ON -DBUILD_TESTING=ON ..
cmake --build . -j$(nproc)
ctest -R pipeline-full --output-on-failure
```

The `pipeline-full` test passed successfully with threadpool support enabled.

## Codex Container Environment

The pipeline test was also executed inside the Codex runner. Hardware details
for this container are listed below.

```
Linux 14efed69da2a 6.12.13 #1 SMP Thu Mar 13 11:34:50 UTC 2025 x86_64 x86_64 x86_64 GNU/Linux
Architecture:                         x86_64
CPU op-mode(s):                       32-bit, 64-bit
Address sizes:                        46 bits physical, 48 bits virtual
Byte Order:                           Little Endian
CPU(s):                               5
On-line CPU(s) list:                  0-4
Vendor ID:                            GenuineIntel
Model name:                           Intel(R) Xeon(R) Platinum 8370C CPU @ 2.80GHz
BIOS Model name:                        CPU @ 0.0GHz
BIOS CPU family:                      0
CPU family:                           6
Model:                                106
Thread(s) per core:                   1
Core(s) per socket:                   5
Socket(s):                            1
Stepping:                             6
BogoMIPS:                             5586.87
Flags:                                fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ss ht syscall nx pdpe1gb rdtscp lm constant_tsc rep_good nopl xtopology cpuid tsc_known_freq pni pclmulqdq ssse3 fma cx16 pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand hypervisor lahf_lm abm 3dnowprefetch cpuid_fault pti fsgsbase tsc_adjust bmi1 hle avx2 smep bmi2 erms invpcid rtm avx512f avx512dq rdseed adx smap clflushopt avx512cd avx512bw avx512vl xsaveopt xsavec xsaves arat umip md_clear arch_capabilities
Hypervisor vendor:                    KVM
Virtualization type:                  full
               total        used        free      shared  buff/cache   available
Mem:           9.9Gi       320Mi       8.9Gi        44Ki       876Mi       9.6Gi
Swap:             0B          0B          0B
```

The `pipeline-full` test passed successfully in this environment as well.

## Additional Codex Container Environment

The tests were run again on another Codex container instance. Details of this
machine are listed below.
```
Linux aeb88f1cfc18 6.12.13 #1 SMP Thu Mar 13 11:34:50 UTC 2025 x86_64 x86_64 x86_64 GNU/Linux
Architecture:                         x86_64
CPU op-mode(s):                       32-bit, 64-bit
Address sizes:                        46 bits physical, 48 bits virtual
Byte Order:                           Little Endian
CPU(s):                               5
On-line CPU(s) list:                  0-4
Vendor ID:                            GenuineIntel
Model name:                           Intel(R) Xeon(R) Platinum 8370C CPU @ 2.80GHz
BIOS Model name:                        CPU @ 0.0GHz
BIOS CPU family:                      0
CPU family:                           6
Model:                                106
Thread(s) per core:                   1
Core(s) per socket:                   5
Socket(s):                            1
Stepping:                             6
BogoMIPS:                             5586.87
Flags:                                fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ss ht syscall nx pdpe1gb rdtscp lm constant_tsc rep_good nopl xtopology cpuid tsc_known_freq pni pclmulqdq ssse3 fma cx16 pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand hypervisor lahf_lm abm 3dnowprefetch cpuid_fault pti fsgsbase tsc_adjust bmi1 hle avx2 smep bmi2 erms invpcid rtm avx512f avx512dq rdseed adx smap clflushopt avx512cd avx512bw avx512vl xsaveopt xsavec xsaves arat umip md_clear arch_capabilities
Hypervisor vendor:                    KVM
Virtualization type:                  full
               total        used        free      shared  buff/cache   available
Mem:           9.9Gi       324Mi       8.9Gi        44Ki       876Mi       9.6Gi
Swap:             0B          0B          0B
```

The `pipeline-full` test succeeded on this instance as well.

## SIMD Benchmark Results

Running the `scripts/cross_simd_test.py` helper validated SSE4.1 and
NEON builds. The Codex container produced the following summary:

```
SSE pack speed : 4320.16 MPix/s
SSE unpack speed : 4265.02 MPix/s
NEON pack speed : 756.23 MPix/s
NEON unpack speed : 393.28 MPix/s
```

## Updated Benchmarks

Running the benchmarks with the default threadpool enabled produced the following numbers on the Codex container:

```
$ ./tools/bayerbench 10
pack:   534.73 MPix/s
unpack: 544.40 MPix/s

$ ./test/swab_benchmark
TIFFSwabArrayOfShort: 0.181 ms
scalar_swab_short: 0.158 ms
TIFFSwabArrayOfLong: 0.256 ms
scalar_swab_long: 0.256 ms
TIFFSwabArrayOfLong8: 0.484 ms
scalar_swab_long8: 0.445 ms
TIFFSwabArrayOfDouble: 0.439 ms
scalar_swab_double: 0.438 ms

$ ./test/predictor_threadpool_benchmark 4 10
predictor+pack with 4 threads (10 loops each): 5.86 ms
```

Executing `scripts/cross_simd_test.py` still reports multi-architecture speeds:

```
SSE pack speed : 4190.95 MPix/s
SSE unpack speed : 4219.14 MPix/s
NEON pack speed : 747.12 MPix/s
NEON unpack speed : 382.29 MPix/s
```

## AES Whitening Benchmarks

The Bayer RAW helpers were benchmarked both with and without the new AES-based
whitening step enabled. The measurements were taken on the Codex container using
a small program that calls `TIFFSetUseAES()` after `TIFFInitSIMD()`.

```
$ ./benchaes 50
use_aes=0 pack 664.31 MPix/s, unpack 680.79 MPix/s
use_aes=1 pack 668.09 MPix/s, unpack 681.42 MPix/s
```

Whitening with AES shows roughly comparable throughput to the unwhitened
pipeline while improving compression ratios when using the ZIP codec.

