#!/bin/bash
set -euo pipefail
# Generated file, master is Makefile.am
. ${srcdir:-.}/common.sh
infile="$srcdir/images/minisblack-1c-16b.tiff"
outfile="o-tiffcrop-doubleflip-minisblack-1c-16b.tiff"
f_test_convert "$TIFFCROP -F both" "$infile" "$outfile"
f_tiffinfo_validate "$outfile"
