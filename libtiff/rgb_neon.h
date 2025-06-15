#ifndef RGB_NEON_H
#define RGB_NEON_H

#include "tiffio.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void TIFFPackRGB24(const uint8_t *src, uint32_t *dst, size_t count);
    void TIFFPackRGBA32(const uint8_t *src, uint32_t *dst, size_t count);
    void TIFFPackRGB48(const uint16_t *src, uint32_t *dst, size_t count);
    void TIFFPackRGBA64(const uint16_t *src, uint32_t *dst, size_t count);

#ifdef __cplusplus
}
#endif

#endif /* RGB_NEON_H */
