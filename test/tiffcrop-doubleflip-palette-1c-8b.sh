#!/bin/bash
set -euo pipefail
# Generated file, master is Makefile.am
. ${srcdir:-.}/common.sh
infile="$srcdir/images/palette-1c-8b.tiff"
outfile="o-tiffcrop-doubleflip-palette-1c-8b.tiff"
f_test_convert "$TIFFCROP -F both" "$infile" "$outfile"
f_tiffinfo_validate "$outfile"
