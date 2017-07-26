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
    ___error = false;
    ___eof = false;
    ___flushed = false;
    set_timeout(1000L * 3600 * 24 * 365 * 10);
}

stream::~stream()
{
    /* flush must be do in parent-class */
    if (!___flushed) {
        zcc_fatal("Derived classe of the zcc::stream should do flush when unconstruct.");
    }
}

stream &stream::set_timeout(long timeout)
{
    if (timeout == -1) {
        timeout = 1000L * 3600 * 24 * 365 * 10;
    }
    ___timeout = timeout_set(timeout);
    return *this;
}

long stream::get_timeout()
{
    return ___timeout;
}

/* read */
int stream::get_char_do()
{
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
    ret = read_fn(read_buf, stream_read_buf_size, time_left);
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
    char *ptr = (char *)buf;
    size_t left_size = size;
    int ch;

    if (size < 1) {
        return 0;
    }
    while (left_size) {
        ch = get();
        if (ch == -1) {
            break;
        }
        *ptr++ = ch;
        left_size-- ;
    }

    if (size > left_size) {
        return size - left_size;
    }
    return -1;
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
    char *ptr = (char *)buf;
    size_t left_size = size;
    int ch;

    if (size < 1) {
        return 0;
    }

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

    if (size > left_size) {
        return size - left_size;
    }
    return -1;
}

ssize_t stream::gets(std::string &str, int delimiter)
{
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
        ret = write_fn(data + wlen, data_len - wlen, time_left);
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


/* ssl ################################################################ */
sslstream::sslstream(SSL *ssl)
{
    _ssl = ssl;
}

sslstream::~sslstream()
{
    flush();
}

SSL *sslstream::get_SSL()
{
    return _ssl;
}

ssize_t sslstream::read_fn(void *buf, size_t size, long timeout)
{
    return openssl_timed_read(_ssl, buf, size, timeout);
};
ssize_t sslstream::write_fn(const void *buf, size_t size, long timeout)
{
    return openssl_timed_write(_ssl, buf, size, timeout);
};

/* tls ################################################################ */
tlsstream::tlsstream(int fd)
{
    _fd = fd;
    _ssl = 0;
}

tlsstream::~tlsstream()
{
    flush();
}

int tlsstream::get_fd()
{
    return _fd;
}

SSL *tlsstream::get_SSL()
{
    return _ssl;
}

bool tlsstream::tls_connect(SSL_CTX *ctx)
{
    long time_left = timeout_left(get_timeout());
    _ssl = openssl_create_SSL(ctx, _fd);
    if (!_ssl) {
        return false;
    }
    if (!openssl_timed_connect(_ssl, time_left)) {
        openssl_SSL_free(_ssl);
        _ssl = 0;
        return false;
    }
    return true;
}

bool tlsstream::tls_accept(SSL_CTX *ctx)
{
    long time_left = timeout_left(get_timeout());
    _ssl = openssl_create_SSL(ctx, _fd);
    if (!_ssl) {
        return false;
    }
    if (!openssl_timed_accept(_ssl, time_left)) {
        openssl_SSL_free(_ssl);
        _ssl = 0;
        return false;
    }
    return true;
}

ssize_t tlsstream::read_fn(void *buf, size_t size, long timeout)
{
    return ((_ssl)?(openssl_timed_read(_ssl, buf, size, timeout)):(timed_read(_fd, buf, size, timeout)));
};
ssize_t tlsstream::write_fn(const void *buf, size_t size, long timeout)
{
    return ((_ssl)?(openssl_timed_write(_ssl, buf, size, timeout)):(timed_write(_fd, buf, size, timeout)));
};

}
