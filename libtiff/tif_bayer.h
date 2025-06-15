#ifndef TIF_BAYER_H
#define TIF_BAYER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void TIFFPackRaw12(const uint16_t *src, uint8_t *dst, size_t count, int bigendian);
void TIFFUnpackRaw12(const uint8_t *src, uint16_t *dst, size_t count, int bigendian);
void TIFFPackRaw10(const uint16_t *src, uint8_t *dst, size_t count, int bigendian);
void TIFFUnpackRaw10(const uint8_t *src, uint16_t *dst, size_t count, int bigendian);
void TIFFPackRaw14(const uint16_t *src, uint8_t *dst, size_t count, int bigendian);
void TIFFUnpackRaw14(const uint8_t *src, uint16_t *dst, size_t count, int bigendian);
void TIFFPackRaw16(const uint16_t *src, uint8_t *dst, size_t count, int bigendian);
void TIFFUnpackRaw16(const uint8_t *src, uint16_t *dst, size_t count, int bigendian);

#ifdef __cplusplus
}
#endif

#endif /* TIF_BAYER_H */
