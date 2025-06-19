#!/usr/bin/env python3
"""Benchmark raw to DNG conversion using raw2tiff."""

import argparse
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path


def parse_resolution(text: str):
    parts = text.lower().split("x")
    if len(parts) != 2:
        raise argparse.ArgumentTypeError("resolution must be WxH")
    return int(parts[0]), int(parts[1])


def run_raw2tiff(raw2tiff: str, raw: Path, dng: Path, width: int, height: int):
    cmd = [
        raw2tiff,
        "-w",
        str(width),
        "-l",
        str(height),
        "-d",
        "byte",
        "-p",
        "minisblack",
        str(raw),
        str(dng),
    ]
    subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def measure(raw2tiff: str, width: int, height: int, frames: int, outdir: Path):
    raw = outdir / "input.raw"
    with open(raw, "wb") as f:
        f.write(os.urandom(width * height))

    dng = outdir / "output.dng"
    start = time.perf_counter()
    for _ in range(frames):
        run_raw2tiff(raw2tiff, raw, dng, width, height)
    elapsed = time.perf_counter() - start
    fps = frames / elapsed if elapsed else 0.0

    try:
        raw.unlink()
        dng.unlink()
    except FileNotFoundError:
        pass

    return fps


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--raw2tiff", default="raw2tiff", help="path to raw2tiff tool")
    parser.add_argument("--width", type=int, default=640, help="image width")
    parser.add_argument("--height", type=int, default=480, help="image height")
    parser.add_argument(
        "--res",
        action="append",
        type=parse_resolution,
        help="resolution as WxH; can be repeated",
    )
    parser.add_argument("--frames", type=int, default=5, help="number of conversions")
    parser.add_argument(
        "--outdir", type=Path, default=Path("raw_to_dng_bench"), help="output directory"
    )
    args = parser.parse_args()

    if not (shutil.which(args.raw2tiff) or Path(args.raw2tiff).exists()):
        print("raw2tiff tool not available; skipping benchmark.", file=sys.stderr)
        return

    args.outdir.mkdir(exist_ok=True)

    resolutions = args.res or [(args.width, args.height)]
    for w, h in resolutions:
        fps = measure(args.raw2tiff, w, h, args.frames, args.outdir)
        print(f"{w}x{h}")
        print(f"  convert_fps: {fps:.2f}")
        print(f"convert_fps_{w}x{h}: {fps:.2f} fps")


if __name__ == "__main__":
    main()
