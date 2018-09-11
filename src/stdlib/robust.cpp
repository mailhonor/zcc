/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2018-07-12
 * ================================
 */

#include "zcc.h"

namespace zcc
{

#define ___ROBUST_DO(exp) \
    int ret; \
    do { \
        ret = exp; \
    } while((ret<0) && (errno==EINTR)); \
    return ret;

int robust_close(int fd)
{
    ___ROBUST_DO(close(fd));
}

int robust_flock(int fd, int operation)
{
    ___ROBUST_DO(flock(fd, operation));
}

int robust_rename(const char *oldpath, const char *newpath)
{
    ___ROBUST_DO(rename(oldpath, newpath));
}

}
