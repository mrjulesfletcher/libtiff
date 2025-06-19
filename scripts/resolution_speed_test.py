#!/usr/bin/env python3
"""Measure TIFF encode/decode speed at various resolutions."""

import argparse
import os
import sys
import time
from pathlib import Path

try:
    from PIL import Image  # type: ignore
    HAVE_PIL = True
except Exception:
    HAVE_PIL = False

DEFAULT_RESOLUTIONS = [
    (640, 480),
    (1280, 720),
    (1920, 1080),
]


def parse_resolution(text: str):
    parts = text.lower().split("x")
    if len(parts) != 2:
        raise argparse.ArgumentTypeError("resolution must be WxH")
    return int(parts[0]), int(parts[1])


def generate_image(width: int, height: int) -> "Image.Image":
    pixels = os.urandom(width * height * 3)
    return Image.frombytes("RGB", (width, height), pixels)


def measure_speed(width: int, height: int, frames: int, outdir: Path):
    img = generate_image(width, height)
    path = outdir / f"{width}x{height}.tiff"

    start = time.perf_counter()
    for _ in range(frames):
        img.save(path, format="TIFF")
    write_time = time.perf_counter() - start

    start = time.perf_counter()
    for _ in range(frames):
        Image.open(path).load()
    read_time = time.perf_counter() - start

    fps_write = frames / write_time if write_time else 0.0
    fps_read = frames / read_time if read_time else 0.0
    return fps_write, fps_read


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--res",
        action="append",
        type=parse_resolution,
        help="resolution as WxH; can be repeated",
    )
    parser.add_argument("--frames", type=int, default=5, help="frames per test")
    parser.add_argument(
        "--outdir", type=Path, default=Path("speed_test_images"), help="output directory"
    )
    args = parser.parse_args()

    if not HAVE_PIL:
        print("Pillow not available; skipping speed test.", file=sys.stderr)
        return

    resolutions = args.res or DEFAULT_RESOLUTIONS
    args.outdir.mkdir(exist_ok=True)

    for w, h in resolutions:
        write_fps, read_fps = measure_speed(w, h, args.frames, args.outdir)
        print(f"{w}x{h}")
        print(f"  write_fps: {write_fps:.2f}")
        print(f"  read_fps:  {read_fps:.2f}")
        # Additional lines used by run_all_benchmarks.parse_results
        print(f"write_fps_{w}x{h}: {write_fps:.2f} fps")
        print(f"read_fps_{w}x{h}: {read_fps:.2f} fps")


if __name__ == "__main__":
    main()
