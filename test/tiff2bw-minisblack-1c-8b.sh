#!/bin/bash
set -euo pipefail
# Generated file, master is Makefile.am
. ${srcdir:-.}/common.sh
infile="$srcdir/images/minisblack-1c-8b.tiff"
outfile="o-tiff2bw-minisblack-1c-8b.tiff"
f_test_convert "$TIFF2BW" "$infile" "$outfile"
f_tiffinfo_validate "$outfile"
