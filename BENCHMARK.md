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
