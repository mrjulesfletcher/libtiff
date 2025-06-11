#ifndef TIF_PREDICT_NEON_H
#define TIF_PREDICT_NEON_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void TIFFPredictHorDiff16_NEON(uint16_t *line, size_t width);
void TIFFPredictHorAcc16_NEON(uint16_t *line, size_t width);
void TIFFPredictMedianDiff16_NEON(const uint16_t *curr, const uint16_t *prev, uint16_t *dst, size_t width);

#ifdef __cplusplus
}
#endif

#endif /* TIF_PREDICT_NEON_H */
