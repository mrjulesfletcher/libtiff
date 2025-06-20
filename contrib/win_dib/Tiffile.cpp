#include "StdAfx.h"

// #define STRICT
#include "diblib.h"
#include <commdlg.h>
#include <stdlib.h> // MAX_ constants
#include <windows.h>
#include <windowsx.h>

/*--------------------------------------------------------------------
        READ TIFF
        Load the TIFF data from the file into memory.  Return
        a pointer to a valid DIB (or nullptr for errors).
        Uses the TIFFRGBA interface to libtiff.lib to convert
        most file formats to a usable form.  We just keep the 32 bit
        form of the data to display, rather than optimizing for the
        display.

        Main entry points:

            int ChkTIFF ( LPCTSTR lpszPath )
            PVOID ReadTIFF ( LPCTSTR lpszPath )

        RETURN
            A valid DIB pointer for success; nullptr for failure.

  --------------------------------------------------------------------*/

#include "TiffLib/tiff.h"
#include "TiffLib/tiffio.h"
#include <array>
#include <assert.h>
#include <memory>
#include <stdio.h>
#include <vector>

// piggyback some data on top of the RGBA Image
struct TIFFDibImage
{
    TIFFRGBAImage tif;
    int dibinstalled;
};

HANDLE LoadTIFFinDIB(LPCTSTR lpFileName);
HANDLE TIFFRGBA2DIB(TIFFDibImage *dib, uint32_t *raster);

static void MyWarningHandler(const char *module, const char *fmt, va_list ap)
{
    // ignore all warnings (unused tags, etc)
    return;
}

static void MyErrorHandler(const char *module, const char *fmt, va_list ap)
{
    return;
}

//  Turn off the error and warning handlers to check if a valid file.
//  Necessary because of the way that the Doc loads images and restart files.
int ChkTIFF(LPCTSTR lpszPath)
{
    int rtn = 0;

    TIFFErrorHandler eh;
    TIFFErrorHandler wh;

    eh = TIFFSetErrorHandler(nullptr);
    wh = TIFFSetWarningHandler(nullptr);

    std::unique_ptr<TIFF, decltype(&TIFFClose)> tif(TIFFOpen(lpszPath, "r"),
                                                    &TIFFClose);
    if (tif)
    {
        rtn = 1;
    }

    TIFFSetErrorHandler(eh);
    TIFFSetWarningHandler(wh);

    return rtn;
}

void DibInstallHack(TIFFDibImage *img);

PVOID ReadTIFF(LPCTSTR lpszPath)
{
    void *pDIB = nullptr;
    TIFFErrorHandler wh;

    wh = TIFFSetWarningHandler(MyWarningHandler);

    if (ChkTIFF(lpszPath))
    {
        std::unique_ptr<TIFF, decltype(&TIFFClose)> tif(TIFFOpen(lpszPath, "r"),
                                                        &TIFFClose);
        if (tif)
        {
            std::array<char, 1024> emsg{};

            if (TIFFRGBAImageOK(tif, emsg.data()))
            {
                TIFFDibImage img;

                if (TIFFRGBAImageBegin(&img.tif, tif, -1, emsg.data()))
                {
                    size_t npixels;
                    DibInstallHack(&img);

                    npixels = img.tif.width * img.tif.height;
                    std::vector<uint32_t> raster(npixels);
                    if (TIFFRGBAImageGet(&img.tif, raster.data(), img.tif.width,
                                         img.tif.height))
                    {
                        pDIB = TIFFRGBA2DIB(&img, raster.data());
                    }
                }
                TIFFRGBAImageEnd(&img.tif);
            }
            else
            {
                TRACE("Unable to open image(%s): %s\n", lpszPath, emsg.data());
            }
        }
    }

    TIFFSetWarningHandler(wh);

    return pDIB;
}

