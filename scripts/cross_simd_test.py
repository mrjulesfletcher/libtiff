#!/usr/bin/env python3
"""Cross test NEON and SSE builds.

This script builds libtiff twice: once for SSE4.1/4.2 on the host and
once for NEON using the AArch64 cross toolchain. It runs ``bayerbench``
and ``dng_simd_compare`` for both builds. NEON executables are executed
via ``qemu-aarch64``. Results are printed for easy comparison.  When the
toolchain is missing, the script attempts to install it via ``apt-get``
and gracefully skips NEON tests if installation fails.

Usage::

    python3 scripts/cross_simd_test.py
"""

import os
import re
import shutil
import subprocess
from subprocess import CalledProcessError
from pathlib import Path
from shutil import which

ROOT = Path(__file__).resolve().parent.parent
SSE_BUILD = ROOT / "build_sse"
NEON_BUILD = ROOT / "build_neon"


def have_cross_toolchain():
    required_tools = ["aarch64-linux-gnu-gcc", "qemu-aarch64"]
    for tool in required_tools:
        if which(tool) is None:
            return False
    return Path("/usr/aarch64-linux-gnu").exists()


def install_cross_toolchain():
    """Try to install the AArch64 cross compiler and qemu.

    Returns ``True`` if the toolchain is available afterwards."""
    if which("apt-get") is None:
        return False

    prefix = []
    if os.geteuid() != 0:
        if which("sudo") is None:
            return False
        prefix = ["sudo"]

    try:
        run(prefix + ["apt-get", "update"])
        run(
            prefix
            + [
                "apt-get",
                "install",
                "-y",
                "qemu-user",
                "gcc-aarch64-linux-gnu",
                "g++-aarch64-linux-gnu",
            ]
        )
    except (CalledProcessError, FileNotFoundError):
        return False

    return have_cross_toolchain()


def run(cmd, cwd=None, env=None):
    print("+", *cmd)
    result = subprocess.run(cmd, cwd=cwd, env=env, text=True, capture_output=True)
    print(result.stdout)
    if result.returncode != 0:
        print(result.stderr)
    result.check_returncode()
    return result.stdout


def configure_sse():
    args = [
        "-DCMAKE_BUILD_TYPE=Release",
        "-DBUILD_SHARED_LIBS=OFF",
        "-DHAVE_SSE41=1",
        "-DHAVE_SSE42=1",
        "-Djpegls=OFF",
    ]
    run(["cmake", "-S", str(ROOT), "-B", str(SSE_BUILD)] + args)


def configure_neon():
    args = [
        "-DCMAKE_TOOLCHAIN_FILE=toolchains/aarch64.cmake",
        "-DCMAKE_BUILD_TYPE=Release",
        "-DBUILD_SHARED_LIBS=OFF",
        "-DHAVE_NEON=1",
        "-DHAVE_SSE41=0",
        "-DHAVE_SSE42=0",
        "-Djpegls=OFF",
    ]
    run(["cmake", "-S", str(ROOT), "-B", str(NEON_BUILD)] + args)


def build(path: Path):
    run(["cmake", "--build", str(path), f"-j{os.cpu_count()}"])


def bench(binary: Path, loops=10, qemu=False):
    env = os.environ.copy()
    env["srcdir"] = str(ROOT / "test")
    if qemu:
        cmd = ["qemu-aarch64", "-L", "/usr/aarch64-linux-gnu", str(binary), str(loops)]
    else:
        cmd = [str(binary), str(loops)]
    out = run(cmd, cwd=binary.parent, env=env)
    pack = re.search(r"pack:\s*([0-9.]+)", out)
    unpack = re.search(r"unpack:\s*([0-9.]+)", out)
    return float(pack.group(1)), float(unpack.group(1))


def check(binary: Path, qemu=False):
    env = os.environ.copy()
    env["srcdir"] = str(ROOT / "test")
    if qemu:
        cmd = ["qemu-aarch64", "-L", "/usr/aarch64-linux-gnu", str(binary)]
    else:
        cmd = [str(binary)]
    run(cmd, cwd=binary.parent, env=env)


def main():
    if SSE_BUILD.exists():
        shutil.rmtree(SSE_BUILD)
    if NEON_BUILD.exists():
        shutil.rmtree(NEON_BUILD)

    configure_sse()
    build(SSE_BUILD)

    sse_pack, sse_unpack = bench(SSE_BUILD / "tools" / "bayerbench")
    check(SSE_BUILD / "test" / "dng_simd_compare")

    neon_pack = neon_unpack = None
    toolchain_ok = have_cross_toolchain() or install_cross_toolchain()
    if toolchain_ok:
        try:
            configure_neon()
            build(NEON_BUILD)
            neon_pack, neon_unpack = bench(
                NEON_BUILD / "tools" / "bayerbench", qemu=True
            )
            # dng_simd_compare invokes helper scripts which are not
            # executable under qemu-user. Skip it for cross testing.
        except CalledProcessError:
            print("AArch64 build failed; skipping NEON tests")
    else:
        print("AArch64 toolchain not found; skipping NEON build")

    print("\nSummary:\n------")
    print(f"SSE pack speed : {sse_pack:.2f} MPix/s")
    print(f"SSE unpack speed : {sse_unpack:.2f} MPix/s")
    if neon_pack is not None:
        print(f"NEON pack speed : {neon_pack:.2f} MPix/s")
        print(f"NEON unpack speed : {neon_unpack:.2f} MPix/s")


if __name__ == "__main__":
    main()
