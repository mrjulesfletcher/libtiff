/******************************************************************************
 * Project:  GeoTIFF Overview Builder
 * Purpose:  Mainline for building overviews in a TIFF file.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 *
 * $Log: addtiffo.c,v $
 * Revision 1.8  2015-05-30 20:30:27  bfriesen
 * * contrib/addtiffo/addtiffo.c (main): Possibly address Coverity
 * 1024226 "Untrusted value as argument".
 *
 * Revision 1.7  2010-06-08 18:55:15  bfriesen
 * * contrib: Add an emacs formatting mode footer to all source files
 * so that emacs can be effectively used.
 *
 * Revision 1.6  2005/12/16 05:59:55  fwarmerdam
 * Major upgrade to support YCbCr subsampled jpeg images
 *
 * Revision 1.4  2004/09/21 13:31:23  dron
 * Add missed include string.h.
 *
 * Revision 1.3  2000/04/18 22:48:31  warmerda
 * Added support for averaging resampling
 *
 * Revision 1.2  2000/01/28 15:36:38  warmerda
 * pass TIFF handle instead of filename to overview builder
 *
 * Revision 1.1  1999/08/17 01:47:59  warmerda
 * New
 *
 * Revision 1.1  1999/03/12 17:46:32  warmerda
 * New
 *
 * Revision 1.2  1999/02/11 22:27:12  warmerda
 * Added multi-sample support
 *
 * Revision 1.1  1999/02/11 18:12:30  warmerda
 * New
 */

#include "tif_ovrcache.h"
#include "tiffio.h"
#define DIV_ROUND_UP32(x, y) (((x) + (y)-1) / (y))
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void TIFFBuildOverviews(TIFF *, int, int *, int, OVRResampleMethod,
                        int (*)(double, void *), void *);

/************************************************************************/
/*                                main()                                */
/************************************************************************/

int main(int argc, char **argv)

{
    int *anOverviews = NULL; /* dynamically allocated overview list */
    int nOverviewCount = 0;
    int bUseSubIFD = 0;
    TIFF *hTIFF;
    OVRResampleMethod eResampling = OVR_RESAMPLE_NEAREST;

    /* -------------------------------------------------------------------- */
    /*      Usage:                                                          */
    /* -------------------------------------------------------------------- */
    if (argc < 2)
    {
        printf("Usage: addtiffo [-r {nearest,average,mode}]\n"
               "                tiff_filename [resolution_reductions]\n"
               "\n"
               "Example:\n"
               " %% addtiffo abc.tif 2 4 8 16\n");
        return (1);
    }

    while (argv[1][0] == '-')
    {
        if (strcmp(argv[1], "-subifd") == 0)
        {
            bUseSubIFD = 1;
            argv++;
            argc--;
        }
        else if (strcmp(argv[1], "-r") == 0)
        {
            argv += 2;
            argc -= 2;
            if (strncmp(*argv, "nearest", 7) == 0)
                eResampling = OVR_RESAMPLE_NEAREST;
            else if (strncmp(*argv, "average", 7) == 0)
                eResampling = OVR_RESAMPLE_AVERAGE;
            else if (strncmp(*argv, "mode", 4) == 0)
                eResampling = OVR_RESAMPLE_MODE;
            else
            {
                fprintf(stderr, "Unknown resampling method: %s\n", *argv);
                free(anOverviews);
                return (1);
            }
        }
        else
        {
            fprintf(stderr, "Incorrect parameters\n");
            return (1);
        }
    }

    /* -------------------------------------------------------------------- */
    /*      Open the file and fetch image dimensions.                        */
    /* -------------------------------------------------------------------- */
    hTIFF = TIFFOpen(argv[1], "r+");
    if (hTIFF == NULL)
    {
        fprintf(stderr, "TIFFOpen(%s) failed.\n", argv[1]);
        return (1);
    }

    uint32_t nImageWidth = 0, nImageLength = 0;
    TIFFGetField(hTIFF, TIFFTAG_IMAGEWIDTH, &nImageWidth);
    TIFFGetField(hTIFF, TIFFTAG_IMAGELENGTH, &nImageLength);

    /* -------------------------------------------------------------------- */
    /*      Collect the user requested reduction factors or compute default. */
    /* -------------------------------------------------------------------- */
    if (argc > 2)
    {
        nOverviewCount = argc - 2;
        anOverviews = (int *)calloc(nOverviewCount, sizeof(int));
        if (anOverviews == NULL)
        {
            fprintf(stderr, "Out of memory.\n");
            TIFFClose(hTIFF);
            return (1);
        }
        for (int i = 0; i < nOverviewCount; i++)
        {
            anOverviews[i] = atoi(argv[i + 2]);
            if (anOverviews[i] <= 0 || anOverviews[i] > 1024)
            {
                fprintf(stderr, "Incorrect parameters\n");
                free(anOverviews);
                TIFFClose(hTIFF);
                return (1);
            }
        }
    }
    else
    {
        int nOvrFactor = 1;
        while (DIV_ROUND_UP32(nImageWidth, nOvrFactor) > 256 ||
               DIV_ROUND_UP32(nImageLength, nOvrFactor) > 256)
        {
            nOvrFactor *= 2;
            int *tmp =
                (int *)realloc(anOverviews, sizeof(int) * (nOverviewCount + 1));
            if (tmp == NULL)
            {
                fprintf(stderr, "Out of memory.\n");
                free(anOverviews);
                TIFFClose(hTIFF);
                return (1);
            }
            anOverviews = tmp;
            anOverviews[nOverviewCount++] = nOvrFactor;
        }
    }

    /* -------------------------------------------------------------------- */
    /*      Build the overview.                                             */
    /* -------------------------------------------------------------------- */
    if (nOverviewCount > 0)
        TIFFBuildOverviews(hTIFF, nOverviewCount, anOverviews, bUseSubIFD,
                           eResampling, NULL, NULL);

    TIFFClose(hTIFF);

/* -------------------------------------------------------------------- */
/*      Optionally test for memory leaks.                               */
/* -------------------------------------------------------------------- */
#ifdef DBMALLOC
    malloc_dump(1);
#endif

    free(anOverviews);
    return (0);
}
