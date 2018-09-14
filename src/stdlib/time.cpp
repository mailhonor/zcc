/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2016-11-25
 * ================================
 */

#include "zcc.h"
#include <sys/time.h>
#include <time.h>
#include <poll.h>

namespace zcc
{
/* ################################################################## */
/* timeout millisecond`*/
long timeout_set(long timeout)
{
    long r;
    struct timeval tv;

    if (timeout > var_max_timeout) {
        timeout = var_max_timeout;
    }
    gettimeofday(&tv, 0);
    r = tv.tv_sec * 1000 + tv.tv_usec / 1000 + timeout;

    return r;
}

long timeout_left(long timeout)
{
    long r;
    struct timeval tv;

    gettimeofday(&tv, 0);
    r = timeout + 1 - (tv.tv_sec * 1000 + tv.tv_usec / 1000);

    return r;
}

/* ################################################################## */
/* sleep */

void msleep(long delay)
{
#if 0
    struct timespec req, rem;

    rem.tv_sec = delay / 1000;
    rem.tv_nsec = (delay % 1000) * 1000 * 1000;

    while ((rem.tv_sec) || (rem.tv_nsec > 1000)) {
        req.tv_sec = rem.tv_sec;
        req.tv_nsec = rem.tv_nsec;
        rem.tv_sec = 0;
        rem.tv_nsec = 0;
        if ((nanosleep(&req, &rem) < 0) && (errno != EINTR)) {
            zcc_fatal("msleep: nanosleep: %m");
        }
    }
#else
    long timeout = timeout_set(delay);
    int left = (int)delay;
    while (left > 0) {
        poll(0, 0, left);
        left = (int)timeout_left(timeout);
    }
#endif
}

void sleep(long delay)
{
    msleep(delay * 1000);
}

}
