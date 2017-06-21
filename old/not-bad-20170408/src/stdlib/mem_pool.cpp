/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-11-26
 * ================================
 */

#include "zcc.h"

namespace zcc
{

system_mem_pool system_mem_pool_instance;

mem_pool::mem_pool()
{
}

mem_pool::~mem_pool()
{
}

char *mem_pool::strdup(const char *ptr)
{
    char *r;
    size_t len;

    if (!ptr) {
        return blank_buffer;
    }
    len = strlen(ptr);
    if ((r = (char *)malloc(len+1)) == NULL) {
        log_fatal("zcc::strdup: insufficient memory : %m");
    }
    memcpy(r, ptr, len+1);

    return r;
}

char *mem_pool::strndup(const char *ptr, size_t n)
{
    size_t i, len;
    char *r;

    if (!ptr) {
        return blank_buffer;
    }

    if (!n) {
        return blank_buffer;
    }

    len = n;
    for (i = 0; i < n; i++) {
        if (ptr[i] == '\0') {
            len = i;
            break;
        }
    }
    r = memdup(ptr, len + 1);
    r[len] = 0;

    return r;
}

char *mem_pool::memdup(const void *ptr, size_t n)
{
    char *r;

    if (!ptr) {
        return blank_buffer;
    }

    if ((r =(char *)malloc(n)) == NULL) {
        log_fatal("zmemdup: insufficient memory for %ld bytes: %m", (long)n);
    }
    if (n > 0) {
        memcpy(r, ptr, n);
    }

    return r;
}

char *mem_pool::memdupnull(const void *ptr, size_t n)
{
    char *r;

    if (!ptr) {
        return blank_buffer;
    }

    if ((r =(char *)malloc(n+1)) == NULL) {
        log_fatal("zmemdup: insufficient memory for %ld bytes: %m", (long)n);
    }
    if (n > 0) {
        memcpy(r, ptr, n);
    }
    r[n] = 0;

    return r;
}


system_mem_pool::system_mem_pool()
{
}

system_mem_pool::~system_mem_pool()
{
}


}
