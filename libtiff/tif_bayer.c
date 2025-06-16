#include "tif_bayer.h"
#include <string.h>
#include "tiff_simd.h"

#if defined(HAVE_NEON) && defined(__ARM_NEON)
#include <arm_neon.h>
#endif
#if defined(HAVE_SSE41)
#include <smmintrin.h>
#endif

static void pack12_scalar(const uint16_t *src, uint8_t *dst, size_t count,
                          int bigendian)
{
    for (size_t i = 0; i < count; i += 2)
    {
        uint16_t a = src[i];
        uint16_t b = src[i + 1];
        if (!bigendian)
        {
            dst[0] = (uint8_t)(a & 0xff);
            dst[1] = (uint8_t)(((a >> 8) & 0x0f) | ((b & 0x0f) << 4));
            dst[2] = (uint8_t)((b >> 4) & 0xff);
        }
        else
        {
            dst[0] = (uint8_t)((a >> 4) & 0xff);
            dst[1] = (uint8_t)(((a & 0x0f) << 4) | ((b >> 8) & 0x0f));
            dst[2] = (uint8_t)(b & 0xff);
        }
        dst += 3;
    }
}

static void pack10_scalar(const uint16_t *src, uint8_t *dst, size_t count,
                          int bigendian)
{
    size_t i = 0;
    for (; i + 4 <= count; i += 4)
    {
        uint64_t v;
        if (!bigendian)
            v = ((uint64_t)src[i]) |
                ((uint64_t)src[i + 1] << 10) |
                ((uint64_t)src[i + 2] << 20) |
                ((uint64_t)src[i + 3] << 30);
        else
            v = ((uint64_t)src[i] << 30) |
                ((uint64_t)src[i + 1] << 20) |
                ((uint64_t)src[i + 2] << 10) |
                ((uint64_t)src[i + 3]);
        if (!bigendian)
        {
            dst[0] = (uint8_t)(v & 0xff);
            dst[1] = (uint8_t)((v >> 8) & 0xff);
            dst[2] = (uint8_t)((v >> 16) & 0xff);
            dst[3] = (uint8_t)((v >> 24) & 0xff);
            dst[4] = (uint8_t)((v >> 32) & 0xff);
        }
        else
        {
            dst[0] = (uint8_t)((v >> 32) & 0xff);
            dst[1] = (uint8_t)((v >> 24) & 0xff);
            dst[2] = (uint8_t)((v >> 16) & 0xff);
            dst[3] = (uint8_t)((v >> 8) & 0xff);
            dst[4] = (uint8_t)(v & 0xff);
        }
        dst += 5;
    }
    if (i < count)
    {
        uint64_t v = 0;
        size_t remain = count - i;
        if (!bigendian)
        {
            for (size_t k = 0; k < remain; k++)
                v |= ((uint64_t)src[i + k]) << (k * 10);
            size_t bytes = (remain * 10 + 7) / 8;
            for (size_t b = 0; b < bytes; b++)
                dst[b] = (uint8_t)((v >> (8 * b)) & 0xff);
        }
        else
        {
            for (size_t k = 0; k < remain; k++)
                v |= ((uint64_t)src[i + k]) << ((remain - 1 - k) * 10);
            size_t bytes = (remain * 10 + 7) / 8;
            for (size_t b = 0; b < bytes; b++)
                dst[b] = (uint8_t)((v >> (8 * (bytes - 1 - b))) & 0xff);
        }
    }
}

static void unpack10_scalar(const uint8_t *src, uint16_t *dst, size_t count,
                            int bigendian)
{
    size_t i = 0;
    for (; i + 4 <= count; i += 4)
    {
        uint64_t v;
        if (!bigendian)
            v = ((uint64_t)src[0]) |
                ((uint64_t)src[1] << 8) |
                ((uint64_t)src[2] << 16) |
                ((uint64_t)src[3] << 24) |
                ((uint64_t)src[4] << 32);
        else
            v = ((uint64_t)src[0] << 32) |
                ((uint64_t)src[1] << 24) |
                ((uint64_t)src[2] << 16) |
                ((uint64_t)src[3] << 8) |
                ((uint64_t)src[4]);
        if (!bigendian)
        {
            dst[i] = (uint16_t)(v & 0x3ff);
            dst[i + 1] = (uint16_t)((v >> 10) & 0x3ff);
            dst[i + 2] = (uint16_t)((v >> 20) & 0x3ff);
            dst[i + 3] = (uint16_t)((v >> 30) & 0x3ff);
        }
        else
        {
            dst[i] = (uint16_t)((v >> 30) & 0x3ff);
            dst[i + 1] = (uint16_t)((v >> 20) & 0x3ff);
            dst[i + 2] = (uint16_t)((v >> 10) & 0x3ff);
            dst[i + 3] = (uint16_t)(v & 0x3ff);
        }
        src += 5;
    }
    if (i < count)
    {
        size_t remain = count - i;
        uint64_t v = 0;
        if (!bigendian)
        {
            for (size_t b = 0; b < (remain * 10 + 7) / 8; b++)
                v |= ((uint64_t)src[b]) << (8 * b);
            for (size_t k = 0; k < remain; k++)
                dst[i + k] = (uint16_t)((v >> (k * 10)) & 0x3ff);
        }
        else
        {
            size_t bytes = (remain * 10 + 7) / 8;
            for (size_t b = 0; b < bytes; b++)
                v |= ((uint64_t)src[b]) << (8 * (bytes - 1 - b));
            for (size_t k = 0; k < remain; k++)
                dst[i + k] = (uint16_t)((v >> ((remain - 1 - k) * 10)) & 0x3ff);
        }
    }
}

