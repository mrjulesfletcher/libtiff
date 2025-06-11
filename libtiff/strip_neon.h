#ifndef STRIP_NEON_H
#define STRIP_NEON_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t *TIFFAssembleStripNEON(const uint16_t *src, uint32_t width,
                               uint32_t height, int apply_predictor,
                               int bigendian, size_t *out_size);

#ifdef __cplusplus
}
#endif

#endif /* STRIP_NEON_H */
