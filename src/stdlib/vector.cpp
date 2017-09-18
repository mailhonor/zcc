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

void vector_reserve(size_t tsize, unsigned int *capacity, unsigned int *size, char **data, size_t reserver_size)
{
    size_t left = *capacity - *size;
    if (reserver_size == 0) {
        reserver_size = 1;
    }
    if (reserver_size <= left) {
        return;
    }
    size_t extend = reserver_size - left;
    if (extend < *capacity) {
        extend = *capacity;
    }

    *data = (char *)zcc::realloc(*data, (tsize * (*capacity + extend +1)));
    *capacity += extend;
}

void vector_resize(size_t tsize, unsigned int *capacity, unsigned int *size, char **data, size_t resize)
{
    if (resize <= *size) {
        *size = resize;
        return;
    }
    if (resize > *capacity) {
        vector_reserve(tsize, capacity, size, data, resize - *capacity);
    }
    memset(*data + tsize * (*size), 0, tsize * (resize - *size));
    *size = resize;
}

void vector_erase(size_t tsize, unsigned int *capacity, unsigned int *size, char **data, size_t n)
{
    if (n >= *size) {
        return;
    }
    size_t end = *size;
    if (tsize == 8) {
        long *ptr = (long *)*data;
        for (size_t i = n; i < end; i++) {
            ptr[i] = ptr[i+1];
        }
    } else if (tsize == 4) {
        int *ptr = (int *)*data;
        for (size_t i = n; i < end; i++) {
            ptr[i] = ptr[i+1];
        }
    } else if (tsize == 2) {
        short int *ptr = (short *)*data;
        for (size_t i = n; i < end; i++) {
            ptr[i] = ptr[i+1];
        }
    } else {
        for (size_t i = n; i < end; i++) {
            memcpy(*data + tsize * i, *data + tsize * (i+1), tsize);
        }
    }
    (*size) --;
}

}
