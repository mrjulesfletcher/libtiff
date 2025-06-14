# Processor capability checks
#
# Copyright © 2015 Open Microscopy Environment / University of Dundee
# Copyright © 2021 Roger Leigh <rleigh@codelibre.net>
# Written by Roger Leigh <rleigh@codelibre.net>
#
# Permission to use, copy, modify, distribute, and sell this software and
# its documentation for any purpose is hereby granted without fee, provided
# that (i) the above copyright notices and this permission notice appear in
# all copies of the software and related documentation, and (ii) the names of
# Sam Leffler and Silicon Graphics may not be used in any advertising or
# publicity relating to the software without the specific, prior written
# permission of Sam Leffler and Silicon Graphics.
#
# THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
# EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
# WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
#
# IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
# ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
# OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
# WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
# LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
# OF THIS SOFTWARE.


include(TestBigEndian)

# CPU endianness
test_big_endian(HOST_BIG_ENDIAN)
if(HOST_BIG_ENDIAN)
    set(WORDS_BIGENDIAN TRUE)
else()
    set(WORDS_BIGENDIAN FALSE)
endif()

# IEEE floating point
set(HAVE_IEEEFP 1)

include(CheckCSourceCompiles)
check_c_source_compiles(
  "#include <arm_neon.h>
  int main(){ uint8x16_t v = vdupq_n_u8(0); return (int)v[0]; }"
  HAVE_NEON)
if(HAVE_NEON)
  add_compile_definitions(HAVE_NEON=1)
endif()

check_c_source_compiles(
  "#include <emmintrin.h>
   int main(){ __m128i v = _mm_setzero_si128(); return _mm_cvtsi128_si32(v); }"
  HAVE_SSE2)
if(HAVE_SSE2)
  add_compile_definitions(HAVE_SSE2=1)
endif()

check_c_source_compiles(
  "#include <smmintrin.h>
   int main(){ __m128i v = _mm_setzero_si128(); return _mm_extract_epi8(v,0); }"
  HAVE_SSE41)
if(HAVE_SSE41)
  add_compile_definitions(HAVE_SSE41=1)
endif()
