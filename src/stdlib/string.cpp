/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

#include "zcc.h"
#include <stdarg.h>

#if 0
static int (*___vsnprintf)(char *str, size_t size, const char *fmt, va_list ap) = vsnprintf;
namespace zcc
{

string &sprintf_1024(string &str, const char *fmt, ...)
{
    va_list ap;
    char buf[1024+1];

    va_start(ap, fmt);
    ___vsnprintf(buf, 1024, fmt, ap);
    va_end(ap);
    str.append(buf);
    return str;
}

string &tolower(string &str)
{
    tolower(str.c_str());
    return str;
}

string &toupper(string &str)
{
    toupper(str.c_str());
    return str;
}

string &size_data_escape(string &str, const void *data, size_t n)
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
        str.push_back(ch);
    } while (left);
    if (n > 0) {
        str.append((const char *)data, n);
    }
    return str;
}

string &size_data_escape(string &str, int i)
{
    char buf[32];
    size_t n = sprintf(buf, "%d", i);
    return size_data_escape(str, buf, n);
}

string &size_data_escape(string &str, long i)
{
    char buf[32];
    size_t n = sprintf(buf, "%ld", i);
    return size_data_escape(str, buf, n);
}

}
#endif

#if 1
static void * (*___memcpy___)(void *dest, const void *src, size_t n) = memcpy;
namespace zcc
{

string::string()
{
    ___data = blank_buffer;
    ___size = ___capacity = 0;
}

string::string(const string &_x)
{
    size_t s = _x.___size;
    if (s) {
        _init_buf(s<13?13:s);
        ___memcpy___(___data, _x.___data, s);
        ___size = s;
    } else {
        ___data = blank_buffer;
        ___size = ___capacity = 0;
    }
}

string::string(const char *str)
{
    size_t len = strlen(str);
    _init_buf(len<13?13:len);
    ___memcpy___(___data, str, len);
    ___size = len;
}

string::string(const char *str, size_t size)
{
    _init_buf(size<13?13:size);
    ___memcpy___(___data, str, size);
    ___size = size;
}

string::string(size_t n, int ch)
{
    ___data = blank_buffer;
    ___size = ___capacity = 0;
    if (n) {
        _init_buf(n<13?13:n);
        memset(___data, ch, n);
        ___size = n;
    }
}

string::string(size_t size)
{
    _init_buf(size<13?13:size);
}

string::~string()
{
    free(___data);
}

void string::_init_buf(size_t size)
{
    ___data = (char *)malloc(size + 1);
    ___capacity = size;
    ___size = 0;
}

string &string::reserve(size_t need)
{
    size_t left, incr;

    left =  ___capacity - ___size;
    if (need <= left) {
        return *this;
    }

    incr = need - left;
    if (incr < ___capacity) {
        incr = ___capacity;
    }
    ___capacity += incr;
    if (___capacity < 13) {
        ___capacity = 13;
    }
    ___data = (char *)realloc(___data, ___capacity + 1);

    return *this;
}

int string::put_do(int ch)
{
    if (___size == ___capacity) {
        reserve(1);
    }
    push_back(ch);
    return ch;
}

string &string::resize(size_t n)
{
    if (n > ___capacity) {
        reserve(n - ___capacity);
    }
    if (n  <  ___capacity) {
        ___size = n;
    }
    return *this;
}

string &string::append(const char *str)
{
    size_t n = (str?strlen(str):0);
    if (n) {
        reserve(n);
        ___memcpy___(___data + ___size, str, n);
        ___size += n;
    }
    return *this;
}

string &string::append(const char *str, size_t n)
{
    if (n) {
        reserve(n);
        ___memcpy___(___data + ___size, str, n);
        ___size += n;
    }
    return *this;
}

string &string::append(string &s)
{
    return append(s.c_str(), s.size());
}

string &string::append(size_t n, int ch)
{
    if (n) {
        reserve(n);
        char *ptr = ___data + ___size;
        resize(___size + n);
        memset(ptr, ch, n);
    }
    return *this;
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

#endif