static void pack14_scalar(const uint16_t *src, uint8_t *dst, size_t count,
                          int bigendian)
{
    size_t i = 0;
    for (; i + 4 <= count; i += 4)
    {
        uint64_t v;
        if (!bigendian)
            v = ((uint64_t)src[i]) |
                ((uint64_t)src[i + 1] << 14) |
                ((uint64_t)src[i + 2] << 28) |
                ((uint64_t)src[i + 3] << 42);
        else
            v = ((uint64_t)src[i] << 42) |
                ((uint64_t)src[i + 1] << 28) |
                ((uint64_t)src[i + 2] << 14) |
                ((uint64_t)src[i + 3]);
        if (!bigendian)
        {
            dst[0] = (uint8_t)(v & 0xff);
            dst[1] = (uint8_t)((v >> 8) & 0xff);
            dst[2] = (uint8_t)((v >> 16) & 0xff);
            dst[3] = (uint8_t)((v >> 24) & 0xff);
            dst[4] = (uint8_t)((v >> 32) & 0xff);
            dst[5] = (uint8_t)((v >> 40) & 0xff);
            dst[6] = (uint8_t)((v >> 48) & 0xff);
        }
        else
        {
            dst[0] = (uint8_t)((v >> 48) & 0xff);
            dst[1] = (uint8_t)((v >> 40) & 0xff);
            dst[2] = (uint8_t)((v >> 32) & 0xff);
            dst[3] = (uint8_t)((v >> 24) & 0xff);
            dst[4] = (uint8_t)((v >> 16) & 0xff);
            dst[5] = (uint8_t)((v >> 8) & 0xff);
            dst[6] = (uint8_t)(v & 0xff);
        }
        dst += 7;
    }
    if (i < count)
    {
        uint64_t v = 0;
        size_t remain = count - i;
        if (!bigendian)
        {
            for (size_t k = 0; k < remain; k++)
                v |= ((uint64_t)src[i + k]) << (k * 14);
            size_t bytes = (remain * 14 + 7) / 8;
            for (size_t b = 0; b < bytes; b++)
                dst[b] = (uint8_t)((v >> (8 * b)) & 0xff);
        }
        else
        {
            for (size_t k = 0; k < remain; k++)
                v |= ((uint64_t)src[i + k]) << ((remain - 1 - k) * 14);
            size_t bytes = (remain * 14 + 7) / 8;
            for (size_t b = 0; b < bytes; b++)
                dst[b] = (uint8_t)((v >> (8 * (bytes - 1 - b))) & 0xff);
        }
    }
}

static void unpack14_scalar(const uint8_t *src, uint16_t *dst, size_t count,
                            int bigendian)
{
    size_t i = 0;
    for (; i + 4 <= count; i += 4)
    {
        uint64_t v;
        if (!bigendian)
            v = ((uint64_t)src[0]) |
                ((uint64_t)src[1] << 8) |
                ((uint64_t)src[2] << 16) |
                ((uint64_t)src[3] << 24) |
                ((uint64_t)src[4] << 32) |
                ((uint64_t)src[5] << 40) |
                ((uint64_t)src[6] << 48);
        else
            v = ((uint64_t)src[0] << 48) |
                ((uint64_t)src[1] << 40) |
                ((uint64_t)src[2] << 32) |
                ((uint64_t)src[3] << 24) |
                ((uint64_t)src[4] << 16) |
                ((uint64_t)src[5] << 8) |
                ((uint64_t)src[6]);
        if (!bigendian)
        {
            dst[i] = (uint16_t)(v & 0x3fff);
            dst[i + 1] = (uint16_t)((v >> 14) & 0x3fff);
            dst[i + 2] = (uint16_t)((v >> 28) & 0x3fff);
            dst[i + 3] = (uint16_t)((v >> 42) & 0x3fff);
        }
        else
        {
            dst[i] = (uint16_t)((v >> 42) & 0x3fff);
            dst[i + 1] = (uint16_t)((v >> 28) & 0x3fff);
            dst[i + 2] = (uint16_t)((v >> 14) & 0x3fff);
            dst[i + 3] = (uint16_t)(v & 0x3fff);
        }
        src += 7;
    }
    if (i < count)
    {
        size_t remain = count - i;
        uint64_t v = 0;
        if (!bigendian)
        {
            for (size_t b = 0; b < (remain * 14 + 7) / 8; b++)
                v |= ((uint64_t)src[b]) << (8 * b);
            for (size_t k = 0; k < remain; k++)
                dst[i + k] = (uint16_t)((v >> (k * 14)) & 0x3fff);
        }
        else
        {
            size_t bytes = (remain * 14 + 7) / 8;
            for (size_t b = 0; b < bytes; b++)
                v |= ((uint64_t)src[b]) << (8 * (bytes - 1 - b));
            for (size_t k = 0; k < remain; k++)
                dst[i + k] =
                    (uint16_t)((v >> ((remain - 1 - k) * 14)) & 0x3fff);
        }
    }
}

static void pack16_scalar(const uint16_t *src, uint8_t *dst, size_t count,
                          int bigendian)
{
    for (size_t i = 0; i < count; i++)
    {
        uint16_t v = src[i];
        if (!bigendian)
        {
            dst[0] = (uint8_t)(v & 0xff);
            dst[1] = (uint8_t)((v >> 8) & 0xff);
        }
        else
        {
            dst[0] = (uint8_t)((v >> 8) & 0xff);
            dst[1] = (uint8_t)(v & 0xff);
        }
        dst += 2;
    }
}

static void unpack16_scalar(const uint8_t *src, uint16_t *dst, size_t count,
                            int bigendian)
{
    for (size_t i = 0; i < count; i++)
    {
        if (!bigendian)
            dst[i] = (uint16_t)(src[0] | (src[1] << 8));
        else
            dst[i] = (uint16_t)((src[0] << 8) | src[1]);
        src += 2;
    }
}

static void unpack12_scalar(const uint8_t *src, uint16_t *dst, size_t count,
                            int bigendian)
{
    for (size_t i = 0; i < count; i += 2)
    {
        if (!bigendian)
        {
            dst[i] = (uint16_t)(src[0] | ((src[1] & 0x0f) << 8));
            dst[i + 1] = (uint16_t)((src[1] >> 4) | (src[2] << 4));
        }
        else
        {
            dst[i] = (uint16_t)((src[0] << 4) | (src[1] >> 4));
            dst[i + 1] = (uint16_t)(((src[1] & 0x0f) << 8) | src[2]);
        }
        src += 3;
    }
}

