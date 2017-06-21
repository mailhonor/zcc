/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-18
 * ================================
 */

#include "zcc.h"
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace zcc
{


void openssl_init(void)
{
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
}

void openssl_fini(void)
{
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data(); 
    ERR_remove_state(0);
    ERR_free_strings();
}

SSL_CTX *openssl_create_SSL_CTX_server(void)
{
     return SSL_CTX_new(SSLv23_server_method());
}

SSL_CTX *openssl_create_SSL_CTX_client(void)
{
     return SSL_CTX_new(SSLv23_client_method());
}

bool openssl_SSL_CTX_set_cert(SSL_CTX *ctx, const char *cert_file, const char *key_file)
{
    ERR_clear_error();
    if ((!cert_file) || (SSL_CTX_use_certificate_chain_file(ctx, cert_file) <= 0)) {
        return (false);
    }
    if ((!key_file) || (SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) <= 0)) {
        return (false);
    }
    if (!SSL_CTX_check_private_key(ctx)) {
        return (false);
    }

    return true;
}

void openssl_SSL_CTX_free(SSL_CTX * ctx)
{
    SSL_CTX_free(ctx);
}

void openssl_get_error(unsigned long *ecode, char *buf, int buf_len)
{
    unsigned long ec;
    ec = ERR_get_error();
    if (ecode) {
        *ecode = ec;
    }

    if (buf) {
        ERR_error_string_n(ec, buf, buf_len);
    }
}

SSL *openssl_create_SSL(SSL_CTX * ctx, int fd)
{
    SSL *ssl;
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, fd);
    return ssl;
}

void openssl_SSL_free(SSL * ssl)
{
    SSL_shutdown(ssl);
    SSL_free(ssl);
}

int openssl_SSL_get_fd(SSL *ssl)
{
    return SSL_get_fd(ssl);
}

#define ___Z_SSL_TIMED_DO(excute_sentence)  \
    int _fd = SSL_get_fd(ssl); \
    int ret, err; long cirtical_time, left_time; \
    cirtical_time = timeout_set(timeout); \
    for (;;) { \
        ret = excute_sentence; \
        if (ret > 0) { break; } \
        err = SSL_get_error(ssl, ret); \
        switch (err) { \
        case SSL_ERROR_WANT_WRITE: \
            if ((left_time = timeout_left(cirtical_time)) < 1) { ret=-1; goto over; } \
            if (!timed_wait_writeable(_fd, left_time)) { ret=-1; goto over; }\
            break; \
        case SSL_ERROR_WANT_READ: \
            if ((left_time = timeout_left(cirtical_time)) < 1) { ret = -1; goto over;} \
            if (!timed_wait_readable(_fd, left_time)) { ret = -1; goto over; } \
            break; \
        case SSL_ERROR_ZERO_RETURN:  /* FIXME */ \
        case SSL_ERROR_SSL: \
        case SSL_ERROR_SYSCALL: \
        default: \
            if (var_openssl_debug) {zcc_info("openssl: found error"); } \
            ret = -1; goto over; \
            break; \
        } \
    } \
over: 


bool openssl_timed_connect(SSL * ssl, long timeout)
{
    ___Z_SSL_TIMED_DO(SSL_connect(ssl));
    return (ret==1);
}

bool openssl_timed_accept(SSL * ssl, long timeout)
{
    ___Z_SSL_TIMED_DO(SSL_accept(ssl));
    return (ret==1);
}

bool openssl_timed_shutdown(SSL * ssl, long timeout)
{
    ___Z_SSL_TIMED_DO(SSL_shutdown(ssl));
    return (ret==1);
}

ssize_t openssl_timed_read(SSL * ssl, void *buf, size_t len, long timeout)
{
    ___Z_SSL_TIMED_DO(SSL_read(ssl, buf, len));
    return ret;
}

ssize_t openssl_timed_write(SSL * ssl, const void *buf, size_t len, long timeout)
{
    ___Z_SSL_TIMED_DO(SSL_write(ssl, buf, len));
    return ret;
}

}
