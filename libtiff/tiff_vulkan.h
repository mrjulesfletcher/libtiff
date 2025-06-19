#ifndef TIFF_VULKAN_H
#define TIFF_VULKAN_H

#include "tif_config.h"
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef HAVE_VULKAN
#include <vulkan/vulkan.h>
    extern int tiff_use_vulkan;
    int TIFFUseVulkan(void);
    void TIFFSetUseVulkan(int enable);
    void TIFFInitVulkan(void);
    void TIFFCleanupVulkan(void);
    void TIFFYCbCr8ToRGBAVulkan(const unsigned char *src, uint32_t *dst,
                                uint32_t pixel_count);
    void TIFFPredictorDiff16Vulkan(const uint16_t *src, uint16_t *dst,
                                   uint32_t count, uint32_t stride);
#else
static inline void TIFFInitVulkan(void) {}
static inline void TIFFCleanupVulkan(void) {}
static inline int TIFFUseVulkan(void) { return 0; }
static inline void TIFFSetUseVulkan(int enable) { (void)enable; }
static inline void TIFFYCbCr8ToRGBAVulkan(const unsigned char *src,
                                          uint32_t *dst, uint32_t pixel_count)
{
    (void)src;
    (void)dst;
    (void)pixel_count;
}
static inline void TIFFPredictorDiff16Vulkan(const uint16_t *src, uint16_t *dst,
                                             uint32_t count, uint32_t stride)
{
    memcpy(dst, src, count * sizeof(uint16_t));
    for (uint32_t i = stride; i < count; i++)
        dst[i] = src[i] - src[i - stride];
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* TIFF_VULKAN_H */
