#!/bin/bash
set -euo pipefail
. ${srcdir:-.}/common.sh
infile="${IMAGES}/TEST_CINEPI_LIBTIFF_DNG.dng"
outfile="o-tiffcp-invalid-compression.tiff"
rm -f "$outfile"
echo "${MEMCHECK[@]} ${TIFFCP} -c bogus $infile $outfile"
if "${MEMCHECK[@]}" "${TIFFCP}" -c bogus "$infile" "$outfile"; then
  echo "Unexpected success with invalid compression option"
  exit 1
else
  echo "tiffcp failed with invalid compression as expected"
fi
