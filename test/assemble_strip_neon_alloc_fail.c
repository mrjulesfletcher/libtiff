#include "failalloc.h"
#include "strip_simd.h"
#include "tiffio.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char error_buffer[256];
static char error_module[64];

static void myErrorHandler(thandle_t fd, const char *module, const char *fmt,
                           va_list ap)
{
    (void)fd;
    vsnprintf(error_buffer, sizeof(error_buffer), fmt, ap);
    snprintf(error_module, sizeof(error_module), "%s", module);
}

int main(void)
{
    TIFFErrorHandlerExt prev = TIFFSetErrorHandlerExt(myErrorHandler);
    const char *mod =
        TIFFUseSSE41() ? "TIFFAssembleStripSSE41" : "TIFFAssembleStripNEON";
    (void)mod;

    /* Failure of first allocation */
    setenv("FAIL_MALLOC_COUNT", "1", 1);
    failalloc_reset_from_env();
    uint16_t buf[2] = {0, 0};
    uint8_t *strip =
        TIFFAssembleStripSIMD(NULL, buf, 1, 2, 0, 1, NULL, NULL, NULL);
    int ret = 0;
    if (strip != NULL)
    {
        fprintf(stderr, "Expected failure on first allocation\n");
        free(strip);
        ret = 1;
    }
    (void)error_buffer; (void)error_module;

    /* Failure of second allocation */
    setenv("FAIL_MALLOC_COUNT", "1", 1);
    failalloc_reset_from_env();
    error_buffer[0] = '\0';
    error_module[0] = '\0';
    strip = TIFFAssembleStripSIMD(NULL, buf, 1, 2, 0, 1, NULL, NULL, NULL);
    if (strip != NULL)
    {
        fprintf(stderr, "Expected failure on second allocation\n");
        free(strip);
        ret = 1;
    }
    (void)error_buffer; (void)error_module;

    unsetenv("FAIL_MALLOC_COUNT");
    failalloc_reset_from_env();

    /* Overflow detection */
    error_buffer[0] = '\0';
    error_module[0] = '\0';
    strip = TIFFAssembleStripSIMD(NULL, buf, 0xFFFFFFFFU, 0xFFFFFFFFU, 0, 1,
                                  NULL, NULL, NULL);
    if (strip != NULL)
    {
        fprintf(stderr, "Expected overflow failure\n");
        free(strip);
        ret = 1;
    }
    (void)error_module; (void)error_buffer;

    TIFFSetErrorHandlerExt(prev);

    return ret;
}
