#!/bin/bash
set -euo pipefail
#
# Basic sanity check for tiffcp with G4 compression
#
. ${srcdir:-.}/common.sh
f_test_convert "${TIFFCP} -c g4" "${IMG_MINISWHITE_1C_1B}" "o-tiffcp-g4.tiff"