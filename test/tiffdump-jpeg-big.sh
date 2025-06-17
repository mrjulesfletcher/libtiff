#!/bin/bash
set -euo pipefail
. ${srcdir:-.}/common.sh
outfile="${TMPDIR:-/tmp}/TEST_JPEG_BIG.tif"
python3 "$SRCDIR/gen_bigtiff_from_jpeg.py" "$IMG_TEST_JPEG" "$outfile" "$TIFFCP"
f_test_reader "${TIFFDUMP}" "$outfile"
rm -f "$outfile"
