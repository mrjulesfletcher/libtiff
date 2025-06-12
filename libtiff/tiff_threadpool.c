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

typedef struct
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

static TIFFThreadPool gPool = {0,
                               NULL,
                               NULL,
                               NULL,
                               0,
                               0,
                               PTHREAD_MUTEX_INITIALIZER,
                               PTHREAD_COND_INITIALIZER};
static int gThreadCount = 1;
static pthread_mutex_t gThreadCountMutex = PTHREAD_MUTEX_INITIALIZER;

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
        free(task);
        pthread_mutex_lock(&pool->mutex);
        pool->active--;
        if (!pool->head && pool->active == 0)
            pthread_cond_broadcast(&pool->cond);
        pthread_mutex_unlock(&pool->mutex);
    }
    return NULL;
}

int _TIFFThreadPoolInit(int workers)
{
    if (workers <= 0)
        workers = 1;
    pthread_mutex_lock(&gPool.mutex);
    pthread_mutex_lock(&gThreadCountMutex);
    gThreadCount = workers;
    pthread_mutex_unlock(&gThreadCountMutex);
    if (gPool.threads)
    {
        pthread_mutex_unlock(&gPool.mutex);
        return 1;
    }
    gPool.workers = workers;
    gPool.threads = (pthread_t *)calloc(workers, sizeof(pthread_t));
    if (!gPool.threads)
    {
        pthread_mutex_unlock(&gPool.mutex);
        return 0;
    }
    for (int i = 0; i < workers; i++)
    {
        if (pthread_create(&gPool.threads[i], NULL, _tiffThreadProc, &gPool) !=
            0)
        {
            for (int j = 0; j < i; j++)
                pthread_join(gPool.threads[j], NULL);
            free(gPool.threads);
            gPool.threads = NULL;
            pthread_mutex_unlock(&gPool.mutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&gPool.mutex);
    return 1;
}

void _TIFFThreadPoolShutdown(void)
{
    pthread_mutex_lock(&gPool.mutex);
    gPool.stop = 1;
    pthread_cond_broadcast(&gPool.cond);
    pthread_mutex_unlock(&gPool.mutex);
    if (gPool.threads)
    {
        for (int i = 0; i < gPool.workers; i++)
            pthread_join(gPool.threads[i], NULL);
        free(gPool.threads);
    }
    pthread_mutex_lock(&gPool.mutex);
    gPool.threads = NULL;
    gPool.head = gPool.tail = NULL;
    gPool.stop = 0;
    pthread_mutex_unlock(&gPool.mutex);
}

int _TIFFThreadPoolSubmit(void (*func)(void *), void *arg)
{
    static const char module[] = "_TIFFThreadPoolSubmit";
    TPTask *t = (TPTask *)malloc(sizeof(TPTask));
    if (!t)
    {
        TIFFErrorExtR(NULL, module, "Out of memory");
        return 0;
    }
    t->func = func;
    t->arg = arg;
    t->next = NULL;
    pthread_mutex_lock(&gPool.mutex);
    if (gPool.tail)
        gPool.tail->next = t;
    else
        gPool.head = t;
    gPool.tail = t;
    pthread_cond_signal(&gPool.cond);
    pthread_mutex_unlock(&gPool.mutex);
    return 1;
}

void _TIFFThreadPoolWait(void)
{
    pthread_mutex_lock(&gPool.mutex);
    while (gPool.head || gPool.active)
        pthread_cond_wait(&gPool.cond, &gPool.mutex);
    pthread_mutex_unlock(&gPool.mutex);
}

void TIFFSetThreadCount(int count)
{
    if (count < 1)
        count = 1;
    _TIFFThreadPoolShutdown();
    _TIFFThreadPoolInit(count);
}

int TIFFGetThreadCount(void)
{
    pthread_mutex_lock(&gThreadCountMutex);
    int count = gThreadCount;
    pthread_mutex_unlock(&gThreadCountMutex);
    return count;
}

#else
/* Stub implementations when thread pool disabled */
#include "tiff_threadpool.h"
int _TIFFThreadPoolInit(int workers)
{
    (void)workers;
    return 1;
}
void _TIFFThreadPoolShutdown(void) {}
int _TIFFThreadPoolSubmit(void (*func)(void *), void *arg)
{
    func(arg);
    return 1;
}
void _TIFFThreadPoolWait(void) {}
void TIFFSetThreadCount(int count) { (void)count; }
int TIFFGetThreadCount(void) { return 1; }
#endif
