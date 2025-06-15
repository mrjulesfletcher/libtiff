#include "tif_config.h"
#ifdef TIFF_USE_THREADPOOL
#include "tiff_threadpool.h"
#include "tiffiop.h"
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define TIFF_THREADPOOL_MAX_QUEUE 256

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
    int queued;
    TPTask *freelist;
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
        pool->queued--;
        pool->active++;
        pthread_mutex_unlock(&pool->mutex);
        task->func(task->arg);
        pthread_mutex_lock(&pool->mutex);
        task->next = pool->freelist;
        pool->freelist = task;
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
    pool->queued = 0;
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
            {
                int rc = pthread_join(pool->threads[j], NULL);
                if (rc != 0)
                    TIFFErrorExtR(NULL, module, "pthread_join failed: %d", rc);
            }
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
        {
            int rc = pthread_join(pool->threads[i], NULL);
            if (rc != 0)
                TIFFErrorExtR(NULL, "_TIFFThreadPoolShutdown",
                              "pthread_join failed: %d", rc);
        }
        free(pool->threads);
    }
    pthread_mutex_lock(&pool->mutex);
    TPTask *t = pool->freelist;
    while (t)
    {
        TPTask *next = t->next;
        _TIFFfreeExt(NULL, t);
        t = next;
    }
    pool->freelist = NULL;
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
    if (pool->queued >= TIFF_THREADPOOL_MAX_QUEUE)
    {
        pthread_mutex_unlock(&pool->mutex);
        TIFFErrorExtR(NULL, module, "Thread pool queue limit exceeded");
        return 0;
    }
    TPTask *t = NULL;
    if (pool->freelist)
    {
        t = pool->freelist;
        pool->freelist = t->next;
    }
    else
    {
        t = (TPTask *)_TIFFmallocExt(NULL, sizeof(TPTask));
        if (!t)
        {
            pthread_mutex_unlock(&pool->mutex);
            TIFFErrorExtR(NULL, module, "Out of memory");
            return 0;
        }
    }
    t->func = func;
    t->arg = arg;
    t->next = NULL;
    if (pool->tail)
        pool->tail->next = t;
    else
        pool->head = t;
    pool->tail = t;
    pool->queued++;
    pthread_cond_signal(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);
    return 1;
}

void _TIFFThreadPoolWait(TIFFThreadPool *pool)
{
    if (!pool)
        return;
    pthread_mutex_lock(&pool->mutex);
    while (pool->queued || pool->active)
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
    if (!tif)
        return 1;
    if (!tif->tif_threadpool)
    {
        int workers = 0;
        const char *env = getenv("TIFF_THREAD_COUNT");
        if (env)
        {
            char *endptr = NULL;
            errno = 0;
            long val = strtol(env, &endptr, 10);
            if (errno != 0 || *endptr != '\0' || endptr == env || val <= 0)
            {
                TIFFErrorExtR(
                    tif, "TIFFGetThreadCount",
                    "Invalid TIFF_THREAD_COUNT value '%s', using default", env);
            }
            else
                workers = (int)val;
        }
        if (workers <= 0)
        {
            long nproc = sysconf(_SC_NPROCESSORS_ONLN);
            if (nproc < 1)
                nproc = 1;
            workers = (int)nproc;
        }
        tif->tif_threadpool = _TIFFThreadPoolInit(workers);
        if (!tif->tif_threadpool)
            return 1;
    }
    return tif->tif_threadpool->workers;
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
