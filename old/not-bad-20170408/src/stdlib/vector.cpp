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
    ___mpool = 0;
    ___data = 0;
}

basic_vector::~basic_vector()
{
    if (___mpool && ___data) {
        ___mpool->free(___data);
    }
}
void basic_vector::push_back_void(const void * v)
{
    if (___capacity == 0) {
        init(0, system_mem_pool_instance);
    }
    if (___size == ___capacity) {
        ___capacity *= 2;
        ___data = (char **)___mpool->realloc(___data, (sizeof(void *) * ___capacity));
    }
    ___data[___size++] = (char *)v;
}

void basic_vector::init(size_t capacity, mem_pool &mpool)
{
    ___capacity = (capacity<1?13:capacity);
    ___size = 0;
    ___mpool = &mpool;
    ___data = (char  **)___mpool->malloc(sizeof(char *) * ___capacity);
}

}