#if defined(HAVE_NEON) && defined(__ARM_NEON)
static void pack10_neon(const uint16_t *src, uint8_t *dst, size_t count,
                        int bigendian)
{
    size_t i = 0;
    for (; i + 8 <= count; i += 8)
    {
        __builtin_prefetch(src + i + 16);
        uint16x8_t v = vld1q_u16(src + i);
        uint64_t p0, p1;
        if (!bigendian)
        {
            p0 = ((uint64_t)vgetq_lane_u16(v, 0)) |
                 ((uint64_t)vgetq_lane_u16(v, 1) << 10) |
                 ((uint64_t)vgetq_lane_u16(v, 2) << 20) |
                 ((uint64_t)vgetq_lane_u16(v, 3) << 30);
            p1 = ((uint64_t)vgetq_lane_u16(v, 4)) |
                 ((uint64_t)vgetq_lane_u16(v, 5) << 10) |
                 ((uint64_t)vgetq_lane_u16(v, 6) << 20) |
                 ((uint64_t)vgetq_lane_u16(v, 7) << 30);
            dst[0] = (uint8_t)(p0 & 0xff);
            dst[1] = (uint8_t)((p0 >> 8) & 0xff);
            dst[2] = (uint8_t)((p0 >> 16) & 0xff);
            dst[3] = (uint8_t)((p0 >> 24) & 0xff);
            dst[4] = (uint8_t)((p0 >> 32) & 0xff);
            dst[5] = (uint8_t)(p1 & 0xff);
            dst[6] = (uint8_t)((p1 >> 8) & 0xff);
            dst[7] = (uint8_t)((p1 >> 16) & 0xff);
            dst[8] = (uint8_t)((p1 >> 24) & 0xff);
            dst[9] = (uint8_t)((p1 >> 32) & 0xff);
        }
        else
        {
            p0 = ((uint64_t)vgetq_lane_u16(v, 0) << 30) |
                 ((uint64_t)vgetq_lane_u16(v, 1) << 20) |
                 ((uint64_t)vgetq_lane_u16(v, 2) << 10) |
                 ((uint64_t)vgetq_lane_u16(v, 3));
            p1 = ((uint64_t)vgetq_lane_u16(v, 4) << 30) |
                 ((uint64_t)vgetq_lane_u16(v, 5) << 20) |
                 ((uint64_t)vgetq_lane_u16(v, 6) << 10) |
                 ((uint64_t)vgetq_lane_u16(v, 7));
            dst[0] = (uint8_t)((p0 >> 32) & 0xff);
            dst[1] = (uint8_t)((p0 >> 24) & 0xff);
            dst[2] = (uint8_t)((p0 >> 16) & 0xff);
            dst[3] = (uint8_t)((p0 >> 8) & 0xff);
            dst[4] = (uint8_t)(p0 & 0xff);
            dst[5] = (uint8_t)((p1 >> 32) & 0xff);
            dst[6] = (uint8_t)((p1 >> 24) & 0xff);
            dst[7] = (uint8_t)((p1 >> 16) & 0xff);
            dst[8] = (uint8_t)((p1 >> 8) & 0xff);
            dst[9] = (uint8_t)(p1 & 0xff);
        }
        dst += 10;
    }
    if (i < count)
        pack10_scalar(src + i, dst, count - i, bigendian);
}

static void unpack10_neon(const uint8_t *src, uint16_t *dst, size_t count,
                          int bigendian)
{
    size_t i = 0;
    for (; i + 8 <= count; i += 8)
    {
        uint64_t p0, p1;
        if (!bigendian)
        {
            p0 = ((uint64_t)src[0]) |
                 ((uint64_t)src[1] << 8) |
                 ((uint64_t)src[2] << 16) |
                 ((uint64_t)src[3] << 24) |
                 ((uint64_t)src[4] << 32);
            p1 = ((uint64_t)src[5]) |
                 ((uint64_t)src[6] << 8) |
                 ((uint64_t)src[7] << 16) |
                 ((uint64_t)src[8] << 24) |
                 ((uint64_t)src[9] << 32);
        }
        else
        {
            p0 = ((uint64_t)src[0] << 32) |
                 ((uint64_t)src[1] << 24) |
                 ((uint64_t)src[2] << 16) |
                 ((uint64_t)src[3] << 8) |
                 ((uint64_t)src[4]);
            p1 = ((uint64_t)src[5] << 32) |
                 ((uint64_t)src[6] << 24) |
                 ((uint64_t)src[7] << 16) |
                 ((uint64_t)src[8] << 8) |
                 ((uint64_t)src[9]);
        }
        uint16x8_t out = { (uint16_t)(p0 & 0x3ff),
                           (uint16_t)((p0 >> 10) & 0x3ff),
                           (uint16_t)((p0 >> 20) & 0x3ff),
                           (uint16_t)((p0 >> 30) & 0x3ff),
                           (uint16_t)(p1 & 0x3ff),
                           (uint16_t)((p1 >> 10) & 0x3ff),
                           (uint16_t)((p1 >> 20) & 0x3ff),
                           (uint16_t)((p1 >> 30) & 0x3ff) };
        vst1q_u16(dst + i, out);
        src += 10;
    }
    if (i < count)
        unpack10_scalar(src, dst + i, count - i, bigendian);
}

