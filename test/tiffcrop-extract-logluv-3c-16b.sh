#!/bin/bash
set -euo pipefail
# Generated file, master is Makefile.am
. ${srcdir:-.}/common.sh
infile="$srcdir/images/logluv-3c-16b.tiff"
outfile="o-tiffcrop-extract-logluv-3c-16b.tiff"
f_test_convert "$TIFFCROP -U px -E top -X 60 -Y 60" "$infile" "$outfile"
f_tiffinfo_validate "$outfile"
