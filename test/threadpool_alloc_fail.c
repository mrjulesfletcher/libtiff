#include "tiff_threadpool.h"
#include "tiffio.h"
#include "failalloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void dummy(void* arg) { (void)arg; }

int main(void)
{
    int ret = 0;
    TIFFThreadPool* tp;
    /* calloc failure during init */
    setenv("FAIL_CALLOC_COUNT", "1", 1);
    failalloc_reset_from_env();
    tp = _TIFFThreadPoolInit(2);
    if (tp)
    {
        fprintf(stderr, "Expected calloc failure\n");
        ret = 1;
        _TIFFThreadPoolShutdown(tp);
    }
    /* pthread_create failure */
    setenv("FAIL_PTHREAD_CREATE_COUNT", "1", 1);
    failalloc_reset_from_env();
    tp = _TIFFThreadPoolInit(2);
    if (tp)
    {
        fprintf(stderr, "Expected pthread_create failure\n");
        ret = 1;
        _TIFFThreadPoolShutdown(tp);
    }
    /* successful init */
    unsetenv("FAIL_PTHREAD_CREATE_COUNT");
    unsetenv("FAIL_CALLOC_COUNT");
    failalloc_reset_from_env();
    tp = _TIFFThreadPoolInit(1);
    if (!tp)
    {
        fprintf(stderr, "Expected init success\n");
        ret = 1;
    }
    /* malloc failure on submit */
    setenv("FAIL_MALLOC_COUNT", "1", 1);
    failalloc_reset_from_env();
    if (_TIFFThreadPoolSubmit(tp, dummy, NULL))
    {
        fprintf(stderr, "Expected submit failure\n");
        ret = 1;
    }
    unsetenv("FAIL_MALLOC_COUNT");
    failalloc_reset_from_env();
    _TIFFThreadPoolShutdown(tp);
    return ret;
}
