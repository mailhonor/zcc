/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-01-21
 * ================================
 */

#include "zcc.h"

namespace zcc
{

basic_vector:: basic_vector()
{
    ___size = 0;
    ___capacity = 0;
    ___gmp = 0;
    ___data = 0;
}

basic_vector::~basic_vector()
{
    if (!___gmp) {
        zcc::free(___data);
    }
}

void basic_vector::push_back_void(const void * v)
{
    if (___capacity == 0) {
        reserve_void(13);
    }
    if (___size == ___capacity) {
        reserve_void(___capacity);
    }
    ___data[___size++] = (char *)v;
    ___data[___size] = 0;
}

void basic_vector::reserve_void(size_t size)
{
    size_t left = ___capacity - ___size;
    if (size <= left) {
        return;
    }
    size = size - left;
    if (size < ___capacity) {
        size = ___capacity;
    }

    if (___gmp) {
        char ** nd = (char **)___gmp->malloc(sizeof(char *) * (___capacity + size +1));
        if (___data && ___size) {
            memcpy(nd, ___data, sizeof(char *) * ___size);
        }
        ___data = nd;
    } else {
        ___data = (char **)zcc::realloc(___data, (sizeof(char *) * (___capacity + size +1)));
    }
    ___data[___size] = 0;
    ___capacity += size;
}

void basic_vector::option_gm_pool_void(gm_pool &gmp)
{
    ___gmp = &gmp;
}

}
