/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-11-25
 * ================================
 */


#include "zcc.h"
static inline void * ___malloc(size_t size)
{
    return malloc(size);
}
static inline void ___free(const void *ptr)
{
    free(const_cast <void *> (ptr));
}
static inline void *___realloc(const void *ptr, size_t size)
{
    return realloc(const_cast <void *> (ptr), size);
}
static inline void *___calloc(size_t nmemb, size_t size)
{
    return calloc(nmemb, size);
}

namespace zcc
{

int none_int = 0;
long none_long = 0;
size_t none_size_t = 0;
char *none_buffer = 0;
char blank_buffer_buffer[2] = {0}, *blank_buffer = blank_buffer_buffer;

void *malloc(size_t size)
{
    void *r;

    if (size < 1) {
        size = 1;
    }
    if ((r = ___malloc(size)) == 0) {
        log_fatal("zcc::malloc: insufficient memory for %ld bytes: %m", (long)size);
    }
    ((char *)r)[0] = 0;

    return r;
}

void *calloc(size_t nmemb, size_t size)
{
    void *r;

    if ((r = ___calloc(nmemb, size)) == 0) {
        log_fatal("zcc::calloc: insufficient memory for %ldx%ld bytes: %m", (long)nmemb, size);
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
    if ((r = ___realloc(ptr, size)) == 0) {
        log_fatal("zcc::realloc: insufficient memory for %ld bytes: %m", (long)size);
    }

    return r;
}

void free(const void *ptr)
{
    if (ptr && (ptr!=blank_buffer)) {
        ___free((void *)ptr);
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
    if ((r = (char *)malloc(len+1)) == NULL) {
        log_fatal("zcc::strdup: insufficient memory : %m");
    }
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
    r = memdup(ptr, len + 1);
    r[len] = 0;

    return r;
}

char *memdup(const void *ptr, size_t n)
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

char *memdupnull(const void *ptr, size_t n)
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
}
