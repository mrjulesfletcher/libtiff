#!/usr/bin/env python3
"""Run FlawFinder on repository C++ sources.

Usage::
    python3 scripts/run_flawfinder.py [output-file]

The script collects ``.cpp`` and ``.cxx`` files tracked by git, runs
``flawfinder`` on them, and writes the report to ``flawfinder.log`` by
default.
"""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def list_cpp_files() -> list[str]:
    result = subprocess.run(
        ["git", "ls-files", "*.cpp", "*.cxx"],
        cwd=ROOT,
        stdout=subprocess.PIPE,
        text=True,
        check=True,
    )
    return result.stdout.split()


def main(argv: list[str]) -> int:
    output = Path(argv[1]) if len(argv) > 1 else Path("flawfinder.log")
    files = list_cpp_files()
    if not files:
        print("No C++ files found.")
        return 0

    with output.open("w") as fh:
        subprocess.run(["flawfinder", *files], cwd=ROOT, text=True, stdout=fh)
    print(f"Wrote flawfinder report to {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
