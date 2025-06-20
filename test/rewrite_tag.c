/*
 * Copyright (c) 2007, Frank Warmerdam <warmerdam@pobox.com>
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

/*
 * TIFF Library
 *
 * Module to test _TIFFRewriteField().
 */

#include "tif_config.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "tiffio.h"
#include "tiffiop.h"

const uint32_t rows_per_strip = 1;

int test_packbits()

{
    TIFF *tif;
    int i;
    unsigned char buf[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    uint32_t width = 10;
    int length = 20;
    const char *filename = "test_packbits.tif";

    /* Test whether we can write tags. */
    tif = TIFFOpen(filename, "w");

    if (!tif)
    {
        fprintf(stderr, "Can't create test TIFF file %s.\n", filename);
        return 1;
    }

    if (!TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_PACKBITS))
    {
        fprintf(stderr, "Can't set Compression tag.\n");
        goto failure;
    }
    if (!TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width))
    {
        fprintf(stderr, "Can't set ImageWidth tag.\n");
        goto failure;
    }
    if (!TIFFSetField(tif, TIFFTAG_IMAGELENGTH, length))
    {
        fprintf(stderr, "Can't set ImageLength tag.\n");
        goto failure;
    }
    if (!TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8))
    {
        fprintf(stderr, "Can't set BitsPerSample tag.\n");
        goto failure;
    }
    if (!TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1))
    {
        fprintf(stderr, "Can't set SamplesPerPixel tag.\n");
        goto failure;
    }
    if (!TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, rows_per_strip))
    {
        fprintf(stderr, "Can't set SamplesPerPixel tag.\n");
        goto failure;
    }
    if (!TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG))
    {
        fprintf(stderr, "Can't set PlanarConfiguration tag.\n");
        goto failure;
    }

    for (i = 0; i < length; i++)
    {
        if (!TIFFWriteEncodedStrip(tif, i, buf, 10))
        {
            fprintf(stderr, "Can't write image data.\n");
            goto failure;
        }
    }

    TIFFClose(tif);

    /* Test whether we can write tags. */

    tif = TIFFOpen(filename, "r+");

    if (!tif)
    {
        fprintf(stderr, "Can't create test TIFF file %s.\n", filename);
        return 1;
    }

    buf[3] = 17;
    buf[6] = 12;

    if (!TIFFWriteEncodedStrip(tif, 6, buf, 10))
    {
        fprintf(stderr, "Can't write image data.\n");
        goto failure;
    }

    TIFFClose(tif);

    unlink(filename);

    return 0;

failure:
    /* Something goes wrong; close file and return unsuccessful status. */
    TIFFClose(tif);
    /*  unlink(filename); */

    return 1;
}

/************************************************************************/
/*                            rewrite_test()                            */
/************************************************************************/
int rewrite_test(const char *filename, uint32_t width, int length, int bigtiff,
                 uint64_t base_value)

