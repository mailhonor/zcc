/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-11-30
 * ================================
 */

#include "zcc.h"

namespace zcc
{

stream::stream()
{
    read_buf_p1 = 0;
    read_buf_p2 = 0;
    write_buf_len = 0;
    ___stream_error = false;
    ___stream_eof = false;
    set_timeout(1000L * 3600 * 24 * 365 * 10);
}

stream::~stream()
{
#if 0
    /* flush must be do in parent-class */
    if (!_flushed) {
        zfatal("Derived classe of the zcc::stream should do flush when unconstruct.");
    }
#endif
}

stream &stream::set_timeout(long timeout)
{
    _timeout = timeout_set(timeout);
    return *this;
}

long stream::get_timeout()
{
    return _timeout;
}

/* read */
int stream::get_char_do()
{
    int ret;
    long time_left;

    if (read_buf_p1 < read_buf_p2) {
        return read_buf[read_buf_p1++];
    }

    if (error() || eof()) {
        return -1;
    }

    flush();

    if (error() || eof()) {
        return -1;
    }

    time_left = timeout_left(_timeout);
    if (time_left < 1) {
        ___stream_error = true;
        return -1;
    }

    read_buf_p1 = read_buf_p2 = 0;
    ret = read_fn(read_buf, stream_read_buf_size, time_left);
    if (ret == 0) {
        ___stream_eof = true;
        return -1;
    }
    if (ret < 0) {
        ___stream_error = true;
        return -1;
    }
    read_buf_p2 = ret;

    return read_buf[read_buf_p1++];
}

ssize_t stream::readn(void *buf, size_t size)
{
    char *ptr = (char *)buf;
    size_t left_size = size;
    int ch;

    if (size < 1) {
        return 0;
    }
    while (left_size) {
        ch = get();
        if (___stream_error || ___stream_eof) {
            break;
        }
        *ptr++ = ch;
        left_size-- ;
    }

    return size - left_size;
}

ssize_t stream::readn(std::string &str, size_t size)
{
    size_t left_size = size;
    int ch;

    str.clear();

    if (size < 1) {
        return 0;
    }
    while (left_size) {
        ch = get();
        if (___stream_error || ___stream_eof) {
            break;
        }
        str.push_back(ch);
        left_size--;
    }

    return size - left_size;
}

ssize_t stream::gets(void *buf, size_t size, int delimiter)
{
    char *ptr = (char *)buf;
    size_t left_size = size;
    int ch;

    if (size < 1) {
        return 0;
    }

    while(left_size) {
        ch = get();
        if (___stream_error || ___stream_eof) {
            break;
        }
        *ptr++ = ch;
        left_size --;
        if (ch == delimiter) {
            break;
        }
    }

    return size - left_size;
}

ssize_t stream::gets(std::string &str, int delimiter)
{
    int ch;
    ssize_t rlen = 0;

    str.clear();

    while(1) {
        ch = get();
        if (___stream_error || ___stream_eof) {
            if (!rlen) {
                return -1;
            }
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

ssize_t stream::size_data_get_size()
{
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

bool stream::flush()
{
    ssize_t ret;
    long time_left;
    char *data = write_buf;
    size_t data_len =write_buf_len;
    size_t wlen = 0;

    if (___stream_error) {
        return false;
    }
    if (write_buf_len < 1) {
        return true;
    }

    while (wlen < data_len) {
        time_left = timeout_left(_timeout);
        if (time_left < 1) {
            ___stream_error = true;
            return false;
        }
        ret = write_fn(data + wlen, data_len - wlen, time_left);
        if (ret < 0) {
            ___stream_error = true;
            return false;
        }
        if (ret == 0) {
            ___stream_eof = true;
            ___stream_error = true;
            return false;
        }
        wlen += ret;
    }
    write_buf_len = 0;

    return true;
}

stream &stream::puts(const char *str)
{
    if (str == 0) {
        str = "";
    }
    while (*str) {
        put(*str);
        if (___stream_error || ___stream_eof) {
            break;
        }
        str++;
    }
    return *this;
}

stream &stream::writen(const void *_data, size_t size)
{
    const char *str = (const char *)_data;
    if (size < 1) {
        return *this;
    }
    if (str == 0) {
        str = "";
    }
    while (size--) {
        put(*str);
        if (___stream_error || ___stream_eof) {
            break;
        }
        str++;
    }
    return *this;
}

stream &stream::printf_1024(const char *format, ...)
{
    va_list ap;
    char buf[1024+1];
    int len;

    if (format == 0) {
        format = "";
    }
    va_start(ap, format);
    len = zcc::vsnprintf(buf, 1024, format, ap);
    va_end(ap);

    return writen(buf, len);
}

/* io ################################################################# */
iostream::iostream(int fd)
{
    _fd = fd;
}

iostream::~iostream()
{
    flush();
}

ssize_t iostream::read_fn(void *buf, size_t size, long timeout)
{
    return timed_read(_fd, buf, size, timeout);
}

ssize_t iostream::write_fn(const void *buf, size_t size, long timeout)
{
    return timed_write(_fd, buf, size, timeout);
}

int iostream::get_fd()
{
    return _fd;
}

}