HANDLE TIFFRGBA2DIB(TIFFDibImage *dib, uint32_t *raster)
{
    void *pDIB = nullptr;
    TIFFRGBAImage *img = &dib->tif;

    uint32_t imageLength;
    uint32_t imageWidth;
    uint16_t BitsPerSample;
    uint16_t SamplePerPixel;
    uint32_t RowsPerStrip;
    uint16_t PhotometricInterpretation;

    BITMAPINFOHEADER bi;
    int dwDIBSize;

    TIFFGetField(img->tif, TIFFTAG_IMAGEWIDTH, &imageWidth);
    TIFFGetField(img->tif, TIFFTAG_IMAGELENGTH, &imageLength);
    TIFFGetField(img->tif, TIFFTAG_BITSPERSAMPLE, &BitsPerSample);
    TIFFGetField(img->tif, TIFFTAG_ROWSPERSTRIP, &RowsPerStrip);
    TIFFGetField(img->tif, TIFFTAG_SAMPLESPERPIXEL, &SamplePerPixel);
    TIFFGetField(img->tif, TIFFTAG_PHOTOMETRIC, &PhotometricInterpretation);

    if (BitsPerSample == 1 && SamplePerPixel == 1 && dib->dibinstalled)
    { // bilevel
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = imageWidth;
        bi.biHeight = imageLength;
        bi.biPlanes = 1; // always
        bi.biBitCount = 1;
        bi.biCompression = BI_RGB;
        bi.biSizeImage = WIDTHBYTES(bi.biWidth * bi.biBitCount) * bi.biHeight;
        bi.biXPelsPerMeter = 0;
        bi.biYPelsPerMeter = 0;
        bi.biClrUsed = 0;      //  must be zero for RGB compression (none)
        bi.biClrImportant = 0; // always

        // Get the size of the DIB
        dwDIBSize = GetDIBSize(&bi);

        // Allocate for the BITMAPINFO structure and the color table.
        pDIB = GlobalAllocPtr(GHND, dwDIBSize);
        if (pDIB == nullptr)
        {
            return nullptr;
        }

        // Copy the header info
        *reinterpret_cast<BITMAPINFOHEADER *>(pDIB) = bi;

        // Get a pointer to the color table
        RGBQUAD *pRgbq = reinterpret_cast<RGBQUAD *>(static_cast<LPSTR>(pDIB) +
                                                     sizeof(BITMAPINFOHEADER));

        pRgbq[0].rgbRed = 0;
        pRgbq[0].rgbBlue = 0;
        pRgbq[0].rgbGreen = 0;
        pRgbq[0].rgbReserved = 0;
        pRgbq[1].rgbRed = 255;
        pRgbq[1].rgbBlue = 255;
        pRgbq[1].rgbGreen = 255;
        pRgbq[1].rgbReserved = 255;

        // Pointers to the bits
        // PVOID pbiBits = (LPSTR)pRgbq + bi.biClrUsed * sizeof(RGBQUAD);
        //
        // In the BITMAPINFOHEADER documentation, it appears that
        // there should be no color table for 32 bit images, but
        // experience shows that the image is off by 3 words if it
        // is not included.  So here it is.
        PVOID pbiBits =
            GetDIBImagePtr(reinterpret_cast<BITMAPINFOHEADER *>(pDIB));

        _TIFFmemcpy(pbiBits, raster, bi.biSizeImage);
    }

    //  For now just always default to the RGB 32 bit form. // save as 32 bit
    //  for simplicity
    else if (true /*BitsPerSample == 8 && SamplePerPixel == 3*/)
    { // 24 bit color

        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = imageWidth;
        bi.biHeight = imageLength;
        bi.biPlanes = 1; // always
        bi.biBitCount = 32;
        bi.biCompression = BI_RGB;
        bi.biSizeImage = WIDTHBYTES(bi.biWidth * bi.biBitCount) * bi.biHeight;
        bi.biXPelsPerMeter = 0;
        bi.biYPelsPerMeter = 0;
        bi.biClrUsed = 0;      //  must be zero for RGB compression (none)
        bi.biClrImportant = 0; // always

        // Get the size of the DIB
        dwDIBSize = GetDIBSize(&bi);

        // Allocate for the BITMAPINFO structure and the color table.
        pDIB = GlobalAllocPtr(GHND, dwDIBSize);
        if (pDIB == nullptr)
        {
            return nullptr;
        }

        // Copy the header info
        *reinterpret_cast<BITMAPINFOHEADER *>(pDIB) = bi;

        // Get a pointer to the color table
        RGBQUAD *pRgbq = reinterpret_cast<RGBQUAD *>(static_cast<LPSTR>(pDIB) +
                                                     sizeof(BITMAPINFOHEADER));

        // Pointers to the bits
        // PVOID pbiBits = (LPSTR)pRgbq + bi.biClrUsed * sizeof(RGBQUAD);
        //
        // In the BITMAPINFOHEADER documentation, it appears that
        // there should be no color table for 32 bit images, but
        // experience shows that the image is off by 3 words if it
        // is not included.  So here it is.
        PVOID pbiBits = (LPSTR)pRgbq + 3 * sizeof(RGBQUAD);

        int sizeWords = bi.biSizeImage / 4;
        RGBQUAD *rgbDib = reinterpret_cast<RGBQUAD *>(pbiBits);
        const uint32_t *rgbTif = raster;

        // Swap the byte order while copying
        for (int i = 0; i < sizeWords; ++i)
        {
            rgbDib[i].rgbRed = TIFFGetR(rgbTif[i]);
            rgbDib[i].rgbBlue = TIFFGetB(rgbTif[i]);
            rgbDib[i].rgbGreen = TIFFGetG(rgbTif[i]);
            rgbDib[i].rgbReserved = 0;
        }
    }

    return pDIB;
}

