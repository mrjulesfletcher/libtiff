#include "tif_config.h"
#ifdef TIFF_USE_THREADPOOL
#include "tiff_threadpool.h"
#include "tiffiop.h"
#include <pthread.h>
#include <stdlib.h>

typedef struct _TPTask
{
    void (*func)(void *);
    void *arg;
    struct _TPTask *next;
} TPTask;

typedef struct TIFFThreadPool
{
    int workers;
    pthread_t *threads;
    TPTask *head;
    TPTask *tail;
    int stop;
    int active;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} TIFFThreadPool;

static void *_tiffThreadProc(void *arg)
{
    TIFFThreadPool *pool = (TIFFThreadPool *)arg;
    for (;;)
    {
        pthread_mutex_lock(&pool->mutex);
        while (!pool->head && !pool->stop)
        {
            pthread_cond_wait(&pool->cond, &pool->mutex);
        }
        if (pool->stop && !pool->head)
        {
            pthread_mutex_unlock(&pool->mutex);
            break;
        }
        TPTask *task = pool->head;
        pool->head = task->next;
        if (pool->head == NULL)
            pool->tail = NULL;
        pool->active++;
        pthread_mutex_unlock(&pool->mutex);
        task->func(task->arg);
        _TIFFfreeExt(NULL, task);
        pthread_mutex_lock(&pool->mutex);
        pool->active--;
        if (!pool->head && pool->active == 0)
            pthread_cond_broadcast(&pool->cond);
        pthread_mutex_unlock(&pool->mutex);
    }
    return NULL;
}

TIFFThreadPool *_TIFFThreadPoolInit(int workers)
{
    if (workers <= 0)
        workers = 1;
    static const char module[] = "_TIFFThreadPoolInit";
    TIFFThreadPool *pool = (TIFFThreadPool *)calloc(1, sizeof(TIFFThreadPool));
    if (!pool)
        return NULL;
    int err = pthread_mutex_init(&pool->mutex, NULL);
    if (err != 0)
    {
        TIFFErrorExtR(NULL, module, "pthread_mutex_init failed: %d", err);
        free(pool);
        return NULL;
    }
    err = pthread_cond_init(&pool->cond, NULL);
    if (err != 0)
    {
        TIFFErrorExtR(NULL, module, "pthread_cond_init failed: %d", err);
        pthread_mutex_destroy(&pool->mutex);
        free(pool);
        return NULL;
    }
    pool->workers = workers;
    pool->threads = (pthread_t *)calloc(workers, sizeof(pthread_t));
    if (!pool->threads)
    {
        pthread_mutex_destroy(&pool->mutex);
        pthread_cond_destroy(&pool->cond);
        free(pool);
        return NULL;
    }
    for (int i = 0; i < workers; i++)
    {
        if (pthread_create(&pool->threads[i], NULL, _tiffThreadProc, pool) != 0)
        {
            for (int j = 0; j < i; j++)
                pthread_join(pool->threads[j], NULL);
            free(pool->threads);
            pthread_mutex_destroy(&pool->mutex);
            pthread_cond_destroy(&pool->cond);
            free(pool);
            return NULL;
        }
    }
    return pool;
}

void _TIFFThreadPoolShutdown(TIFFThreadPool *pool)
{
    if (!pool)
        return;
    pthread_mutex_lock(&pool->mutex);
    pool->stop = 1;
    pthread_cond_broadcast(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);
    if (pool->threads)
    {
        for (int i = 0; i < pool->workers; i++)
            pthread_join(pool->threads[i], NULL);
        free(pool->threads);
    }
    pthread_mutex_lock(&pool->mutex);
    pool->threads = NULL;
    pool->head = pool->tail = NULL;
    pool->stop = 0;
    pthread_mutex_unlock(&pool->mutex);
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond);
    free(pool);
}

int _TIFFThreadPoolSubmit(TIFFThreadPool *pool, void (*func)(void *), void *arg)
{
    static const char module[] = "_TIFFThreadPoolSubmit";
    if (!pool)
    {
        TIFFErrorExtR(NULL, module, "Thread pool not initialized");
        return 0;
    }
    pthread_mutex_lock(&pool->mutex);
    if (pool->threads == NULL || pool->stop)
    {
        pthread_mutex_unlock(&pool->mutex);
        TIFFErrorExtR(NULL, module, "Thread pool not initialized");
        return 0;
    }
    TPTask *t = (TPTask *)_TIFFmallocExt(NULL, sizeof(TPTask));
    if (!t)
    {
        pthread_mutex_unlock(&pool->mutex);
        TIFFErrorExtR(NULL, module, "Out of memory");
        return 0;
    }
    t->func = func;
    t->arg = arg;
    t->next = NULL;
    if (pool->tail)
        pool->tail->next = t;
    else
        pool->head = t;
    pool->tail = t;
    pthread_cond_signal(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);
    return 1;
}

void _TIFFThreadPoolWait(TIFFThreadPool *pool)
{
    if (!pool)
        return;
    pthread_mutex_lock(&pool->mutex);
    while (pool->head || pool->active)
        pthread_cond_wait(&pool->cond, &pool->mutex);
    pthread_mutex_unlock(&pool->mutex);
}

void TIFFSetThreadCount(TIFF *tif, int count)
{
    if (count < 1)
        count = 1;
    _TIFFThreadPoolShutdown(tif ? tif->tif_threadpool : NULL);
    if (tif)
        tif->tif_threadpool = _TIFFThreadPoolInit(count);
}

int TIFFGetThreadCount(TIFF *tif)
{
    if (tif && tif->tif_threadpool)
        return tif->tif_threadpool->workers;
    return 1;
}

#else
/* Stub implementations when thread pool disabled */
#include "tiff_threadpool.h"
TIFFThreadPool *_TIFFThreadPoolInit(int workers)
{
    (void)workers;
    return NULL;
}
void _TIFFThreadPoolShutdown(TIFFThreadPool *pool) { (void)pool; }
int _TIFFThreadPoolSubmit(TIFFThreadPool *pool, void (*func)(void *), void *arg)
{
    (void)pool;
    func(arg);
    return 1;
}
void _TIFFThreadPoolWait(TIFFThreadPool *pool) { (void)pool; }
void TIFFSetThreadCount(TIFF *tif, int count)
{
    (void)tif;
    (void)count;
}
int TIFFGetThreadCount(TIFF *tif)
{
    (void)tif;
    return 1;
}
#endif
