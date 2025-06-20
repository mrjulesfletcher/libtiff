#!/usr/bin/env python3
"""Build libtiff with tests and run all benchmark programs.

The script configures the project with CMake, builds it, runs the test
suite via ``ctest`` and finally executes all available benchmark
executables. Output from each benchmark is collected and summarised.

This utility is intended for quick performance sanity checks across the
available SIMD and thread pool helpers. It can be invoked from the
repository root as::

    python3 scripts/run_all_benchmarks.py
"""
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build_bench"


def run(cmd, cwd=None):
    print("+", *cmd)
    result = subprocess.run(cmd, cwd=cwd, text=True, capture_output=True)
    print(result.stdout)
    if result.returncode != 0:
        print(result.stderr)
    result.check_returncode()
    return result.stdout


def configure_and_build():
    if BUILD.exists():
        shutil.rmtree(BUILD)
    run(["cmake", "-S", str(ROOT), "-B", str(BUILD), "-DBUILD_TESTING=ON"])
    run(["cmake", "--build", str(BUILD), f"-j{os.cpu_count()}"])


def run_ctest():
    run(["ctest", "--output-on-failure"], cwd=BUILD)


def run_bench(path, args=None):
    exe = BUILD / path
    if not exe.exists():
        return None
    cmd = [str(exe)]
    if args:
        cmd.extend(args)
    try:
        out = run(cmd, cwd=exe.parent)
    except subprocess.CalledProcessError as e:
        out = e.stdout + e.stderr
    return out


def parse_results(out):
    results = {}
    if out is None:
        return results
    for line in out.splitlines():
        m = re.search(r"([A-Za-z0-9_+x-]+):\s*([0-9.e+-]+)\s*(ms|MPix/s|fps)?", line)
        if m:
            key = m.group(1)
            try:
                val = float(m.group(2))
            except ValueError:
                # Skip entries where the numeric portion isn't actually a
                # number (for example error messages like "TIFFOpen:")
                continue
            unit = m.group(3) or ""
            results[f"{key} ({unit.strip()})"] = val
    if not results:
        out = out.strip()
        if out:
            results["output"] = out
        else:
            results["status"] = "ok"
    return results


def main():
    configure_and_build()
    run_ctest()

    bench_bins = {
        "tools/bayerbench": ["10"],
        "test/swab_benchmark": None,
        "test/bayer_simd_benchmark": None,
        "test/predictor_threadpool_benchmark": ["4", "10"],
        "test/pack_uring_benchmark": None,
        "test/assemble_strip_neon_test": None,
        "test/bayer_neon_test": None,
        "test/bayer_pack_test": None,
        "test/gray_flip_neon_test": None,
        "test/memmove_simd_test": None,
        "test/predictor_sse41_test": None,
        "test/reverse_bits_neon_test": None,
        "test/rgb_pack_neon_test": None,
        "test/ycbcr_neon_test": None,
        "test/dng_simd_compare": None,
    }

    summary = {}
    for rel, args in bench_bins.items():
        print(f"Running {rel}...")
        out = run_bench(rel, args)
        summary[rel] = parse_results(out)

    speed_script = ROOT / "scripts" / "resolution_speed_test.py"
    if speed_script.exists():
        print("Running resolution_speed_test.py...")
        out = run([sys.executable, str(speed_script), "--frames", "2"])
        summary["resolution_speed_test"] = parse_results(out)

    raw_dng_script = ROOT / "scripts" / "raw_to_dng_benchmark.py"
    if raw_dng_script.exists():
        raw2tiff = BUILD / "tools" / "raw2tiff"
        print("Running raw_to_dng_benchmark.py...")
        out = run([
            sys.executable,
            str(raw_dng_script),
            "--raw2tiff",
            str(raw2tiff),
            "--frames",
            "2",
            "--res",
            "1920x1080",
            "--res",
            "2560x1440",
            "--res",
            "3840x2160",
            "--res",
            "5760x3240",
        ])
        summary["raw_to_dng_benchmark"] = parse_results(out)

    print("\nBenchmark summary:\n------------------")
    for prog, res in summary.items():
        print(prog)
        for k, v in res.items():
            print(f"  {k}: {v}")
        if not res:
            print("  (no results)")


if __name__ == "__main__":
    main()