static void pack14_neon(const uint16_t *src, uint8_t *dst, size_t count,
                        int bigendian)
{
    size_t i = 0;
    for (; i + 8 <= count; i += 8)
    {
        __builtin_prefetch(src + i + 16);
        uint16x8_t v = vld1q_u16(src + i);
        uint64_t p0, p1;
        if (!bigendian)
        {
            p0 = ((uint64_t)vgetq_lane_u16(v, 0)) |
                 ((uint64_t)vgetq_lane_u16(v, 1) << 14) |
                 ((uint64_t)vgetq_lane_u16(v, 2) << 28) |
                 ((uint64_t)vgetq_lane_u16(v, 3) << 42);
            p1 = ((uint64_t)vgetq_lane_u16(v, 4)) |
                 ((uint64_t)vgetq_lane_u16(v, 5) << 14) |
                 ((uint64_t)vgetq_lane_u16(v, 6) << 28) |
                 ((uint64_t)vgetq_lane_u16(v, 7) << 42);
            dst[0] = (uint8_t)(p0 & 0xff);
            dst[1] = (uint8_t)((p0 >> 8) & 0xff);
            dst[2] = (uint8_t)((p0 >> 16) & 0xff);
            dst[3] = (uint8_t)((p0 >> 24) & 0xff);
            dst[4] = (uint8_t)((p0 >> 32) & 0xff);
            dst[5] = (uint8_t)((p0 >> 40) & 0xff);
            dst[6] = (uint8_t)((p0 >> 48) & 0xff);
            dst[7] = (uint8_t)(p1 & 0xff);
            dst[8] = (uint8_t)((p1 >> 8) & 0xff);
            dst[9] = (uint8_t)((p1 >> 16) & 0xff);
            dst[10] = (uint8_t)((p1 >> 24) & 0xff);
            dst[11] = (uint8_t)((p1 >> 32) & 0xff);
            dst[12] = (uint8_t)((p1 >> 40) & 0xff);
            dst[13] = (uint8_t)((p1 >> 48) & 0xff);
        }
        else
        {
            p0 = ((uint64_t)vgetq_lane_u16(v, 0) << 42) |
                 ((uint64_t)vgetq_lane_u16(v, 1) << 28) |
                 ((uint64_t)vgetq_lane_u16(v, 2) << 14) |
                 ((uint64_t)vgetq_lane_u16(v, 3));
            p1 = ((uint64_t)vgetq_lane_u16(v, 4) << 42) |
                 ((uint64_t)vgetq_lane_u16(v, 5) << 28) |
                 ((uint64_t)vgetq_lane_u16(v, 6) << 14) |
                 ((uint64_t)vgetq_lane_u16(v, 7));
            dst[0] = (uint8_t)((p0 >> 48) & 0xff);
            dst[1] = (uint8_t)((p0 >> 40) & 0xff);
            dst[2] = (uint8_t)((p0 >> 32) & 0xff);
            dst[3] = (uint8_t)((p0 >> 24) & 0xff);
            dst[4] = (uint8_t)((p0 >> 16) & 0xff);
            dst[5] = (uint8_t)((p0 >> 8) & 0xff);
            dst[6] = (uint8_t)(p0 & 0xff);
            dst[7] = (uint8_t)((p1 >> 48) & 0xff);
            dst[8] = (uint8_t)((p1 >> 40) & 0xff);
            dst[9] = (uint8_t)((p1 >> 32) & 0xff);
            dst[10] = (uint8_t)((p1 >> 24) & 0xff);
            dst[11] = (uint8_t)((p1 >> 16) & 0xff);
            dst[12] = (uint8_t)((p1 >> 8) & 0xff);
            dst[13] = (uint8_t)(p1 & 0xff);
        }
        dst += 14;
    }
    if (i < count)
        pack14_scalar(src + i, dst, count - i, bigendian);
}

static void unpack14_neon(const uint8_t *src, uint16_t *dst, size_t count,
                          int bigendian)
{
    size_t i = 0;
    for (; i + 8 <= count; i += 8)
    {
        uint64_t p0, p1;
        if (!bigendian)
        {
            p0 = ((uint64_t)src[0]) |
                 ((uint64_t)src[1] << 8) |
                 ((uint64_t)src[2] << 16) |
                 ((uint64_t)src[3] << 24) |
                 ((uint64_t)src[4] << 32) |
                 ((uint64_t)src[5] << 40) |
                 ((uint64_t)src[6] << 48);
            p1 = ((uint64_t)src[7]) |
                 ((uint64_t)src[8] << 8) |
                 ((uint64_t)src[9] << 16) |
                 ((uint64_t)src[10] << 24) |
                 ((uint64_t)src[11] << 32) |
                 ((uint64_t)src[12] << 40) |
                 ((uint64_t)src[13] << 48);
        }
        else
        {
            p0 = ((uint64_t)src[0] << 48) |
                 ((uint64_t)src[1] << 40) |
                 ((uint64_t)src[2] << 32) |
                 ((uint64_t)src[3] << 24) |
                 ((uint64_t)src[4] << 16) |
                 ((uint64_t)src[5] << 8) |
                 ((uint64_t)src[6]);
            p1 = ((uint64_t)src[7] << 48) |
                 ((uint64_t)src[8] << 40) |
                 ((uint64_t)src[9] << 32) |
                 ((uint64_t)src[10] << 24) |
                 ((uint64_t)src[11] << 16) |
                 ((uint64_t)src[12] << 8) |
                 ((uint64_t)src[13]);
        }
        uint16x8_t out = { (uint16_t)(p0 & 0x3fff),
                           (uint16_t)((p0 >> 14) & 0x3fff),
                           (uint16_t)((p0 >> 28) & 0x3fff),
                           (uint16_t)((p0 >> 42) & 0x3fff),
                           (uint16_t)(p1 & 0x3fff),
                           (uint16_t)((p1 >> 14) & 0x3fff),
                           (uint16_t)((p1 >> 28) & 0x3fff),
                           (uint16_t)((p1 >> 42) & 0x3fff) };
        vst1q_u16(dst + i, out);
        src += 14;
    }
    if (i < count)
        unpack14_scalar(src, dst + i, count - i, bigendian);
}

static void pack16_neon(const uint16_t *src, uint8_t *dst, size_t count,
                        int bigendian)
{
    size_t i = 0;
    for (; i + 8 <= count; i += 8)
    {
        __builtin_prefetch(src + i + 16);
        uint16x8_t v = vld1q_u16(src + i);
        uint8x8_t lo = vmovn_u16(v);
        uint8x8_t hi = vshrn_n_u16(v, 8);
        uint8x8x2_t outv;
        if (!bigendian)
        {
            outv.val[0] = lo;
            outv.val[1] = hi;
        }
        else
        {
            outv.val[0] = hi;
            outv.val[1] = lo;
        }
        vst2_u8(dst, outv);
        dst += 16;
    }
    if (i < count)
        pack16_scalar(src + i, dst, count - i, bigendian);
}

static void unpack16_neon(const uint8_t *src, uint16_t *dst, size_t count,
                          int bigendian)
{
    size_t i = 0;
    for (; i + 8 <= count; i += 8)
    {
        __builtin_prefetch(src + 16);
        uint8x8x2_t v = vld2_u8(src);
        uint16x8_t out;
        if (!bigendian)
            out = vorrq_u16(vmovl_u8(v.val[0]), vshll_n_u8(v.val[1], 8));
        else
            out = vorrq_u16(vshll_n_u8(v.val[0], 8), vmovl_u8(v.val[1]));
        vst1q_u16(dst + i, out);
        src += 16;
    }
    if (i < count)
        unpack16_scalar(src, dst + i, count - i, bigendian);
}

