# Tag Image File Format (TIFF) Software
#
# Copyright (C) 2004, Andrey Kiselev <dron@ak4719.spb.edu>
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

# Process this file with automake to produce Makefile.in.

LIBPORT = $(top_builddir)/port/libport.la
LIBTIFF = $(top_builddir)/libtiff/libtiff.la
libtiffincludedir = $(includedir)

EXTRA_DIST = \
	CMakeLists.txt \
	libtiff.def \
	libtiff.map \
	libtiffxx.map \
	tiffvers.h.cmake.in \
	tif_config.h.cmake.in \
	tiffconf.h.cmake.in \
	tif_win32_versioninfo.rc

libtiffinclude_HEADERS = \
	tiff.h \
	tiffio.h \
	tiffvers.h

if HAVE_CXX
libtiffinclude_HEADERS += tiffio.hxx
endif

noinst_HEADERS = \
	t4.h \
	tif_dir.h \
	tif_hash_set.h \
        tif_predict.h \
        tiffiop.h \
        uvcode.h \
        tif_bayer.h \
        rgb_neon.h \
        strip_neon.h \
        strip_sse41.h \
        strip_simd.h \
        reverse_bits_neon.h \
        reverse_bits_sse.h \
        vulkan_shader_spv.h \
        predictor_spv.h

nodist_libtiffinclude_HEADERS = \
	tiffconf.h

libtiff_la_SOURCES = \
	tif_aux.c \
	tif_close.c \
	tif_codec.c \
	tif_color.c \
	tif_compress.c \
	tif_dir.c \
	tif_dirinfo.c \
	tif_dirread.c \
	tif_dirwrite.c \
	tif_dumpmode.c \
	tif_error.c \
	tif_extension.c \
	tif_fax3.c \
	tif_fax3sm.c \
	tif_flush.c \
	tif_getimage.c \
	tif_hash_set.c \
	tif_jbig.c \
	tif_jpeg.c \
	tif_jpeg_12.c \
	tif_lerc.c \
	tif_luv.c \
	tif_lzma.c \
	tif_lzw.c \
	tif_next.c \
	tif_ojpeg.c \
	tif_open.c \
	tif_packbits.c \
	tif_pixarlog.c \
        tif_predict.c \
        tif_print.c \
       tif_read.c \
      tif_strip.c \
      tif_strip_neon.c \
      tif_strip_sse41.c \
      tif_strip_simd.c \
      tiff_simd.c \
      tif_swab.c \
        tif_thunder.c \
        tif_tile.c \
        tif_version.c \
        tif_warning.c \
        tif_jpegls.c \
        tif_webp.c \
        tif_bayer.c \
        tif_rgb.c \
        tif_write.c \
        tif_uring.c \
        tif_vulkan.c \
        tif_hvs.c \
        tiff_threadpool.c \
        tif_mmap.c \
        tif_zip.c \
        tif_zstd.c

libtiffxx_la_SOURCES = \
	tif_stream.cxx

if WIN32_IO
EXTRA_DIST += tif_unix.c
libtiff_la_SOURCES += tif_win32.c
else
EXTRA_DIST += tif_win32.c
libtiff_la_SOURCES += tif_unix.c
endif

lib_LTLIBRARIES = libtiff.la
if HAVE_CXX
lib_LTLIBRARIES += libtiffxx.la
endif

libtiff_la_LDFLAGS = \
	-no-undefined \
	-version-info $(LIBTIFF_VERSION_INFO)
if HAVE_RPATH
libtiff_la_LDFLAGS += $(LIBDIR)
endif
if HAVE_LD_VERSION_SCRIPT
libtiff_la_LDFLAGS += -Wl,--version-script=$(srcdir)/libtiff.map
endif

libtiffxx_la_LDFLAGS = \
	-no-undefined \
	-version-info $(LIBTIFF_VERSION_INFO)
if HAVE_RPATH
libtiffxx_la_LDFLAGS += $(LIBDIR)
endif
if HAVE_LD_VERSION_SCRIPT
libtiffxx_la_LDFLAGS += -Wl,--version-script=$(srcdir)/libtiffxx.map
endif
libtiffxx_la_LIBADD = $(LIBTIFF)
libtiffxx_la_DEPENDENCIES = libtiff.la
