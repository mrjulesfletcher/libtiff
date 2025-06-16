#include "tiffio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int write_and_read(const char* fname, uint16_t bps, uint16_t spp,
                          uint32_t width, uint32_t height,
                          const uint8_t* buf, size_t size,
                          uint8_t **out)
{
    TIFF *tif = TIFFOpen(fname, "w");
    if (!tif) return 1;
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bps);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, spp);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, height);
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC,
                 spp == 1 ? PHOTOMETRIC_MINISBLACK : PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_PREDICTOR, 2);
    if (TIFFWriteEncodedStrip(tif, 0, (void *)buf, size) == -1) {
        TIFFClose(tif);
        return 1;
    }
    TIFFClose(tif);

    tif = TIFFOpen(fname, "r");
    if (!tif) return 1;
    *out = (uint8_t*)malloc(size);
    if (!*out) {
        TIFFClose(tif);
        return 1;
    }
    tmsize_t n = TIFFReadRawStrip(tif, 0, *out, size);
    TIFFClose(tif);
    if (n != (tmsize_t)size) {
        free(*out);
        return 1;
    }
    return 0;
}

static int test_case(uint16_t bps, uint16_t spp)
{
    const uint32_t width = 32;
    const uint32_t height = 1;
    size_t bytes_per_sample = (bps + 7) / 8;
    size_t size = width * spp * bytes_per_sample;
    uint8_t *data = (uint8_t*)malloc(size);
    if (!data) return 1;
    for (size_t i = 0; i < size; i++)
        data[i] = (uint8_t)(i * 3 + 1);

    int sse = TIFFUseSSE41();

    uint8_t *ref = NULL, *simd = NULL;
    TIFFSetUseSSE41(0);
    if (write_and_read("ref.tif", bps, spp, width, height, data, size, &ref)) {
        free(data);
        return 1;
    }
    TIFFSetUseSSE41(1);
    if (write_and_read("simd.tif", bps, spp, width, height, data, size, &simd)) {
        free(data);
        free(ref);
        return 1;
    }
    TIFFSetUseSSE41(sse);

    int ret = 0;
    if (memcmp(ref, simd, size) != 0)
        ret = 1;

    free(data);
    free(ref);
    free(simd);
    remove("ref.tif");
    remove("simd.tif");
    return ret;
}

int main(void)
{
    TIFFInitSIMD();
    if (test_case(8, 1)) {
        fprintf(stderr, "predictor 8bpp stride1 mismatch\n");
        return 1;
    }
    if (test_case(8, 3)) {
        fprintf(stderr, "predictor 8bpp stride3 mismatch\n");
        return 1;
    }
    if (test_case(8, 4)) {
        fprintf(stderr, "predictor 8bpp stride4 mismatch\n");
        return 1;
    }
    if (test_case(16, 1)) {
        fprintf(stderr, "predictor 16bpp mismatch\n");
        return 1;
    }
    if (test_case(32, 1)) {
        fprintf(stderr, "predictor 32bpp mismatch\n");
        return 1;
    }
    if (test_case(64, 1)) {
        fprintf(stderr, "predictor 64bpp mismatch\n");
        return 1;
    }
    return 0;
}
