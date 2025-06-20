# Common code fragment for tests
set -euo pipefail

# Ensure VERBOSE is defined to avoid "unbound variable" errors when
# running tests in environments where it is not set (such as the Codex
# runner).
VERBOSE=${VERBOSE:-FALSE}
#
srcdir=${srcdir:-.}
BUILDDIR=$(pwd)
SRCDIR=$(dirname "$0")
SRCDIR=$(cd "$SRCDIR" && pwd)
TOPSRCDIR=$(cd "$srcdir/.." && pwd)
TOOLS=$(cd ../tools && pwd)
IMAGES="${SRCDIR}/images"
REFS="${SRCDIR}/refs"

# Split MEMCHECK environment variable into an array for safe invocation
if [ -n "${MEMCHECK-}" ]; then
  # shellcheck disable=SC2206
  MEMCHECK=( $MEMCHECK )
else
  MEMCHECK=()
fi

# Aliases for built tools
FAX2PS=${TOOLS}/fax2ps
FAX2TIFF=${TOOLS}/fax2tiff
PAL2RGB=${TOOLS}/pal2rgb
PPM2TIFF=${TOOLS}/ppm2tiff
RAW2TIFF=${TOOLS}/raw2tiff
RGB2YCBCR=${TOOLS}/rgb2ycbcr
THUMBNAIL=${TOOLS}/thumbnail
TIFF2BW=${TOOLS}/tiff2bw
TIFF2PDF=${TOOLS}/tiff2pdf
TIFF2PS=${TOOLS}/tiff2ps
TIFF2RGBA=${TOOLS}/tiff2rgba
TIFFCMP=${TOOLS}/tiffcmp
TIFFCP=${TOOLS}/tiffcp
TIFFCROP=${TOOLS}/tiffcrop
TIFFDITHER=${TOOLS}/tiffdither
TIFFDUMP=${TOOLS}/tiffdump
TIFFINFO=${TOOLS}/tiffinfo
TIFFMEDIAN=${TOOLS}/tiffmedian
TIFFSET=${TOOLS}/tiffset
TIFFSPLIT=${TOOLS}/tiffsplit

# Aliases for input test files
IMG_MINISBLACK_1C_16B=${IMAGES}/minisblack-1c-16b.tiff
IMG_MINISBLACK_1C_8B=${IMAGES}/minisblack-1c-8b.tiff
IMG_MINISWHITE_1C_1B=${IMAGES}/miniswhite-1c-1b.tiff
IMG_PALETTE_1C_1B=${IMAGES}/palette-1c-1b.tiff
IMG_PALETTE_1C_4B=${IMAGES}/palette-1c-4b.tiff
IMG_PALETTE_1C_8B=${IMAGES}/palette-1c-8b.tiff
IMG_RGB_3C_16B=${IMAGES}/rgb-3c-16b.tiff
IMG_RGB_3C_8B=${IMAGES}/rgb-3c-8b.tiff
IMG_MINISBLACK_2C_8B_ALPHA=${IMAGES}/minisblack-2c-8b-alpha.tiff
IMG_QUAD_LZW_COMPAT=${IMAGES}/quad-lzw-compat.tiff
IMG_LZW_SINGLE_STROP=${IMAGES}/lzw-single-strip.tiff

IMG_TEST_CINEPI_LIBTIFF_DNG=${IMAGES}/TEST_CINEPI_LIBTIFF_DNG.dng
IMG_TEST_JPEG=${IMAGES}/TEST_JPEG.jpg

IMG_MINISWHITE_1C_1B_PBM=${IMAGES}/miniswhite-1c-1b.pbm
IMG_MINISBLACK_1C_8B_PGM=${IMAGES}/minisblack-1c-8b.pgm
IMG_RGB_3C_16B_PPM=${IMAGES}/rgb-3c-16b.ppm
IMG_RGB_3C_8B_PPM=${IMAGES}/rgb-3c-8b.ppm
IMG_TIFF_WITH_SUBIFD_CHAIN=${IMAGES}/tiff_with_subifd_chain.tif

# All uncompressed image files
IMG_UNCOMPRESSED="${IMG_MINISBLACK_1C_16B} ${IMG_MINISBLACK_1C_8B} ${IMG_MINISWHITE_1C_1B} ${IMG_PALETTE_1C_1B} ${IMG_PALETTE_1C_4B} ${IMG_PALETTE_1C_4B} ${IMG_PALETTE_1C_8B} ${IMG_RGB_3C_8B} ${IMG_RGB_3C_16B}"

#
# Test a simple convert-like command.
#
# f_test_convert command infile outfile
f_test_convert ()
{ 
  command=$1
  infile=$2
  outfile=$3
  rm -f "$outfile"
  echo "${MEMCHECK[@]} $command $infile $outfile"
  cmd="${MEMCHECK[*]} $command \"$infile\" \"$outfile\""
  eval "$cmd"
  status=$?
  if [ "$status" != 0 ] ; then
    echo "Returned failed status $status!"
    echo "Output (if any) is in \"${outfile}\"."
    exit $status
  fi
}

#
# Test a simple command which sends output to stdout
#
# f_test_stdout command infile outfile
f_test_stdout ()
{ 
  command=$1
  infile=$2
  outfile=$3
  rm -f "$outfile"
  echo "${MEMCHECK[@]} $command $infile > $outfile"
  cmd="${MEMCHECK[*]} $command \"$infile\""
  eval "$cmd" > "$outfile"
  status=$?
  if [ "$status" != 0 ] ; then
    echo "Returned failed status $status!"
    echo "Output (if any) is in \"${outfile}\"."
    exit $status
  fi
}

#
# Execute a simple command (e.g. tiffinfo) with one input file
#
# f_test_exec command infile
f_test_reader ()
{ 
  command=$1
  infile=$2
  echo "${MEMCHECK[@]} $command $infile"
  cmd="${MEMCHECK[*]} $command \"$infile\""
  eval "$cmd"
  status=$?
  if [ "$status" != 0 ] ; then
    echo "Returned failed status $status!"
    exit $status
  fi
}

#
# Execute tiffinfo on a specified file to validate it
#
# f_tiffinfo_validate infile
f_tiffinfo_validate ()
{
    f_test_reader "$TIFFINFO -D" "$1"
}

if test "$VERBOSE" = TRUE
then
  set -x
fi