///////////////////////////////////////////////////////////////
//
//  Hacked from tif_getimage.c in libtiff in v3.5.7
//
//
typedef unsigned char u_char;

#define DECLAREContigPutFunc(name)                                             \
    static void name(TIFFRGBAImage *img, uint32_t *cp, uint32_t x, uint32_t y, \
                     uint32_t w, uint32_t h, int32_t fromskew, int32_t toskew, \
                     u_char *pp)

#define DECLARESepPutFunc(name)                                                \
    static void name(TIFFRGBAImage *img, uint32_t *cp, uint32_t x, uint32_t y, \
                     uint32_t w, uint32_t h, int32_t fromskew, int32_t toskew, \
                     u_char *r, u_char *g, u_char *b, u_char *a)

DECLAREContigPutFunc(putContig1bitTile);
static int getStripContig1Bit(TIFFRGBAImage *img, uint32_t *uraster, uint32_t w,
                              uint32_t h);

// typedef struct TIFFDibImage {
//     TIFFRGBAImage tif;
//     dibinstalled;
// } TIFFDibImage ;

void DibInstallHack(TIFFDibImage *dib)
{
    TIFFRGBAImage *img = &dib->tif;
    dib->dibinstalled = false;
    switch (img->photometric)
    {
        case PHOTOMETRIC_MINISWHITE:
        case PHOTOMETRIC_MINISBLACK:
            switch (img->bitspersample)
            {
                case 1:
                    img->put.contig = putContig1bitTile;
                    img->get = getStripContig1Bit;
                    dib->dibinstalled = true;
                    break;
            }
            break;
    }
}

/*
 * 1-bit packed samples => 1-bit
 *
 *   Override to just copy the data
 */
DECLAREContigPutFunc(putContig1bitTile)
{
    int samplesperpixel = img->samplesperpixel;

    (void)y;
    fromskew *= samplesperpixel;
    int wb = WIDTHBYTES(w);
    u_char *ucp = (u_char *)cp;

    /* Convert 'w' to bytes from pixels (rounded up) */
    w = (w + 7) / 8;

    while (h-- > 0)
    {
        _TIFFmemcpy(ucp, pp, w);
        /*
        for (x = wb; x-- > 0;) {
            *cp++ = rgbi(Map[pp[0]], Map[pp[1]], Map[pp[2]]);
            pp += samplesperpixel;
        }
        */
        ucp += (wb + toskew);
        pp += (w + fromskew);
    }
}

