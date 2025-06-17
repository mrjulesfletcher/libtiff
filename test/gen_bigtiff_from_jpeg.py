#!/usr/bin/env python3
import sys
import subprocess
import os

try:
    from PIL import Image
except Exception:
    sys.stderr.write("Pillow is required. Install with 'pip install pillow'.\n")
    sys.exit(1)


def main(src, dst, tiffcp):
    tmp = dst + ".tmp.tif"
    Image.open(src).save(tmp, format="TIFF")
    subprocess.run([tiffcp, '-8', tmp, dst], check=True)
    os.remove(tmp)

if __name__ == '__main__':
    if len(sys.argv) != 4:
        print("Usage: gen_bigtiff_from_jpeg.py INPUT.jpg OUTPUT.tif TIFFCP", file=sys.stderr)
        sys.exit(1)
    main(sys.argv[1], sys.argv[2], sys.argv[3])
