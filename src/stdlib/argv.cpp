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
    ___size = 0;
    ___capacity = 13;
    ___data = (char **)malloc(sizeof(char *) * (___capacity  + 1));
    ___data[___size] = 0;
}

argv::~argv()
{
    clear();
    free(___data);
}

argv::argv(const char *str, const char *delim)
{
    ___size = 0;
    ___capacity = 13;
    ___data = (char **)malloc(sizeof(char *) * (___capacity  + 1));
    ___data[___size] = 0;
    split(str, delim);
}

void argv::push_back(const std::string &v)
{
    ___append(memdupnull(v.c_str(), v.size()));
}

void argv::push_back(const char *v)
{
    ___append(strdup(v));
}

void argv::push_back(const char *v, size_t n)
{
    ___append(memdupnull(v, n));
}

void argv::truncate(size_t n)
{
    if (n < 0) {
        return;
    }
    for (size_t i = n; i < ___size; i++) {
        free(___data[i]);
    }
    ___size = n;
    ___data[___size] = 0;
}

void argv::clear()
{
    for (size_t i = 0; i < ___size; i++) {
        free(___data[i]);
    }
    ___size = 0;
}

void argv::split(const char *str, const char *delim)
{
    strtok splitor;
    splitor.set_str(str);
    while (splitor.tok(delim)) {
        ___append(memdupnull(splitor.ptr(), splitor.size()));
    }
}

void argv::debug_show()
{
    for (size_t i = 0; i < ___size; i++) {
        printf("%2d, %s\n", (int)i, ___data[i]);
    }
}

void argv::___append(const char *v)
{
    if (___size == ___capacity) {
        ___capacity *= 2 ;
        ___data = (char **)realloc(___data, sizeof(char *) * (___capacity + 1));
    }
    ___data[___size++] = const_cast<char *>(v);
    ___data[___size] = 0;
}

}
