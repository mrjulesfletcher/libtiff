#!/usr/bin/env python3
"""Report undocumented APIs used in test files.

Usage::
    python3 scripts/test_doc_coverage.py <test.c> [...]

The script scans ``tiffio.h`` for exported function declarations,
collects those used in the provided test sources and checks that an
``.rst`` page exists under ``doc/functions`` for each. It exits with a
non‑zero status if any used API lacks documentation.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path


def exported_api_names() -> set[str]:
    """Return names of exported functions declared in ``tiffio.h``."""
    header = Path("libtiff") / "tiffio.h"
    text = header.read_text(errors="ignore")
    pattern = re.compile(r"\bextern\b[^\n]*?\b(\w+)\s*\(")
    return {m.group(1) for m in pattern.finditer(text) if not m.group(1).startswith("_")}


def documented_api_names() -> set[str]:
    """Return names documented under ``doc/functions``."""
    return {p.stem for p in Path("doc/functions").glob("*.rst")}


def used_api_names(files: list[Path], exported: set[str]) -> set[str]:
    """Return exported API names used in the given files."""
    if not exported:
        return set()
    pattern = re.compile(r"\b(" + "|".join(map(re.escape, sorted(exported))) + r")\b")
    names: set[str] = set()
    for path in files:
        text = path.read_text(errors="ignore")
        names.update(pattern.findall(text))
    return names


def main(argv: list[str]) -> int:
    if not argv:
        print("Usage: test_doc_coverage.py <test.c> [...]", file=sys.stderr)
        return 2

    test_files = [Path(a) for a in argv]
    exported = exported_api_names()
    documented = documented_api_names()
    used = used_api_names(test_files, exported)
    missing = sorted(used - documented)

    if missing:
        print("Missing documentation for:")
        for name in missing:
            print(f"  {name}")
        return 1

    print("All used APIs are documented.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
