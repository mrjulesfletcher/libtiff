#ifndef TIFF_THREADPOOL_H
#define TIFF_THREADPOOL_H

#include "tiffiop.h"

typedef struct TIFFThreadPool TIFFThreadPool;

#ifdef __cplusplus
extern "C" {
#endif

TIFFThreadPool *_TIFFThreadPoolInit(int workers);
void _TIFFThreadPoolShutdown(TIFFThreadPool *);
int _TIFFThreadPoolSubmit(TIFFThreadPool *, void (*func)(void*), void* arg);
void _TIFFThreadPoolWait(TIFFThreadPool *);

#ifdef __cplusplus
}
#endif

#endif /* TIFF_THREADPOOL_H */
