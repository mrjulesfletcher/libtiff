#!/usr/bin/env python3
import os
import re
import subprocess
import sys
from pathlib import Path

BUILD_DIR = Path('vector_build')

CMAKE_FLAGS = [
    '-DCMAKE_C_COMPILER=clang',
    '-DCMAKE_C_FLAGS=-O2 -Rpass=loop-vectorize -Rpass-missed=loop-vectorize'
]

def run(cmd, cwd=None):
    result = subprocess.run(cmd, cwd=cwd, text=True, capture_output=True)
    return result.stdout + result.stderr, result.returncode

def configure():
    BUILD_DIR.mkdir(exist_ok=True)
    out, code = run(['cmake', '..'] + CMAKE_FLAGS, cwd=BUILD_DIR)
    Path(BUILD_DIR / 'cmake.log').write_text(out)
    return code == 0

def build():
    out, _ = run(['make', '-k', f'-j{os.cpu_count()}'], cwd=BUILD_DIR)
    Path(BUILD_DIR / 'build.log').write_text(out)
    return out

def parse(log):
    pattern = re.compile(r'^(.*?):(\d+):\d+: remark: (.*?)(?: \[-Rpass.*\])')
    missed = []
    for line in log.splitlines():
        m = pattern.match(line)
        if not m:
            continue
        path, ln, msg = m.groups()
        if 'not vectorized' in msg or 'not beneficial' in msg:
            missed.append((path, int(ln), msg))
    return missed

def report(missed):
    if not missed:
        print('All loops vectorized')
        return 0
    print('Missed vectorization opportunities:')
    for path, ln, msg in missed:
        print(f"{path}:{ln}: {msg}")
    print('\nSummary:')
    counts = {}
    for path, _, _ in missed:
        counts[path] = counts.get(path, 0) + 1
    for path, cnt in counts.items():
        print(f"{path}: {cnt} misses")
    return 1

def main():
    if not configure():
        print('CMake configuration failed')
        return 1
    log = build()
    missed = parse(log)
    return report(missed)

if __name__ == '__main__':
    sys.exit(main())
