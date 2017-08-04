/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-07-25
 * ================================
 */

#include "zcc.h"
#include <sys/time.h>
#include <sys/types.h>
#include <pthread.h>

namespace zcc
{

void debug_kv_show(const char *k, const char *v)
{
    printf("%32s = %s\n", k, v);
}

void debug_kv_show(const char *k, long v)
{
    printf("%32s = %lu\n", k, v);
}

#if defined(PTHREAD_SPINLOCK_INITIALIZER)
static pthread_spinlock_t build_unique_id_lock = PTHREAD_SPINLOCK_INITIALIZER;
#else 
static pthread_mutex_t build_unique_id_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

static long build_unique_id_plus = 0;
static pid_t build_unique_id_tid = 0;
char *build_unique_filename_id(char *buf)
{
    long plus;
#if defined(PTHREAD_SPINLOCK_INITIALIZER)
    pthread_spin_lock(&build_unique_id_lock);
#else
    pthread_mutex_lock(&build_unique_id_lock);
#endif
    plus = build_unique_id_plus++;
    if (build_unique_id_tid == 0) {
        pid_t gettid(void);
        build_unique_id_tid = gettid();
    }
#if defined(PTHREAD_SPINLOCK_INITIALIZER)
    pthread_spin_unlock(&build_unique_id_lock);
#else
    pthread_mutex_unlock(&build_unique_id_lock);
#endif

    struct timeval tv;
    gettimeofday(&tv, 0);
    sprintf(buf, "%lx_%ld_%lx_%lx", (long)tv.tv_usec, (long)tv.tv_sec, (unsigned long)build_unique_id_tid, plus);

    return buf;
}

}
