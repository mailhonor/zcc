/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-10-15
 * ================================
 */

#include "zcc.h"

namespace zcc
{

void vector_reserve(size_t, unsigned int *, unsigned int *, char **, size_t);
void vector_resize(size_t, unsigned int *, unsigned int *, char **, size_t);
void vector_erase(size_t, unsigned int *, unsigned int *, char **, size_t);
void vector_pop(size_t, unsigned int *, unsigned int *, char **, char **v);
template <typename T>
class vector
{
public:
    inline vector() { _c = _s = 0; _d = 0; }
    inline ~vector() { free(_d); }
    inline T &operator[](size_t n) const { return _d[n]; }
    inline void clear() { _s = 0; }
    inline size_t size() const { return _s; }
    inline void push_back(const T v){if(_c==_s){vector_reserve(sizeof(T),&_c,&_s,(char **)&_d, _c);}_d[_s++]=v;}
    inline bool pop(T *v) {if(_s<1)return false;if(v)*v=_d[_s-1];_s--;return true;}
    inline void reserve(size_t size) { vector_reserve(sizeof(T), &_c, &_s, (char **)&_d, size); }
    inline void resize(size_t size) { vector_resize(sizeof(T), &_c, &_s, (char **)&_d, size); }
    inline void erase(size_t n) { vector_erase(sizeof(T), &_c, &_s, (char **)&_d, n); }
private:
    unsigned int _c; /* capacity */
    unsigned int _s; /* size */
    T *_d; /* data */
};
#define zcc_vector_walk_begin(var_your_vec, var_your_ptr) { \
    typeof(var_your_vec) &___V_VEC = (var_your_vec); \
    typeof(___V_VEC[0]) var_your_ptr; \
     size_t var_zcc_vector_opti = 0, ___C_VEC=(___V_VEC).size(); \
    for (; var_zcc_vector_opti < ___C_VEC; var_zcc_vector_opti ++) { \
        var_your_ptr = (___V_VEC)[var_zcc_vector_opti]; {
#define zcc_vector_walk_end }}}

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
