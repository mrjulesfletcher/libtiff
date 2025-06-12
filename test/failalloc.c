#define _GNU_SOURCE
#include <stdlib.h>
#include <dlfcn.h>
#include <pthread.h>
#include <errno.h>

static int malloc_fail_count = -1;
static int calloc_fail_count = -1;
static int pthread_create_fail_count = -1;
static int pthread_mutex_init_fail_count = -1;
static int pthread_cond_init_fail_count = -1;

static void update_from_env(void)
{
    const char *s = getenv("FAIL_MALLOC_COUNT");
    malloc_fail_count = s ? atoi(s) : -1;
    s = getenv("FAIL_CALLOC_COUNT");
    calloc_fail_count = s ? atoi(s) : -1;
    s = getenv("FAIL_PTHREAD_CREATE_COUNT");
    pthread_create_fail_count = s ? atoi(s) : -1;
    s = getenv("FAIL_PTHREAD_MUTEX_INIT_COUNT");
    pthread_mutex_init_fail_count = s ? atoi(s) : -1;
    s = getenv("FAIL_PTHREAD_COND_INIT_COUNT");
    pthread_cond_init_fail_count = s ? atoi(s) : -1;
}

void failalloc_reset_from_env(void) { update_from_env(); }

__attribute__((constructor)) static void init(void) { update_from_env(); }

void *malloc(size_t size)
{
    static void *(*real_malloc)(size_t) = NULL;
    if (!real_malloc)
        real_malloc = dlsym(RTLD_NEXT, "malloc");
    if (malloc_fail_count > 0)
    {
        malloc_fail_count--;
        if (malloc_fail_count == 0)
            return NULL;
    }
    return real_malloc(size);
}

void *calloc(size_t nmemb, size_t size)
{
    static void *(*real_calloc)(size_t,size_t) = NULL;
    if (!real_calloc)
        real_calloc = dlsym(RTLD_NEXT, "calloc");
    if (calloc_fail_count > 0)
    {
        calloc_fail_count--;
        if (calloc_fail_count == 0)
            return NULL;
    }
    return real_calloc(nmemb, size);
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void*), void *arg)
{
    static int (*real_pthread_create)(pthread_t*, const pthread_attr_t*,
                                      void*(*)(void*), void*) = NULL;
    if (!real_pthread_create)
        real_pthread_create = dlsym(RTLD_NEXT, "pthread_create");
    if (pthread_create_fail_count > 0)
    {
        pthread_create_fail_count--;
        if (pthread_create_fail_count == 0)
            return EAGAIN;
    }
    return real_pthread_create(thread, attr, start_routine, arg);
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
    static int (*real_pthread_mutex_init)(pthread_mutex_t *, const pthread_mutexattr_t *) = NULL;
    if (!real_pthread_mutex_init)
        real_pthread_mutex_init = dlsym(RTLD_NEXT, "pthread_mutex_init");
    if (pthread_mutex_init_fail_count > 0)
    {
        pthread_mutex_init_fail_count--;
        if (pthread_mutex_init_fail_count == 0)
            return EAGAIN;
    }
    return real_pthread_mutex_init(mutex, attr);
}

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
    static int (*real_pthread_cond_init)(pthread_cond_t *, const pthread_condattr_t *) = NULL;
    if (!real_pthread_cond_init)
        real_pthread_cond_init = dlsym(RTLD_NEXT, "pthread_cond_init");
    if (pthread_cond_init_fail_count > 0)
    {
        pthread_cond_init_fail_count--;
        if (pthread_cond_init_fail_count == 0)
            return EAGAIN;
    }
    return real_pthread_cond_init(cond, attr);
}
