#include "tiff_threadpool.h"
#include "failalloc.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int ret = 0;
    TIFFThreadPool* tp;

    /* pthread_mutex_init failure */
    setenv("FAIL_PTHREAD_MUTEX_INIT_COUNT", "1", 1);
    failalloc_reset_from_env();
    tp = _TIFFThreadPoolInit(1);
    if (tp)
    {
        fprintf(stderr, "Expected pthread_mutex_init failure\n");
        ret = 1;
        _TIFFThreadPoolShutdown(tp);
    }
    unsetenv("FAIL_PTHREAD_MUTEX_INIT_COUNT");

    /* pthread_cond_init failure */
    setenv("FAIL_PTHREAD_COND_INIT_COUNT", "1", 1);
    failalloc_reset_from_env();
    tp = _TIFFThreadPoolInit(1);
    if (tp)
    {
        fprintf(stderr, "Expected pthread_cond_init failure\n");
        ret = 1;
        _TIFFThreadPoolShutdown(tp);
    }
    unsetenv("FAIL_PTHREAD_COND_INIT_COUNT");

    /* successful init */
    failalloc_reset_from_env();
    tp = _TIFFThreadPoolInit(1);
    if (!tp)
    {
        fprintf(stderr, "Expected init success\n");
        ret = 1;
    }
    if (tp)
        _TIFFThreadPoolShutdown(tp);

    return ret;
}