static void pack12_neon(const uint16_t *src, uint8_t *dst, size_t count,
                        int bigendian)
{
    size_t i = 0;
    uint8x8_t mask4 = vdup_n_u8(0x0f);
    for (; i + 16 <= count; i += 16)
    {
        __builtin_prefetch(src + i + 32);
        uint16x8x2_t v = vld2q_u16(src + i);
        uint16x8_t even = v.val[0];
        uint16x8_t odd = v.val[1];

        uint8x8_t even_lo = vmovn_u16(even);
        uint8x8_t even_mid = vshrn_n_u16(even, 4);
        uint8x8_t even_hi = vshrn_n_u16(even, 8);
        uint8x8_t odd_lo = vmovn_u16(odd);
        uint8x8_t odd_mid = vshrn_n_u16(odd, 4);
        uint8x8_t odd_hi = vshrn_n_u16(odd, 8);

        uint8x8x3_t outv;
        if (!bigendian)
        {
            outv.val[0] = even_lo;
            outv.val[1] = vorr_u8(vand_u8(even_hi, mask4),
                                  vshl_n_u8(vand_u8(odd_lo, mask4), 4));
            outv.val[2] = odd_mid;
        }
        else
        {
            outv.val[0] = even_mid;
            outv.val[1] = vorr_u8(vshl_n_u8(vand_u8(even_lo, mask4), 4),
                                  vand_u8(odd_hi, mask4));
            outv.val[2] = odd_lo;
        }
        vst3_u8(dst, outv);
        dst += 24;
    }
    if (i < count)
        pack12_scalar(src + i, dst, count - i, bigendian);
}

static void unpack12_neon(const uint8_t *src, uint16_t *dst, size_t count,
                          int bigendian)
{
    size_t i = 0;
    uint8x8_t mask4 = vdup_n_u8(0x0f);
    for (; i + 16 <= count; i += 16)
    {
        __builtin_prefetch(src + 32);
        uint8x8x3_t v = vld3_u8(src);
        uint16x8_t out0, out1;
        if (!bigendian)
        {
            out0 = vorrq_u16(vmovl_u8(v.val[0]),
                             vshll_n_u8(vand_u8(v.val[1], mask4), 8));
            out1 = vorrq_u16(vshll_n_u8(vshr_n_u8(v.val[1], 4), 4),
                             vshll_n_u8(v.val[2], 4));
        }
        else
        {
            out0 = vorrq_u16(vshll_n_u8(v.val[0], 4),
                             vshll_n_u8(vshr_n_u8(v.val[1], 4), 0));
            out1 = vorrq_u16(vshll_n_u8(vand_u8(v.val[1], mask4), 8),
                             vmovl_u8(v.val[2]));
        }
        uint16x8x2_t res = {out0, out1};
        vst2q_u16(dst + i, res);
        src += 24;
    }
    if (i < count)
        unpack12_scalar(src, dst + i, count - i, bigendian);
}
#endif /* HAVE_NEON */

#if defined(HAVE_SSE41)
static void pack10_sse41(const uint16_t *src, uint8_t *dst, size_t count,
                         int bigendian)
{
    size_t i = 0;
    for (; i + 8 <= count; i += 8)
    {
        __m128i v = _mm_loadu_si128((const __m128i *)(src + i));
        uint64_t p0, p1;
        if (!bigendian)
        {
            p0 = ((uint64_t)_mm_extract_epi16(v, 0)) |
                 ((uint64_t)_mm_extract_epi16(v, 1) << 10) |
                 ((uint64_t)_mm_extract_epi16(v, 2) << 20) |
                 ((uint64_t)_mm_extract_epi16(v, 3) << 30);
            p1 = ((uint64_t)_mm_extract_epi16(v, 4)) |
                 ((uint64_t)_mm_extract_epi16(v, 5) << 10) |
                 ((uint64_t)_mm_extract_epi16(v, 6) << 20) |
                 ((uint64_t)_mm_extract_epi16(v, 7) << 30);
            dst[0] = (uint8_t)(p0 & 0xff);
            dst[1] = (uint8_t)((p0 >> 8) & 0xff);
            dst[2] = (uint8_t)((p0 >> 16) & 0xff);
            dst[3] = (uint8_t)((p0 >> 24) & 0xff);
            dst[4] = (uint8_t)((p0 >> 32) & 0xff);
            dst[5] = (uint8_t)(p1 & 0xff);
            dst[6] = (uint8_t)((p1 >> 8) & 0xff);
            dst[7] = (uint8_t)((p1 >> 16) & 0xff);
            dst[8] = (uint8_t)((p1 >> 24) & 0xff);
            dst[9] = (uint8_t)((p1 >> 32) & 0xff);
        }
        else
        {
            p0 = ((uint64_t)_mm_extract_epi16(v, 0) << 30) |
                 ((uint64_t)_mm_extract_epi16(v, 1) << 20) |
                 ((uint64_t)_mm_extract_epi16(v, 2) << 10) |
                 ((uint64_t)_mm_extract_epi16(v, 3));
            p1 = ((uint64_t)_mm_extract_epi16(v, 4) << 30) |
                 ((uint64_t)_mm_extract_epi16(v, 5) << 20) |
                 ((uint64_t)_mm_extract_epi16(v, 6) << 10) |
                 ((uint64_t)_mm_extract_epi16(v, 7));
            dst[0] = (uint8_t)((p0 >> 32) & 0xff);
            dst[1] = (uint8_t)((p0 >> 24) & 0xff);
            dst[2] = (uint8_t)((p0 >> 16) & 0xff);
            dst[3] = (uint8_t)((p0 >> 8) & 0xff);
            dst[4] = (uint8_t)(p0 & 0xff);
            dst[5] = (uint8_t)((p1 >> 32) & 0xff);
            dst[6] = (uint8_t)((p1 >> 24) & 0xff);
            dst[7] = (uint8_t)((p1 >> 16) & 0xff);
            dst[8] = (uint8_t)((p1 >> 8) & 0xff);
            dst[9] = (uint8_t)(p1 & 0xff);
        }
        dst += 10;
    }
    if (i < count)
        pack10_scalar(src + i, dst, count - i, bigendian);
}

