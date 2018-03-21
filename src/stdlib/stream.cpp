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
    read_buf = (char *) malloc(stream_read_buf_size + 1 + stream_write_buf_size+ 1);
    init_all();
}

stream::stream(SSL *ssl)
{
    read_buf = (char *) malloc(stream_read_buf_size + 1 + stream_write_buf_size+ 1);
    init_all();
    open(ssl);
}

stream::stream(int fd)
{
    read_buf = (char *) malloc(stream_read_buf_size + 1 + stream_write_buf_size+ 1);
    init_all();
    open(fd);
}

stream::~stream()
{
    close();
    free(read_buf);
}

stream &stream::open(SSL *ssl)
{
    close();
    init_all();
    ___opened = true;
    ___ssl_opened_mode = true;
    ___ssl_mode = true;
    ___fd.ssl = ssl;
    return *this;
}

stream &stream::open(int fd)
{
    close();
    init_all();
    ___opened = true;
    ___fd.fd = fd;
    return *this;
}

stream &stream::close()
{
    if (!___opened) {
        return *this;
    }
    flush();
    if (___auto_release) {
        if (___ssl_mode) {
            int fd = openssl_SSL_get_fd(___fd.ssl);
            openssl_SSL_free(___fd.ssl);
            ::close(fd);
        } else {
            ::close(___fd.fd);
        }
    } else {
        if (___ssl_mode && (!___ssl_opened_mode)) {
            openssl_SSL_free(___fd.ssl);
        }
    }
    ___opened = false;

    return *this;
}

stream &stream::set_timeout(long timeout)
{
    if (timeout < 1) {
        timeout = 1000L * 3600 * 24 * 365 * 10;
    }
    ___timeout = timeout_set(timeout);
    return *this;
}

int stream::timed_wait_readable(long timeout)
{
    if (!___opened) {
        return -1;
    }
    if (read_buf_p1 < read_buf_p2) {
        return 1;
    }
    return zcc::timed_wait_readable(___ssl_mode?openssl_SSL_get_fd(___fd.ssl):___fd.fd, timeout);
}

int stream::timed_wait_writeable(long timeout)
{
    if (!___opened) {
        return -1;
    }
    return zcc::timed_wait_writeable(___ssl_mode?openssl_SSL_get_fd(___fd.ssl):___fd.fd, timeout);
}

/* read */
int stream::get_char_do()
{
    if (!___opened) {
        return -1;
    }
    int ret;
    long time_left;

    if (read_buf_p1 < read_buf_p2) {
        return read_buf[read_buf_p1++];
    }

    if (___error || ___eof) {
        return -1;
    }

    if (write_buf_len > 0) {
        flush();
    }

    if (___error || ___eof) {
        return -1;
    }

    time_left = timeout_left(___timeout);
    if (time_left < 1) {
        ___error = true;
        return -1;
    }

    read_buf_p1 = read_buf_p2 = 0;
    if(___ssl_mode) {
        ret = openssl_timed_read(___fd.ssl, read_buf, stream_read_buf_size, time_left);
    } else {
        ret = timed_read(___fd.fd, read_buf, stream_read_buf_size, time_left);
    }
    if (ret == 0) {
        ___eof = true;
        return -1;
    }
    if (ret < 0) {
        ___error = true;
        return -1;
    }
    read_buf_p2 = ret;

    return read_buf[read_buf_p1++];
}

ssize_t stream::readn(void *buf, size_t size)
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

ssize_t stream::readn(std::string &str, size_t size)
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

ssize_t stream::gets(void *buf, size_t size, int delimiter)
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

ssize_t stream::gets(std::string &str, int delimiter)
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
    if (rlen) {
        return rlen;
    }
    return -1;
}

ssize_t stream::size_data_get_size()
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

bool stream::flush()
{
    if (!___opened) {
        return false;
    }
    ssize_t ret;
    long time_left;
    char *data = write_buf;
    size_t data_len =write_buf_len;
    size_t wlen = 0;

    ___flushed = true;

    if (___error) {
        return false;
    }
    if (write_buf_len < 1) {
        return true;
    }

    while (wlen < data_len) {
        time_left = timeout_left(___timeout);
        if (time_left < 1) {
            ___error = true;
            return false;
        }
        if (___ssl_mode) {
            ret = openssl_timed_write(___fd.ssl, data + wlen, data_len - wlen, time_left);
        } else {
            ret = timed_write(___fd.fd, data + wlen, data_len - wlen, time_left);
        }
        if (ret < 0) {
            ___error = true;
            return false;
        }
        if (ret == 0) {
            ___error = true;
            return false;
        }
        wlen += ret;
    }
    write_buf_len = 0;

    return true;
}

stream &stream::puts(const char *str)
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

stream &stream::write(const void *_data, size_t size)
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

stream &stream::printf_1024(const char *format, ...)
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

void stream::init_all()
{
    read_buf_p1 = 0;
    read_buf_p2 = 0;
    write_buf_len = 0;
    ___opened = false;
    ___auto_release = false;
    ___ssl_mode = false;
    ___ssl_opened_mode = false;
    ___error = false;
    ___eof = false;
    ___flushed = false;
    write_buf = read_buf + stream_read_buf_size + 1;
    set_timeout(-1);
}

int stream::get_fd()
{
    return (___opened?(___ssl_mode?openssl_SSL_get_fd(___fd.ssl):___fd.fd):-1);
}

SSL *stream::get_SSL()
{
    return (___opened&&___ssl_mode)?___fd.ssl:0;
}

bool stream::tls_connect(SSL_CTX *ctx)
{
    if (!___opened) {
        return false;
    }
    if (___ssl_mode) {
        return true;
    }

    SSL *_ssl = openssl_create_SSL(ctx, ___fd.fd);
    if (!_ssl) {
        return false;
    }
    if (!openssl_timed_connect(_ssl, timeout_left(get_timeout()))) {
        openssl_SSL_free(_ssl);
        _ssl = 0;
        return false;
    }
    ___ssl_mode = true;
    ___fd.ssl = _ssl;
    return true;
}

bool stream::tls_accept(SSL_CTX *ctx)
{
    if (!___opened) {
        return false;
    }
    if (___ssl_mode) {
        return true;
    }

    SSL *_ssl = openssl_create_SSL(ctx, ___fd.fd);
    if (!_ssl) {
        return false;
    }
    if (!openssl_timed_accept(_ssl, timeout_left(get_timeout()))) {
        openssl_SSL_free(_ssl);
        _ssl = 0;
        return false;
    }
    ___ssl_mode = true;
    ___fd.ssl = _ssl;
    return true;
}

}
