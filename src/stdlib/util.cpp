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
    unsigned long plus;
#if defined(PTHREAD_SPINLOCK_INITIALIZER)
    pthread_spin_lock(&build_unique_id_lock);
#else
    pthread_mutex_lock(&build_unique_id_lock);
#endif
    plus = build_unique_id_plus++;
    if (build_unique_id_tid == 0) {
        pid_t getpid(void);
        build_unique_id_tid = ::getpid();
    }
#if defined(PTHREAD_SPINLOCK_INITIALIZER)
    pthread_spin_unlock(&build_unique_id_lock);
#else
    pthread_mutex_unlock(&build_unique_id_lock);
#endif

    struct timeval tv;
    gettimeofday(&tv, 0);
    unsigned int plus2 = plus&0XFFF;
    unsigned char dec2hex[18] = "0123456789abcdef";

    sprintf(buf, "%05x%c%lx%c%05x%c", (unsigned int)tv.tv_usec, dec2hex[(plus2>>8)],
            (long)tv.tv_sec, dec2hex[((plus2>>4)&0XF)], 
            ((unsigned int)build_unique_id_tid)&0XFFFFF, dec2hex[(plus2&0XF)]);

    return buf;
}

std::string &build_unique_filename_id(std::string &path)
{
    char buf[64];
    build_unique_filename_id(buf);
    path.append(buf);
    return path;
}

long get_time_from_unique_filename_id(char *buf)
{
    long r = 0;
    for (int i = 6; i < 14; i++) {
        r = (r<<4) + buf[i] - '0';
    }
    return r;
}


}