static void unpack10_sse41(const uint8_t *src, uint16_t *dst, size_t count,
                           int bigendian)
{
    size_t i = 0;
    for (; i + 8 <= count; i += 8)
    {
        uint64_t p0, p1;
        if (!bigendian)
        {
            p0 = ((uint64_t)src[0]) |
                 ((uint64_t)src[1] << 8) |
                 ((uint64_t)src[2] << 16) |
                 ((uint64_t)src[3] << 24) |
                 ((uint64_t)src[4] << 32);
            p1 = ((uint64_t)src[5]) |
                 ((uint64_t)src[6] << 8) |
                 ((uint64_t)src[7] << 16) |
                 ((uint64_t)src[8] << 24) |
                 ((uint64_t)src[9] << 32);
        }
        else
        {
            p0 = ((uint64_t)src[0] << 32) |
                 ((uint64_t)src[1] << 24) |
                 ((uint64_t)src[2] << 16) |
                 ((uint64_t)src[3] << 8) |
                 ((uint64_t)src[4]);
            p1 = ((uint64_t)src[5] << 32) |
                 ((uint64_t)src[6] << 24) |
                 ((uint64_t)src[7] << 16) |
                 ((uint64_t)src[8] << 8) |
                 ((uint64_t)src[9]);
        }
        __m128i out = _mm_set_epi16((uint16_t)((p1 >> 30) & 0x3ff),
                                    (uint16_t)((p1 >> 20) & 0x3ff),
                                    (uint16_t)((p1 >> 10) & 0x3ff),
                                    (uint16_t)(p1 & 0x3ff),
                                    (uint16_t)((p0 >> 30) & 0x3ff),
                                    (uint16_t)((p0 >> 20) & 0x3ff),
                                    (uint16_t)((p0 >> 10) & 0x3ff),
                                    (uint16_t)(p0 & 0x3ff));
        _mm_storeu_si128((__m128i *)(dst + i), out);
        src += 10;
    }
    if (i < count)
        unpack10_scalar(src, dst + i, count - i, bigendian);
}

static void pack14_sse41(const uint16_t *src, uint8_t *dst, size_t count,
                         int bigendian)
{
    size_t i = 0;
    for (; i + 8 <= count; i += 8)
    {
        __m128i v = _mm_loadu_si128((const __m128i *)(src + i));
        uint64_t p0, p1;
        if (!bigendian)
        {
            p0 = ((uint64_t)_mm_extract_epi16(v, 0)) |
                 ((uint64_t)_mm_extract_epi16(v, 1) << 14) |
                 ((uint64_t)_mm_extract_epi16(v, 2) << 28) |
                 ((uint64_t)_mm_extract_epi16(v, 3) << 42);
            p1 = ((uint64_t)_mm_extract_epi16(v, 4)) |
                 ((uint64_t)_mm_extract_epi16(v, 5) << 14) |
                 ((uint64_t)_mm_extract_epi16(v, 6) << 28) |
                 ((uint64_t)_mm_extract_epi16(v, 7) << 42);
            dst[0] = (uint8_t)(p0 & 0xff);
            dst[1] = (uint8_t)((p0 >> 8) & 0xff);
            dst[2] = (uint8_t)((p0 >> 16) & 0xff);
            dst[3] = (uint8_t)((p0 >> 24) & 0xff);
            dst[4] = (uint8_t)((p0 >> 32) & 0xff);
            dst[5] = (uint8_t)((p0 >> 40) & 0xff);
            dst[6] = (uint8_t)((p0 >> 48) & 0xff);
            dst[7] = (uint8_t)(p1 & 0xff);
            dst[8] = (uint8_t)((p1 >> 8) & 0xff);
            dst[9] = (uint8_t)((p1 >> 16) & 0xff);
            dst[10] = (uint8_t)((p1 >> 24) & 0xff);
            dst[11] = (uint8_t)((p1 >> 32) & 0xff);
            dst[12] = (uint8_t)((p1 >> 40) & 0xff);
            dst[13] = (uint8_t)((p1 >> 48) & 0xff);
        }
        else
        {
            p0 = ((uint64_t)_mm_extract_epi16(v, 0) << 42) |
                 ((uint64_t)_mm_extract_epi16(v, 1) << 28) |
                 ((uint64_t)_mm_extract_epi16(v, 2) << 14) |
                 ((uint64_t)_mm_extract_epi16(v, 3));
            p1 = ((uint64_t)_mm_extract_epi16(v, 4) << 42) |
                 ((uint64_t)_mm_extract_epi16(v, 5) << 28) |
                 ((uint64_t)_mm_extract_epi16(v, 6) << 14) |
                 ((uint64_t)_mm_extract_epi16(v, 7));
            dst[0] = (uint8_t)((p0 >> 48) & 0xff);
            dst[1] = (uint8_t)((p0 >> 40) & 0xff);
            dst[2] = (uint8_t)((p0 >> 32) & 0xff);
            dst[3] = (uint8_t)((p0 >> 24) & 0xff);
            dst[4] = (uint8_t)((p0 >> 16) & 0xff);
            dst[5] = (uint8_t)((p0 >> 8) & 0xff);
            dst[6] = (uint8_t)(p0 & 0xff);
            dst[7] = (uint8_t)((p1 >> 48) & 0xff);
            dst[8] = (uint8_t)((p1 >> 40) & 0xff);
            dst[9] = (uint8_t)((p1 >> 32) & 0xff);
            dst[10] = (uint8_t)((p1 >> 24) & 0xff);
            dst[11] = (uint8_t)((p1 >> 16) & 0xff);
            dst[12] = (uint8_t)((p1 >> 8) & 0xff);
            dst[13] = (uint8_t)(p1 & 0xff);
        }
        dst += 14;
    }
    if (i < count)
        pack14_scalar(src + i, dst, count - i, bigendian);
}

