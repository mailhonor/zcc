/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-12-02
 * ================================
 */

#include "zcc.h"

namespace zcc
{

/* ssl */
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

/* tls */
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
