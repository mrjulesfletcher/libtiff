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

# Determine NEON flags for this toolchain
set(TIFF_NEON_FLAGS "" CACHE STRING "Flags used to enable NEON instructions")
set(_neon_flags "${TIFF_NEON_FLAGS}")
if(NOT _neon_flags)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)")
    set(_neon_flags "-march=armv8-a+simd")
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    set(_neon_flags "-mfpu=neon")
  endif()
endif()

set(_save_required_flags "${CMAKE_REQUIRED_FLAGS}")
set(CMAKE_REQUIRED_FLAGS "${_neon_flags}")
check_c_source_compiles(
  "#include <arm_neon.h>
  int main(){ uint8x16_t v = vdupq_n_u8(0); return (int)v[0]; }"
  HAVE_NEON)
set(CMAKE_REQUIRED_FLAGS "${_save_required_flags}")
if(HAVE_NEON)
  add_compile_definitions(HAVE_NEON=1)
  if(_neon_flags)
    add_compile_options("${_neon_flags}")
  endif()
endif()

check_c_source_compiles(
  "#include <arm_acle.h>
   int main(){ unsigned v = 0; v = __crc32d(v, 0); return (int)v; }"
  HAVE_ARM_CRC32)
if(HAVE_ARM_CRC32)
  add_compile_definitions(HAVE_ARM_CRC32=1)
endif()

# Determine SSE flags
set(TIFF_SSE_FLAGS "" CACHE STRING "Flags used to enable SSE instructions")
set(_sse41_flags "${TIFF_SSE_FLAGS}")
set(_sse42_flags "${TIFF_SSE_FLAGS}")
if(NOT _sse41_flags)
  if(CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    set(_sse41_flags "-msse4.1")
  endif()
endif()
if(NOT _sse42_flags)
  if(CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    set(_sse42_flags "-msse4.2")
  endif()
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
  if(_sse41_flags)
    add_compile_options("${_sse41_flags}")
  endif()
endif()

check_c_source_compiles(
  "#include <nmmintrin.h>
   int main(){ return _mm_crc32_u64(0,0); }"
  HAVE_SSE42)
if(HAVE_SSE42)
  add_compile_definitions(HAVE_SSE42=1)
  if(_sse42_flags)
    add_compile_options("${_sse42_flags}")
  endif()
endif()

# Detect AES instruction support
set(_aes_flags "${TIFF_SSE_FLAGS}")
if(NOT _aes_flags)
  if(CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|arm)")
      set(_aes_flags "-march=armv8-a+crypto")
    else()
      set(_aes_flags "-maes")
    endif()
  endif()
endif()

check_c_source_compiles(
  "#if defined(__aarch64__) || defined(__arm__)
   #include <arm_neon.h>
   #include <arm_acle.h>
   int main(){ uint8x16_t v = vdupq_n_u8(0); v = vaeseq_u8(v, v); v = vaesmcq_u8(v); return (int)v[0]; }
   #else
   #include <wmmintrin.h>
   int main(){ __m128i v = _mm_setzero_si128(); v = _mm_aesenc_si128(v, _mm_setzero_si128()); return _mm_extract_epi8(v,0); }
   #endif"
  HAVE_HW_AES)
if(HAVE_HW_AES)
  add_compile_definitions(HAVE_HW_AES=1)
  if(_aes_flags)
    add_compile_options("${_aes_flags}")
  endif()
endif()

# Detect PMULL / CLMUL instruction support
set(_pmull_flags "${TIFF_SSE_FLAGS}")
if(NOT _pmull_flags)
  if(CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|arm)")
      set(_pmull_flags "-march=armv8-a+crypto")
    else()
      set(_pmull_flags "-mpclmul")
    endif()
  endif()
endif()

check_c_source_compiles(
  "#if defined(__aarch64__) || defined(__arm__)
   #include <arm_neon.h>
   int main(){ poly64_t a=0,b=0; (void)vmull_p64(a,b); return 0; }
   #else
   #include <wmmintrin.h>
   int main(){ __m128i x=_mm_setzero_si128(); __m128i y=_mm_setzero_si128(); __m128i r=_mm_clmulepi64_si128(x,y,0); return _mm_cvtsi128_si32(r); }
   #endif"
  HAVE_PMULL)
if(HAVE_PMULL)
  add_compile_definitions(HAVE_PMULL=1)
  if(_pmull_flags)
    add_compile_options("${_pmull_flags}")
  endif()
endif()
