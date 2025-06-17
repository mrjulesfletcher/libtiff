Cross Testing NEON and SSE
==========================

The :file:`scripts/cross_simd_test.py` helper automates building libtiff for both
SSE4 and NEON and runs a couple of representative tests under each build.  The
NEON binaries are executed under ``qemu-aarch64`` so the script can be run on an
x86 host without real ARM hardware.

Running the script requires the ``qemu-user`` package and the AArch64 cross
compiler suite::

    sudo apt-get install qemu-user gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

Then simply execute::

    python3 scripts/cross_simd_test.py

It will build two temporary directories, ``build_sse`` and ``build_neon``,
compile ``bayerbench`` and ``dng_simd_compare`` in each, run them, and print a
short summary comparing the NEON and SSE performance figures.
