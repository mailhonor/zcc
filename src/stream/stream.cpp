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

stream::stream()
{
    init();
}

stream::stream(SSL *ssl)
{
    init();
    open(ssl);
}

stream::stream(int fd)
{
    init();
    open(fd);
}

stream::stream(const char *path, const char *mode)
{
    init();
    open(path, mode);
}

stream::~stream()
{
    close();
    fini();
}

stream &stream::open(SSL *ssl)
{
    close();
    ___opened = true;
    ___ssl_opened_mode = true;
    ___ssl_mode = true;
    ___fd.ssl = ssl;
    return *this;
}

stream &stream::open(int fd)
{
    close();
    ___opened = true;
    ___fd.fd = fd;
    return *this;
}

bool stream::open(const char *pathname, const char *mode)
{
    close();
    int fd, flags;
    if (*mode == 'r') {
        flags = O_RDONLY;
        if (mode[1] == '+') {
            flags = O_RDWR;
        }
    } else  if (*mode == 'w') {
        flags = O_WRONLY|O_TRUNC|O_CREAT;
        if (mode[1] == '+') {
            flags = O_RDWR|O_TRUNC|O_CREAT;
        }
    } else  if (*mode == 'a') {
        flags = O_WRONLY|O_TRUNC|O_CREAT|O_APPEND;
        if (mode[1] == '+') {
            flags = O_RDWR|O_TRUNC|O_CREAT|O_APPEND;
        }
    } else {
        flags = O_RDONLY;
    }
    fd = ::open(pathname, flags, 0666);
    if (fd == -1) {
        ___error = true;
        return false;
    }
    open(fd);
    set_auto_close_fd();
    return true;
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
    basic_stream::reset();
    ___opened = false;
    ___auto_release = false;
    ___ssl_mode = false;
    ___ssl_opened_mode = false;
    set_timeout(-1);

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
    read_buf_p1 = 1;
    read_buf_p2 = ret;

    return read_buf[0];
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

void stream::init()
{
    ___auto_release = false;
    ___ssl_mode = false;
    ___ssl_opened_mode = false;
    set_timeout(-1);
}

void stream::fini()
{
}

int stream::get_fd()
{
    return (___opened?(___ssl_mode?openssl_SSL_get_fd(___fd.ssl):___fd.fd):-1);
}

SSL *stream::get_SSL()
{
    return (___opened&&___ssl_mode)?___fd.ssl:0;
}

int stream::detach_fd()
{
    if (!___opened) {
        return -1;
    }
    if (___ssl_mode) {
        return -1;
    }
    int fd = ___fd.fd;
    basic_stream::reset();
    ___opened = false;
    ___auto_release = false;
    ___ssl_mode = false;
    ___ssl_opened_mode = false;
    set_timeout(-1);
    return fd;
}

SSL *stream::detach_SSL()
{
    if (!___opened) {
        return 0;
    }

    if (!___ssl_mode) {
        return 0;
    }
    SSL *ssl = ___fd.ssl;
    basic_stream::reset();
    ___opened = false;
    ___auto_release = false;
    ___ssl_mode = false;
    ___ssl_opened_mode = false;
    set_timeout(-1);
    return ssl;
}

bool stream::tls_connect(SSL_CTX *ctx)
{
    if (!___opened) {
        return false;
    }
    if (___ssl_mode) {
        return true;
    }

    SSL *_ssl = openssl_SSL_create(ctx, ___fd.fd);
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

    SSL *_ssl = openssl_SSL_create(ctx, ___fd.fd);
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
