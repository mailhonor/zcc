/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-02-08
 * ================================
 */

#include "zcc.h"

namespace zcc
{

argv::argv()
{
    ___capacity = 0;
    ___size = 0;
    ___mpool = 0;
    ___data = 0;
}
argv::~argv()
{
    if (___mpool && ___data) {
        clear();
        ___mpool->free(___data);
    }
}

void argv::clear()
{
    if (___mpool && ___data) {
        for (size_t i = 0; i < ___size; i++) {
            ___mpool->free(___data[i]);
        }
        ___size = 0;
    }
}


void argv::truncate(size_t n)
{
    if (n < 0) {
        return;
    }
    if (___mpool && ___data) {
        for (size_t i = n; i < ___size; i++) {
            ___mpool->free(___data[i]);
        }
        ___size = n;
        ___data[___size] = 0;
    }
}

void argv::add(const char *v)
{
    if (___capacity == 0) {
        init(0);
    }
    push_back(___mpool->strdup(v));
}

void argv::addn(const char *v, size_t n)
{
    if (___capacity == 0) {
        init(0);
    }
    push_back(___mpool->strndup(v, n));
}

void argv::push_back(const char *v)
{
    if (___capacity == 0) {
        init(0);
    }
    if (___size == ___capacity) {
        char **___nd = (char **)___mpool->malloc(sizeof(char *) * (___capacity * 2 + 1));
        memcpy(___nd, ___data, (sizeof(char *) * ___capacity));
        ___capacity = ___capacity * 2;
        ___mpool->free(___data);
        ___data = ___nd;
    }
    ___data[___size++] = const_cast<char *>(v);
    ___data[___size] = 0;
}

void argv::split(const char *str, const char *delim)
{
    if (___capacity == 0) {
        init(0);
    }
    char *p;
    strtok splitor;
    splitor.set_str(str);
    while (1) {
        if(!splitor.tok(delim)) {
            break;
        }

        p = splitor.ptr();
        p = ___mpool->memdupnull(p, splitor.size());
        push_back(p);
    }
}

void argv::init(size_t capacity, mem_pool &mpool)
{
    if (___capacity == 0) {
        return;
    }
    ___mpool = &mpool;
    if (capacity < 1) {
        capacity = 13;
    }
    ___data = (char **)___mpool->malloc(sizeof(char *) * (___capacity + 1));
    ___data[0] = 0;
}

void argv::debug_show()
{
    for (size_t i = 0; i < ___size; i++) {
        printf("%2d, %s\n", (int)i, ___data[i]);
    }
}

}