static void unpack14_sse41(const uint8_t *src, uint16_t *dst, size_t count,
                           int bigendian)
{
    size_t i = 0;
    for (; i + 8 <= count; i += 8)
    {
        uint64_t p0, p1;
        if (!bigendian)
        {
            p0 = ((uint64_t)src[0]) |
                 ((uint64_t)src[1] << 8) |
                 ((uint64_t)src[2] << 16) |
                 ((uint64_t)src[3] << 24) |
                 ((uint64_t)src[4] << 32) |
                 ((uint64_t)src[5] << 40) |
                 ((uint64_t)src[6] << 48);
            p1 = ((uint64_t)src[7]) |
                 ((uint64_t)src[8] << 8) |
                 ((uint64_t)src[9] << 16) |
                 ((uint64_t)src[10] << 24) |
                 ((uint64_t)src[11] << 32) |
                 ((uint64_t)src[12] << 40) |
                 ((uint64_t)src[13] << 48);
        }
        else
        {
            p0 = ((uint64_t)src[0] << 48) |
                 ((uint64_t)src[1] << 40) |
                 ((uint64_t)src[2] << 32) |
                 ((uint64_t)src[3] << 24) |
                 ((uint64_t)src[4] << 16) |
                 ((uint64_t)src[5] << 8) |
                 ((uint64_t)src[6]);
            p1 = ((uint64_t)src[7] << 48) |
                 ((uint64_t)src[8] << 40) |
                 ((uint64_t)src[9] << 32) |
                 ((uint64_t)src[10] << 24) |
                 ((uint64_t)src[11] << 16) |
                 ((uint64_t)src[12] << 8) |
                 ((uint64_t)src[13]);
        }
        __m128i out = _mm_set_epi16((uint16_t)((p1 >> 42) & 0x3fff),
                                    (uint16_t)((p1 >> 28) & 0x3fff),
                                    (uint16_t)((p1 >> 14) & 0x3fff),
                                    (uint16_t)(p1 & 0x3fff),
                                    (uint16_t)((p0 >> 42) & 0x3fff),
                                    (uint16_t)((p0 >> 28) & 0x3fff),
                                    (uint16_t)((p0 >> 14) & 0x3fff),
                                    (uint16_t)(p0 & 0x3fff));
        _mm_storeu_si128((__m128i *)(dst + i), out);
        src += 14;
    }
    if (i < count)
        unpack14_scalar(src, dst + i, count - i, bigendian);
}

static void pack16_sse41(const uint16_t *src, uint8_t *dst, size_t count,
                         int bigendian)
{
    size_t i = 0;
    for (; i + 8 <= count; i += 8)
    {
        __m128i v = _mm_loadu_si128((const __m128i *)(src + i));
        if (!bigendian)
            _mm_storeu_si128((__m128i *)dst, v);
        else
        {
            __m128i swapped = _mm_or_si128(_mm_slli_epi16(v, 8),
                                           _mm_srli_epi16(v, 8));
            _mm_storeu_si128((__m128i *)dst, swapped);
        }
        dst += 16;
    }
    if (i < count)
        pack16_scalar(src + i, dst, count - i, bigendian);
}

static void unpack16_sse41(const uint8_t *src, uint16_t *dst, size_t count,
                           int bigendian)
{
    size_t i = 0;
    for (; i + 8 <= count; i += 8)
    {
        __m128i v = _mm_loadu_si128((const __m128i *)src);
        if (!bigendian)
            _mm_storeu_si128((__m128i *)(dst + i), v);
        else
        {
            __m128i swapped = _mm_or_si128(_mm_slli_epi16(v, 8),
                                           _mm_srli_epi16(v, 8));
            _mm_storeu_si128((__m128i *)(dst + i), swapped);
        }
        src += 16;
    }
    if (i < count)
        unpack16_scalar(src, dst + i, count - i, bigendian);
}

static void pack12_sse41(const uint16_t *src, uint8_t *dst, size_t count,
                         int bigendian)
{
    size_t i = 0;
    for (; i + 8 <= count; i += 8)
    {
        __m128i v = _mm_loadu_si128((const __m128i *)(src + i));
        uint64_t p0, p1;
        if (!bigendian)
        {
            p0 = ((uint64_t)_mm_extract_epi16(v, 0)) |
                 ((uint64_t)_mm_extract_epi16(v, 1) << 12) |
                 ((uint64_t)_mm_extract_epi16(v, 2) << 24) |
                 ((uint64_t)_mm_extract_epi16(v, 3) << 36);
            p1 = ((uint64_t)_mm_extract_epi16(v, 4)) |
                 ((uint64_t)_mm_extract_epi16(v, 5) << 12) |
                 ((uint64_t)_mm_extract_epi16(v, 6) << 24) |
                 ((uint64_t)_mm_extract_epi16(v, 7) << 36);
            dst[0] = (uint8_t)(p0 & 0xff);
            dst[1] = (uint8_t)((p0 >> 8) & 0xff);
            dst[2] = (uint8_t)((p0 >> 16) & 0xff);
            dst[3] = (uint8_t)((p0 >> 24) & 0xff);
            dst[4] = (uint8_t)((p0 >> 32) & 0xff);
            dst[5] = (uint8_t)((p0 >> 40) & 0xff);
            dst[6] = (uint8_t)(p1 & 0xff);
            dst[7] = (uint8_t)((p1 >> 8) & 0xff);
            dst[8] = (uint8_t)((p1 >> 16) & 0xff);
            dst[9] = (uint8_t)((p1 >> 24) & 0xff);
            dst[10] = (uint8_t)((p1 >> 32) & 0xff);
            dst[11] = (uint8_t)((p1 >> 40) & 0xff);
        }
        else
        {
            p0 = ((uint64_t)_mm_extract_epi16(v, 0) << 36) |
                 ((uint64_t)_mm_extract_epi16(v, 1) << 24) |
                 ((uint64_t)_mm_extract_epi16(v, 2) << 12) |
                 ((uint64_t)_mm_extract_epi16(v, 3));
            p1 = ((uint64_t)_mm_extract_epi16(v, 4) << 36) |
                 ((uint64_t)_mm_extract_epi16(v, 5) << 24) |
                 ((uint64_t)_mm_extract_epi16(v, 6) << 12) |
                 ((uint64_t)_mm_extract_epi16(v, 7));
            dst[0] = (uint8_t)((p0 >> 40) & 0xff);
            dst[1] = (uint8_t)((p0 >> 32) & 0xff);
            dst[2] = (uint8_t)((p0 >> 24) & 0xff);
            dst[3] = (uint8_t)((p0 >> 16) & 0xff);
            dst[4] = (uint8_t)((p0 >> 8) & 0xff);
            dst[5] = (uint8_t)(p0 & 0xff);
            dst[6] = (uint8_t)((p1 >> 40) & 0xff);
            dst[7] = (uint8_t)((p1 >> 32) & 0xff);
            dst[8] = (uint8_t)((p1 >> 24) & 0xff);
            dst[9] = (uint8_t)((p1 >> 16) & 0xff);
            dst[10] = (uint8_t)((p1 >> 8) & 0xff);
            dst[11] = (uint8_t)(p1 & 0xff);
        }
        dst += 12;
    }
    if (i < count)
        pack12_scalar(src + i, dst, count - i, bigendian);
}

