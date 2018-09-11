/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2015-10-12
 * ================================
 */

#include "zcc.h"
#include <stdarg.h>

namespace zcc
{

std::string var_std_string_ignore("");

std::string &sprintf_1024(std::string &str, const char *format, ...)
{
    va_list ap;
    char buf[1024+1];

    va_start(ap, format);
    ::vsnprintf(buf, 1024, format, ap);
    va_end(ap);
    str.append(buf);
    return str;
}

std::string &vsprintf_1024(std::string &str, const char *format, va_list ap)
{
    char buf[1024+1];
    ::vsnprintf(buf, 1024, format, ap);
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
        if (data) {
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

string &string::printf_1024(const char *format, ...)
{
    va_list ap;
    char buf[1024+1];

    va_start(ap, format);
    ::vsnprintf(buf, 1024, format, ap);
    va_end(ap);
    append(buf);
    return *this;
}

std::vector<std::string> split(const char *s, const char *delims)
{
    std::vector<std::string> r;
    std::string stmp;
    strtok splitor(s);
    while (splitor.tok(delims)) {
        stmp.clear();
        stmp.append(splitor.ptr(), splitor.size());
        r.push_back(stmp);
    }
    return r;
}

}

