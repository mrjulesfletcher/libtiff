#ifndef STRIP_NEON_H
#define STRIP_NEON_H

#include "tiffio.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    uint8_t *TIFFAssembleStripNEON(TIFF *tif, const uint16_t *src,
                                   uint32_t width, uint32_t height,
                                   int apply_predictor, int bigendian,
                                   size_t *out_size);

#ifdef __cplusplus
}
#endif

#endif /* STRIP_NEON_H */