static void unpack12_sse41(const uint8_t *src, uint16_t *dst, size_t count,
                           int bigendian)
{
    size_t i = 0;
    for (; i + 8 <= count; i += 8)
    {
        uint64_t p0, p1;
        if (!bigendian)
        {
            p0 = ((uint64_t)src[0]) |
                 ((uint64_t)src[1] << 8) |
                 ((uint64_t)src[2] << 16) |
                 ((uint64_t)src[3] << 24) |
                 ((uint64_t)src[4] << 32) |
                 ((uint64_t)src[5] << 40);
            p1 = ((uint64_t)src[6]) |
                 ((uint64_t)src[7] << 8) |
                 ((uint64_t)src[8] << 16) |
                 ((uint64_t)src[9] << 24) |
                 ((uint64_t)src[10] << 32) |
                 ((uint64_t)src[11] << 40);
        }
        else
        {
            p0 = ((uint64_t)src[0] << 40) |
                 ((uint64_t)src[1] << 32) |
                 ((uint64_t)src[2] << 24) |
                 ((uint64_t)src[3] << 16) |
                 ((uint64_t)src[4] << 8) |
                 ((uint64_t)src[5]);
            p1 = ((uint64_t)src[6] << 40) |
                 ((uint64_t)src[7] << 32) |
                 ((uint64_t)src[8] << 24) |
                 ((uint64_t)src[9] << 16) |
                 ((uint64_t)src[10] << 8) |
                 ((uint64_t)src[11]);
        }
        __m128i out = _mm_set_epi16((uint16_t)((p1 >> 36) & 0xfff),
                                    (uint16_t)((p1 >> 24) & 0xfff),
                                    (uint16_t)((p1 >> 12) & 0xfff),
                                    (uint16_t)(p1 & 0xfff),
                                    (uint16_t)((p0 >> 36) & 0xfff),
                                    (uint16_t)((p0 >> 24) & 0xfff),
                                    (uint16_t)((p0 >> 12) & 0xfff),
                                    (uint16_t)(p0 & 0xfff));
        _mm_storeu_si128((__m128i *)(dst + i), out);
        src += 12;
    }
    if (i < count)
        unpack12_scalar(src, dst + i, count - i, bigendian);
}
#endif /* HAVE_SSE41 */

void TIFFPackRaw12(const uint16_t *src, uint8_t *dst, size_t count, int bigendian)
{
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    if (tiff_use_neon)
        pack12_neon(src, dst, count, bigendian);
    else
#endif
#if defined(HAVE_SSE41)
    if (tiff_use_sse41)
        pack12_sse41(src, dst, count, bigendian);
    else
#endif
        pack12_scalar(src, dst, count, bigendian);
}

void TIFFUnpackRaw12(const uint8_t *src, uint16_t *dst, size_t count,
                     int bigendian)
{
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    if (tiff_use_neon)
        unpack12_neon(src, dst, count, bigendian);
    else
#endif
#if defined(HAVE_SSE41)
    if (tiff_use_sse41)
        unpack12_sse41(src, dst, count, bigendian);
    else
#endif
        unpack12_scalar(src, dst, count, bigendian);
}

void TIFFPackRaw10(const uint16_t *src, uint8_t *dst, size_t count, int bigendian)
{
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    if (tiff_use_neon)
        pack10_neon(src, dst, count, bigendian);
    else
#endif
#if defined(HAVE_SSE41)
    if (tiff_use_sse41)
        pack10_sse41(src, dst, count, bigendian);
    else
#endif
        pack10_scalar(src, dst, count, bigendian);
}

void TIFFUnpackRaw10(const uint8_t *src, uint16_t *dst, size_t count, int bigendian)
{
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    if (tiff_use_neon)
        unpack10_neon(src, dst, count, bigendian);
    else
#endif
#if defined(HAVE_SSE41)
    if (tiff_use_sse41)
        unpack10_sse41(src, dst, count, bigendian);
    else
#endif
        unpack10_scalar(src, dst, count, bigendian);
}

void TIFFPackRaw14(const uint16_t *src, uint8_t *dst, size_t count, int bigendian)
{
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    if (tiff_use_neon)
        pack14_neon(src, dst, count, bigendian);
    else
#endif
#if defined(HAVE_SSE41)
    if (tiff_use_sse41)
        pack14_sse41(src, dst, count, bigendian);
    else
#endif
        pack14_scalar(src, dst, count, bigendian);
}

void TIFFUnpackRaw14(const uint8_t *src, uint16_t *dst, size_t count, int bigendian)
{
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    if (tiff_use_neon)
        unpack14_neon(src, dst, count, bigendian);
    else
#endif
#if defined(HAVE_SSE41)
    if (tiff_use_sse41)
        unpack14_sse41(src, dst, count, bigendian);
    else
#endif
        unpack14_scalar(src, dst, count, bigendian);
}

void TIFFPackRaw16(const uint16_t *src, uint8_t *dst, size_t count, int bigendian)
{
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    if (tiff_use_neon)
        pack16_neon(src, dst, count, bigendian);
    else
#endif
#if defined(HAVE_SSE41)
    if (tiff_use_sse41)
        pack16_sse41(src, dst, count, bigendian);
    else
#endif
        pack16_scalar(src, dst, count, bigendian);
}

void TIFFUnpackRaw16(const uint8_t *src, uint16_t *dst, size_t count, int bigendian)
{
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    if (tiff_use_neon)
        unpack16_neon(src, dst, count, bigendian);
    else
#endif
#if defined(HAVE_SSE41)
    if (tiff_use_sse41)
        unpack16_sse41(src, dst, count, bigendian);
    else
#endif
        unpack16_scalar(src, dst, count, bigendian);
}

