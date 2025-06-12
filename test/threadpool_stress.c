#include "tiff_threadpool.h"
#include <pthread.h>
#include <stdio.h>

#define PRODUCER_THREADS 4
#define TASKS_PER_THREAD 50

static pthread_mutex_t cnt_mutex = PTHREAD_MUTEX_INITIALIZER;
static int counter = 0;

static void inc_task(void *arg)
{
    (void)arg;
    pthread_mutex_lock(&cnt_mutex);
    counter++;
    pthread_mutex_unlock(&cnt_mutex);
}

static void *producer(void *arg)
{
    (void)arg;
    for (int i = 0; i < TASKS_PER_THREAD; i++)
        _TIFFThreadPoolSubmit(inc_task, NULL);
    return NULL;
}

int main(void)
{
    _TIFFThreadPoolInit(PRODUCER_THREADS);

    pthread_t prod[PRODUCER_THREADS];
    for (int i = 0; i < PRODUCER_THREADS; i++)
        pthread_create(&prod[i], NULL, producer, NULL);
    for (int i = 0; i < PRODUCER_THREADS; i++)
        pthread_join(prod[i], NULL);

    _TIFFThreadPoolWait();
    _TIFFThreadPoolShutdown();

    int expected = PRODUCER_THREADS * TASKS_PER_THREAD;
    if (counter != expected)
    {
        fprintf(stderr, "counter=%d expected=%d\n", counter, expected);
        return 1;
    }
    return 0;
}