/*
 *  Hacked from the tif_getimage.c file.
 */
static uint32_t setorientation(TIFFRGBAImage *img, uint32_t h)
{
    TIFF *tif = img->tif;
    uint32_t y;

    switch (img->orientation)
    {
        case ORIENTATION_BOTRIGHT:
        case ORIENTATION_RIGHTBOT: /* XXX */
        case ORIENTATION_LEFTBOT:  /* XXX */
            TIFFWarning(TIFFFileName(tif), "using bottom-left orientation");
            img->orientation = ORIENTATION_BOTLEFT;
        /* fall through... */
        case ORIENTATION_BOTLEFT:
            y = 0;
            break;
        case ORIENTATION_TOPRIGHT:
        case ORIENTATION_RIGHTTOP: /* XXX */
        case ORIENTATION_LEFTTOP:  /* XXX */
        default:
            TIFFWarning(TIFFFileName(tif), "using top-left orientation");
            img->orientation = ORIENTATION_TOPLEFT;
        /* fall through... */
        case ORIENTATION_TOPLEFT:
            y = h - 1;
            break;
    }
    return (y);
}

/*
 * Get a strip-organized image that has
 *  PlanarConfiguration contiguous if SamplesPerPixel > 1
 * or
 *  SamplesPerPixel == 1
 *
 *  Hacked from the tif_getimage.c file.
 *
 *    This is set up to allow us to just copy the data to the raster
 *    for 1-bit bitmaps
 */
static int getStripContig1Bit(TIFFRGBAImage *img, uint32_t *raster, uint32_t w,
                              uint32_t h)
{
    TIFF *tif = img->tif;
    tileContigRoutine put = img->put.contig;
    uint16_t orientation;
    uint32_t row, y, nrow, rowstoread;
    uint32_t pos;
    std::vector<u_char> buf;
    uint32_t rowsperstrip;
    uint32_t imagewidth = img->width;
    tsize_t scanline;
    int32_t fromskew, toskew;
    tstrip_t strip;
    tsize_t stripsize;
    u_char *braster = (u_char *)raster; // byte wide raster
    uint32_t wb = WIDTHBYTES(w);
    int ret = 1;

    buf.resize(TIFFStripSize(tif));
    if (buf.empty())
    {
        TIFFErrorExtR(tif, TIFFFileName(tif), "No space for strip buffer");
        return (0);
    }
    y = setorientation(img, h);
    orientation = img->orientation;
    toskew = -(int32_t)(orientation == ORIENTATION_TOPLEFT ? wb + wb : wb - wb);
    TIFFGetFieldDefaulted(tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
    scanline = TIFFScanlineSize(tif);
    fromskew = (w < imagewidth ? imagewidth - w : 0) / 8;
    for (row = 0; row < h; row += nrow)
    {
        rowstoread = rowsperstrip - (row + img->row_offset) % rowsperstrip;
        nrow = (row + rowstoread > h ? h - row : rowstoread);
        strip = TIFFComputeStrip(tif, row + img->row_offset, 0);
        stripsize = ((row + img->row_offset) % rowsperstrip + nrow) * scanline;
        if (TIFFReadEncodedStrip(tif, strip, buf.data(), stripsize) < 0 &&
            img->stoponerr)
        {
            ret = 0;
            break;
        }

        pos = ((row + img->row_offset) % rowsperstrip) * scanline;
        (*put)(img, (uint32_t *)(braster + y * wb), 0, y, w, nrow, fromskew,
               toskew, buf.data() + pos);
        y += (orientation == ORIENTATION_TOPLEFT ? -(int32_t)nrow
                                                 : (int32_t)nrow);
    }
    return (ret);
}
