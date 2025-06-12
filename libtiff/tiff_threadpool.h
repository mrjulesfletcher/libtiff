#ifndef TIFF_THREADPOOL_H
#define TIFF_THREADPOOL_H

#include "tiffiop.h"

#ifdef __cplusplus
extern "C" {
#endif

void _TIFFThreadPoolInit(int workers);
void _TIFFThreadPoolShutdown(void);
void _TIFFThreadPoolSubmit(void (*func)(void*), void* arg);
void _TIFFThreadPoolWait(void);

#ifdef __cplusplus
}
#endif

#endif /* TIFF_THREADPOOL_H */
