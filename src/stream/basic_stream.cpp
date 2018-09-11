/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2016-11-30
 * ================================
 */

#include "zcc.h"
#include <fcntl.h>

namespace zcc
{

basic_stream::basic_stream()
{
    read_buf_p1 = 0;
    read_buf_p2 = 0;
    write_buf_len = 0;
    ___opened = false;
    ___error = false;
    ___eof = false;
    ___flushed = false;
    read_buf = (char *) malloc(stream_read_buf_size + 1 + stream_write_buf_size+ 1);
    write_buf = read_buf + stream_read_buf_size + 1;
}

basic_stream::~basic_stream()
{
    free(read_buf);
}

basic_stream &basic_stream::reset()
{
    read_buf_p1 = 0;
    read_buf_p2 = 0;
    write_buf_len = 0;
    ___opened = false;
    ___error = false;
    ___eof = false;
    ___flushed = false;
    return *this;
}

/* read */
int basic_stream::get_char_do()
{
    return -1;
}

ssize_t basic_stream::readn(void *buf, size_t size)
{
    if (!___opened) {
        return -1;
    }
    char *ptr = (char *)buf;
    size_t left_size = size;
    int ch;

    if (size < 1) {
        return 0;
    }
    if (ptr) {
        while (left_size) {
            ch = get();
            if (ch == -1) {
                break;
            }
            *ptr++ = ch;
            left_size-- ;
        }
    } else {
        while (left_size) {
            ch = get();
            if (ch == -1) {
                break;
            }
            left_size-- ;
        }
    }

    if (size > left_size) {
        return size - left_size;
    }
    return -1;
}

ssize_t basic_stream::readn(std::string &str, size_t size)
{
    if (!___opened) {
        return -1;
    }
    size_t left_size = size;
    int ch;

    str.clear();
    if (size < 1) {
        return 0;
    }
    while (left_size) {
        ch = get();
        if (ch == -1) {
            break;
        }
        str.push_back(ch);
        left_size--;
    }

    if (size > left_size) {
        return size - left_size;
    }
    return -1;
}

ssize_t basic_stream::gets(void *buf, size_t size, int delimiter)
{
    if (!___opened) {
        return -1;
    }
    char *ptr = (char *)buf;
    size_t left_size = size;
    int ch;

    if (size < 1) {
        return 0;
    }

    if (ptr) {
        while(left_size) {
            ch = get();
            if (ch == -1) {
                break;
            }
            *ptr++ = ch;
            left_size --;
            if (ch == delimiter) {
                break;
            }
        }
    } else {
        while(left_size) {
            ch = get();
            if (ch == -1) {
                break;
            }
            left_size --;
            if (ch == delimiter) {
                break;
            }
        }
    }
    if (size > left_size) {
        return size - left_size;
    }
    return -1;
}

ssize_t basic_stream::gets(std::string &str, int delimiter)
{
    if (!___opened) {
        return -1;
    }
    int ch;
    ssize_t rlen = 0;
    str.clear();

    while(1) {
        ch = get();
        if (ch == -1) {
            break;
        }
        str.push_back(ch);
        rlen++;
        if (ch == delimiter) {
            break;
        }
    }
    return rlen;
}

ssize_t basic_stream::size_data_get_size()
{
    if (!___opened) {
        return -1;
    }
    int ch, size = 0, shift = 0;
    while (1) {
        ch = get();
        if (ch == -1) {
            return -1;
        }
        size |= ((ch & 0177) << shift);
        if (ch & 0200) {
            break;
        }
        shift += 7;
    }
    return size;
}

/* write */
bool basic_stream::flush()
{
    return false;
}

basic_stream &basic_stream::puts(const char *str)
{
    if (!___opened) {
        return *this;
    }
    if (str == 0) {
        str = "";
    }
    while (*str) {
        put(*str);
        if (___error || ___eof) {
            break;
        }
        str++;
    }
    return *this;
}

basic_stream &basic_stream::write(const void *_data, size_t size)
{
    if (!___opened) {
        return *this;
    }
    const char *str = (const char *)_data;
    if (size < 1) {
        return *this;
    }
    if (str == 0) {
        str = "";
    }
    while (size--) {
        put(*str);
        if (___error || ___eof) {
            break;
        }
        str++;
    }
    return *this;
}

basic_stream &basic_stream::printf_1024(const char *format, ...)
{
    if (!___opened) {
        return *this;
    }
    va_list ap;
    char buf[1024+1];
    int len;

    if (format == 0) {
        format = "";
    }
    va_start(ap, format);
    len = zcc::vsnprintf(buf, 1024, format, ap);
    va_end(ap);

    return write(buf, len);
}

}
