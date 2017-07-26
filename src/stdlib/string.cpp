/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

#include "zcc.h"

static int (*___vsnprintf)(char *str, size_t size, const char *fmt, va_list ap) = vsnprintf;

namespace zcc
{

std::string &sprintf_1024(std::string &str, const char *fmt, ...)
{
    va_list ap;
    char buf[1024+1];

    va_start(ap, fmt);
    ___vsnprintf(buf, 1024, fmt, ap);
    va_end(ap);
    str.append(buf);
    return str;
}

std::string &tolower(std::string &str)
{
    tolower(str.c_str());
    return str;
}

std::string &toupper(std::string &str)
{
    toupper(str.c_str());
    return str;
}

std::string &size_data_escape(std::string &str, const void *data, size_t n)
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

std::string &size_data_escape(std::string &str, int i)
{
    char buf[32];
    size_t n = sprintf(buf, "%d", i);
    return size_data_escape(str, buf, n);
}

std::string &size_data_escape(std::string &str, long i)
{
    char buf[32];
    size_t n = sprintf(buf, "%ld", i);
    return size_data_escape(str, buf, n);
}

}


#if 0

namespace zcc
{

class string
{
public:
    string();
    string(const string &_x);
    string(const char *str);
    string(const char *str, size_t size);
    string(size_t n, int ch);
    string(size_t size);
    ~string();

    inline /* const */ char *c_str() const {___data[___size] = 0; return ___data;}
    inline size_t empty() const { return !___size; }
    inline size_t size() const { return ___size; }
    inline size_t length() const { return ___size; }
    inline size_t capability() const { return ___capacity; }
    inline void push_back(int ch){(___size<___capacity)?(___data[___size++]=ch):put_do(ch);}
    inline string &put(int ch) { push_back(ch); return *this; }
    inline void clear() { ___size = 0; }
    string &resize(size_t n);
    string &reserve(size_t n);
    inline string &truncate(size_t n) { return resize(n); }
    inline string &append(int ch) { push_back(ch); return *this; }
    string &append(string &s);
    string &append(const char *str);
    string &append(const char *str, size_t n);
    string &append(size_t n, int ch);
    string &printf_1024(const char *format, ...);

    inline string & operator=(const char *str) { clear(); return append(str); }
    inline string & operator=(const string &s) { clear(); return append(s.___data, s.___size); }
    inline string & operator+=(const char *str) { return append(str); }
    inline string & operator+=(const string &s) { return append(s.___data, s.___size); }
    inline string & operator+=(int ch) { return append(ch); }
    inline int operator[](size_t n) { return ___data[n]; }

    string &size_data_escape(const void *data, size_t n = 0);
    string &size_data_escape(int i);
    string &size_data_escape(long i);

private:
    void _init_buf(size_t size);
    int put_do(int ch);
private:
    char *___data;
    unsigned int ___size;
    unsigned int ___capacity;
};

}

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
}

string::string(const char *str, size_t size)
{
    _init_buf(size<13?13:size);
    ___memcpy___(___data, str, size);
}

string::string(size_t n, int ch)
{
    ___data = blank_buffer;
    ___size = ___capacity = 0;
    if (n) {
        reserve(n);
        char *ptr = ___data + ___size;
        resize(___size + n);
        memset(ptr, ch, n);
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