{
    TIFF *tif;
    int i;
    unsigned char *buf;
    uint64_t *rowoffset, *rowbytes;
    uint64_t *upd_rowoffset = NULL;
    uint64_t *upd_bytecount = NULL;

    buf = calloc(1, width);
    assert(buf);

    /* Test whether we can write tags. */
    if (bigtiff)
        tif = TIFFOpen(filename, "w8");
    else
        tif = TIFFOpen(filename, "w4");

    if (!tif)
    {
        fprintf(stderr, "Can't create test TIFF file %s.\n", filename);
        free(buf);
        return 1;
    }

    if (!TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_PACKBITS))
    {
        fprintf(stderr, "Can't set Compression tag.\n");
        goto failure;
    }
    if (!TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width))
    {
        fprintf(stderr, "Can't set ImageWidth tag.\n");
        goto failure;
    }
    if (!TIFFSetField(tif, TIFFTAG_IMAGELENGTH, length))
    {
        fprintf(stderr, "Can't set ImageLength tag.\n");
        goto failure;
    }
    if (!TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8))
    {
        fprintf(stderr, "Can't set BitsPerSample tag.\n");
        goto failure;
    }
    if (!TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1))
    {
        fprintf(stderr, "Can't set SamplesPerPixel tag.\n");
        goto failure;
    }
    if (!TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, rows_per_strip))
    {
        fprintf(stderr, "Can't set SamplesPerPixel tag.\n");
        goto failure;
    }
    if (!TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG))
    {
        fprintf(stderr, "Can't set PlanarConfiguration tag.\n");
        goto failure;
    }

    for (i = 0; i < length; i++)
    {
        if (TIFFWriteScanline(tif, buf, i, 0) == -1)
        {
            fprintf(stderr, "Can't write image data.\n");
            goto failure;
        }
    }

    TIFFClose(tif);

    /* Ok, now test whether we can read written values. */
    tif = TIFFOpen(filename, "r+");
    if (!tif)
    {
        fprintf(stderr, "Can't open test TIFF file %s.\n", filename);
        free(buf);
        return 1;
    }

    if (!TIFFGetField(tif, TIFFTAG_STRIPOFFSETS, &rowoffset))
    {
        fprintf(stderr, "Can't fetch STRIPOFFSETS.\n");
        goto failure;
    }

    if (!TIFFGetField(tif, TIFFTAG_STRIPBYTECOUNTS, &rowbytes))
    {
        fprintf(stderr, "Can't fetch STRIPBYTECOUNTS.\n");
        goto failure;
    }

    upd_rowoffset = (uint64_t *)_TIFFmalloc(sizeof(uint64_t) * length);
    if (!upd_rowoffset)
    {
        TIFFError("rewrite_test", "Failed to allocate upd_rowoffset buffer");
        goto failure;
    }
    for (i = 0; i < length; i++)
        upd_rowoffset[i] = base_value + i * width;

    if (!_TIFFRewriteField(tif, TIFFTAG_STRIPOFFSETS, TIFF_LONG8, length,
                           upd_rowoffset))
    {
        fprintf(stderr, "Can't rewrite STRIPOFFSETS.\n");
        goto failure;
    }

    _TIFFfree(upd_rowoffset);
    upd_rowoffset = NULL;

    upd_bytecount = (uint64_t *)_TIFFmalloc(sizeof(uint64_t) * length);
    if (!upd_bytecount)
    {
        TIFFError("rewrite_test", "Failed to allocate upd_bytecount buffer");
        goto failure;
    }
    for (i = 0; i < length; i++)
        upd_bytecount[i] = 100 + (uint64_t)i * width;

    if (!_TIFFRewriteField(tif, TIFFTAG_STRIPBYTECOUNTS, TIFF_LONG8, length,
                           upd_bytecount))
    {
        fprintf(stderr, "Can't rewrite STRIPBYTECOUNTS.\n");
        goto failure;
    }

    _TIFFfree(upd_bytecount);
    upd_bytecount = NULL;

    TIFFClose(tif);

    /* Reopen file and read back to verify contents */

    tif = TIFFOpen(filename, "r");
    if (!tif)
    {
        fprintf(stderr, "Can't open test TIFF file %s.\n", filename);
        free(buf);
        return 1;
    }

    if (!TIFFGetField(tif, TIFFTAG_STRIPOFFSETS, &rowoffset))
    {
        fprintf(stderr, "Can't fetch STRIPOFFSETS.\n");
        goto failure;
    }

    for (i = 0; i < length; i++)
    {
        uint64_t expect = base_value + i * width;

        if (rowoffset[i] != expect)
        {
            fprintf(stderr,
                    "%s:STRIPOFFSETS[%d]: Got %X:%08X instead of %X:%08X.\n",
                    filename, i, (int)(rowoffset[i] >> 32),
                    (int)(rowoffset[i] & 0xFFFFFFFF), (int)(expect >> 32),
                    (int)(expect & 0xFFFFFFFF));
            goto failure;
        }
    }

    if (!TIFFGetField(tif, TIFFTAG_STRIPBYTECOUNTS, &rowbytes))
    {
        fprintf(stderr, "Can't fetch STRIPBYTECOUNTS.\n");
        goto failure;
    }

    for (i = 0; i < length; i++)
    {
        uint64_t expect = 100 + (uint64_t)i * width;

        if (rowbytes[i] != expect)
        {
            fprintf(stderr,
                    "%s:STRIPBYTECOUNTS[%d]: Got %X:%08X instead of %X:%08X.\n",
                    filename, i, (int)(rowbytes[i] >> 32),
                    (int)(rowbytes[i] & 0xFFFFFFFF), (int)(expect >> 32),
                    (int)(expect & 0xFFFFFFFF));
            goto failure;
        }
    }

    TIFFClose(tif);
    free(buf);

    /* All tests passed; delete file and exit with success status. */
    unlink(filename);
    return 0;

failure:
    /* Something goes wrong; close file and return unsuccessful status. */
    TIFFClose(tif);
    free(buf);
    if (upd_rowoffset != NULL)
    {
        _TIFFfree(upd_rowoffset);
    }
    if (upd_bytecount != NULL)
    {
        _TIFFfree(upd_bytecount);
    }
    /*  unlink(filename); */

    return 1;
}

/************************************************************************/
/*                                main()                                */
/************************************************************************/
int main(void)
{
    int failure = 0;

    failure |= test_packbits();

    /* test fairly normal use */
    failure |= rewrite_test("rewrite1.tif", 10, 10, 0, 100);
    failure |= rewrite_test("rewrite2.tif", 10, 10, 1, 100);

    /* test case of fitting all in directory entry */
    failure |= rewrite_test("rewrite3.tif", 10, 1, 0, 100);
    failure |= rewrite_test("rewrite4.tif", 10, 1, 1, 100);

    /* test with very large values that don't fit in 4bytes (bigtiff only) */
    failure |= rewrite_test("rewrite5.tif", 10, 1000, 1, 0x6000000000ULL);
    failure |= rewrite_test("rewrite6.tif", 10, 1, 1, 0x6000000000ULL);

    /* StripByteCounts on LONG */
    failure |= rewrite_test("rewrite7.tif", 65536, 1, 0, 100);
    failure |= rewrite_test("rewrite8.tif", 65536, 2, 0, 100);

    return failure;
}
