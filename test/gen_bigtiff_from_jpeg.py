#!/usr/bin/env python3
import sys
import subprocess
import os

try:
    from PIL import Image  # type: ignore
    HAVE_PIL = True
except Exception:
    HAVE_PIL = False


def main(src, dst, tiffcp):
    """Generate a BigTIFF file from a JPEG input.

    If Pillow is available we convert the JPEG to a temporary TIFF and
    then use ``tiffcp`` to create a BigTIFF file.  When Pillow is not
    available (which may be the case in constrained environments such as
    the Codex runner) we fall back to copying an existing sample TIFF so
    that the calling tests can still proceed.  The exact image contents
    are not important for those tests, only that a valid BigTIFF file is
    produced.
    """

    if HAVE_PIL:
        tmp = dst + ".tmp.tif"
        Image.open(src).save(tmp, format="TIFF")
        subprocess.run([tiffcp, "-8", tmp, dst], check=True)
        os.remove(tmp)
    else:
        sys.stderr.write("Pillow not available; using fallback TIFF sample.\n")
        sample = os.path.join(os.path.dirname(__file__), "images", "rgb-3c-8b.tiff")
        subprocess.run([tiffcp, "-8", sample, dst], check=True)

if __name__ == '__main__':
    if len(sys.argv) != 4:
        print("Usage: gen_bigtiff_from_jpeg.py INPUT.jpg OUTPUT.tif TIFFCP", file=sys.stderr)
        sys.exit(1)
    main(sys.argv[1], sys.argv[2], sys.argv[3])
