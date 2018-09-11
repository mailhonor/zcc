
/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2016-11-25
 * ================================
 */


#include "zcc.h"
#include <string.h>
#include <stdlib.h>

namespace zcc
{
static char blank_buffer_buffer[2] = {0};
char *blank_buffer = blank_buffer_buffer;

void *malloc(size_t size)
{
    void *r;

    if (size < 1) {
        size = 1;
    }
    if ((r = ::malloc(size)) == 0) {
        zcc_fatal("malloc: insufficient memory for %zu bytes: %m", size);
    }
    ((char *)r)[0] = 0;

    return r;
}

void *calloc(size_t nmemb, size_t size)
{
    void *r;

    if ((r = ::calloc(nmemb, size)) == 0) {
        zcc_fatal("calloc: insufficient memory for %zux%zu bytes: %m", nmemb, size);
    }

    return r;
}

void *realloc(const void *ptr, size_t size)
{
    void *r;

    if (size < 1) {
        size = 1;
    }

    if (ptr == (const void *)blank_buffer) {
        ptr = 0;
    }
    if ((r = ::realloc(const_cast<void *>(ptr), size)) == 0) {
        zcc_fatal("realloc: insufficient memory for %zu bytes: %m", size);
    }

    return r;
}

void free(const void *ptr)
{
    if (ptr && (ptr!=blank_buffer)) {
        ::free((void *)ptr);
    }
}

char *strdup(const char *ptr)
{
    char *r;
    size_t len;

    if (!ptr) {
        return blank_buffer;
    }
    if (ptr == (const char *)blank_buffer) {
        return blank_buffer;
    }
    len = strlen(ptr);
    r = (char *)malloc(len+1);
    memcpy(r, ptr, len+1);

    return r;
}

char *strndup(const char *ptr, size_t n)
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
    r = zcc::memdup(ptr, len + 1);
    r[len] = 0;

    return r;
}

char *memdup(const void *ptr, size_t n)
{
    char *r;

    if (!ptr) {
        return blank_buffer;
    }

    r =(char *)malloc(n);
    if (n > 0) {
        memcpy(r, ptr, n);
    }

    return r;
}

char *memdupnull(const void *ptr, size_t n)
{
    char *r;

    if (!ptr) {
        return blank_buffer;
    }

    r =(char *)malloc(n+1);
    if (n > 0) {
        memcpy(r, ptr, n);
    }
    r[n] = 0;

    return r;
}
}
