/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

#include "zcc.h"

static inline void *___memcpy___(void *dest, const void *src, size_t n){ return memcpy(dest, src, n); }

namespace zcc
{

string none_string;

string::string()
{
    ___data = blank_buffer;
    ___size = ___capability = 0;
    ___static_mode = false;
    ___const_mode = false;
}

string::string(const char *str)
{
    size_t len = strlen(str);
    _init_buf(len<13?13:len);
    ___memcpy___(___data, str, len);
}

string::string(const char *str, size_t size)
{
    _init_buf(size<13?13:size);
    ___memcpy___(___data, str, size);
}

string::string(size_t size)
{
    _init_buf(size<13?13:size);
}

string::~string()
{
    if (!___static_mode) {
        free(___data);
    }
}

void string::_init_buf(size_t size)
{
    ___data = (char *)malloc(size + 1);
    ___capability = size;
    ___size = 0;
    ___static_mode = false;
}

void string::set_static_buf(void *data, size_t size, mem_pool &mpool)
{
    if (!___const_mode) {
        mpool.free(___data);
    }
    ___data = (char *)data;
    ___capability = size;
    ___size = 0;
    ___static_mode = true;
    ___const_mode = false;
}

void string::set_const_buf(const void *data, size_t size, mem_pool &mpool)
{
    if (!___const_mode) {
        mpool.free(___data);
    }
    ___data = (char *)data;
    ___capability = size;
    ___size = 0;
    ___static_mode = true;
    ___const_mode = true;
}

bool string::need_space(size_t need)
{
    size_t left, incr;

    left =  ___capability - ___size;
    if (need <= left) {
        return true;
    }

    if (___static_mode) {
        return false;
    }

    incr = need - left;
    if (incr < ___capability) {
        incr = ___capability;
    }
    ___capability += incr;
    if (___capability < 13) {
        ___capability = 13;
    }
    ___data = (char *)realloc(___data, ___capability + 1);

    return true;
}

int string::put_do(int ch)
{
    if (___size == ___capability) {
        if (!need_space(1)) {
            return -1;
        }
    }
    push_back(ch);
    return ch;
}

string &string::resize(size_t n)
{
    if (n > ___capability) {
        need_space(n - ___capability);
    }
    if (n  <  ___capability) {
        ___size = n;
    }
    return *this;
}

string &string::append(const char *str)
{
    size_t n = (str?strlen(str):0);
    if (n) {
        if (need_space(n)) {
            ___memcpy___(___data + ___size, str, n);
            ___size += n;
        } else {
            while (n--) {
                push_back(*str++);
            }
        }
    }
    return *this;
}

string &string::append(const char *str, size_t n)
{
    if (n) {
        if (need_space(n)) {
            ___memcpy___(___data + ___size, str, n);
            ___size += n;
        } else {
            while (n--) {
                push_back(*str++);
            }
        }
    }
    return *this;
}

string &string::append(string &s)
{
    return append(s.c_str(), s.size());
}

string &string::printf_1024(const char *format, ...)
{
    va_list ap;
    char buf[1024+1];
    size_t n;

    va_start(ap, format);
    n = vsnprintf(buf, 1024, format, ap);
    va_end(ap);
    return append(buf, n);
}

string &string::size_data_escape(const void *data, size_t n)
{
	int ch, left;
	if (n == 0) {
        if (*((const char *)data)) {
            n = strlen((const char *)data);
        }
	}
    left = (int)n;
	do {
		ch = left & 0177;
		left >>= 7;
		if (!left) {
			ch |= 0200;
		}
        push_back(ch);
	} while (left);
	if (n > 0) {
        append((const char *)data, n);
	}
    return *this;
}

string &string::size_data_escape(int i)
{
    char buf[32];
    size_t n = sprintf(buf, "%d", i);
    return size_data_escape(buf, n);
}

string &string::size_data_escape(long i)
{
    char buf[32];
    size_t n = sprintf(buf, "%zd", i);
    return size_data_escape(buf, n);
}

}
