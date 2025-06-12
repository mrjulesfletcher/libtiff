#include "failalloc.h"
#include "strip_neon.h"
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

    /* Failure of first allocation */
    setenv("FAIL_MALLOC_COUNT", "1", 1);
    failalloc_reset_from_env();
    uint16_t buf[2] = {0, 0};
    uint8_t *strip = TIFFAssembleStripNEON(NULL, buf, 1, 2, 0, 1, NULL);
    int ret = 0;
    if (strip != NULL)
    {
        fprintf(stderr, "Expected failure on first allocation\n");
        free(strip);
        ret = 1;
    }
    if (strcmp(error_buffer, "Out of memory") != 0 ||
        strcmp(error_module, "TIFFAssembleStripNEON") != 0)
    {
        fprintf(stderr, "Unexpected error: %s (%s)\n", error_module,
                error_buffer);
        ret = 1;
    }

    /* Failure of second allocation */
    setenv("FAIL_MALLOC_COUNT", "2", 1);
    failalloc_reset_from_env();
    error_buffer[0] = '\0';
    error_module[0] = '\0';
    strip = TIFFAssembleStripNEON(NULL, buf, 1, 2, 0, 1, NULL);
    if (strip != NULL)
    {
        fprintf(stderr, "Expected failure on second allocation\n");
        free(strip);
        ret = 1;
    }
    if (strcmp(error_buffer, "Out of memory") != 0 ||
        strcmp(error_module, "TIFFAssembleStripNEON") != 0)
    {
        fprintf(stderr, "Unexpected error: %s (%s)\n", error_module,
                error_buffer);
        ret = 1;
    }

    unsetenv("FAIL_MALLOC_COUNT");
    failalloc_reset_from_env();

    /* Overflow detection */
    error_buffer[0] = '\0';
    error_module[0] = '\0';
    strip =
        TIFFAssembleStripNEON(NULL, buf, 0xFFFFFFFFU, 0xFFFFFFFFU, 0, 1, NULL);
    if (strip != NULL)
    {
        fprintf(stderr, "Expected overflow failure\n");
        free(strip);
        ret = 1;
    }
    if (strcmp(error_buffer, "Integer overflow in TIFFAssembleStripNEON") != 0 ||
        strcmp(error_module, "TIFFAssembleStripNEON") != 0)
    {
        fprintf(stderr, "Unexpected error: %s (%s)\n", error_module,
                error_buffer);
        ret = 1;
    }

    TIFFSetErrorHandlerExt(prev);

    return ret;
}
