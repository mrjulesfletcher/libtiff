#!/bin/bash
set -euo pipefail
# Generated file, master is Makefile.am
. ${srcdir:-.}/common.sh
infile="$srcdir/images/miniswhite-1c-1b.tiff"
outfile="o-tiffcp-miniswhite-jpegls.tiff"
f_test_convert "${TIFFCP} -c jpegls" "$infile" "$outfile"
f_tiffinfo_validate "$outfile"

