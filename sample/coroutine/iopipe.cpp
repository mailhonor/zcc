/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-07-18
 * ================================
 */

#include "zcc.h"
#include <poll.h>
#include <openssl/ssl.h>

static char *proxy_address = 0;
static bool proxy_ssl = false;

static char *dest_address = 0;
static bool dest_ssl = false;

static char *ssl_key = 0;
static char *ssl_cert = 0;

static SSL_CTX * ssl_proxy_ctx = 0;
static SSL_CTX * ssl_dest_ctx = 0;

static void ___usage(char *arg = 0)
{

    printf("USAGE: %s -proxy host:port -dest host:port\n", zcc::var_progname);
    printf("USAGE: %s -proxy host:port -ssl-dest host:port\n", zcc::var_progname);
    printf("USAGE: %s -ssl-proxy host:port -dest host:port -ssl-cert filename -ssl-key filename\n", zcc::var_progname);
    exit(1);
}

int times = 999999;
static int ___times = 0;
static int ___stop = 0;
static void after_close(void *ctx)
{
    ___times++;
    fprintf(stderr, "times: %d\n", ___times);
    if (___times == times) {
        ___stop = 1;
        zcc::coroutine_base_stop_notify();
        fprintf(stderr, "... stop\n");
    }
}

static void parameters_do(int argc, char **argv)
{
    zcc::main_parameter_run(argc, argv);

    proxy_address = zcc::default_config.get_str("proxy");
    if (zcc::empty(proxy_address)) {
        proxy_address = zcc::default_config.get_str("ssl-proxy");
        proxy_ssl = true;
    }

    dest_address = zcc::default_config.get_str("dest");
    if (zcc::empty(dest_address)) {
        dest_address = zcc::default_config.get_str("ssl-dest");
        dest_ssl = true;
    }

    ssl_key = zcc::default_config.get_str("ssl_key");
    ssl_cert = zcc::default_config.get_str("ssl_cert");
    times = zcc::default_config.get_int("times", 3, 1, 1000000);

    if (proxy_ssl && dest_ssl) {
        proxy_ssl = false;
        dest_ssl = false;
    }
    if (zcc::empty(proxy_address)) {
        printf("ERR proxy'address is null\n");
        ___usage();
    }
    if (zcc::empty(dest_address)) {
        printf("ERR dest'address is null\n");
        ___usage();
    }
}

static void ssl_do()
{
    zcc::openssl_init();

    ssl_proxy_ctx = zcc::openssl_SSL_CTX_create_server();

    ssl_dest_ctx = zcc::openssl_SSL_CTX_create_client();

    if (proxy_ssl) {
        if (zcc::empty(ssl_key) || zcc::empty(ssl_cert)) {
            printf("ERR ssl-proxy mode, need --ssl-key, --ssl-cert\n");
            ___usage();
        }
        if (!zcc::openssl_SSL_CTX_set_cert(ssl_proxy_ctx, ssl_cert, ssl_key)) {
            printf("ERR can load ssl err: %s, %s\n", ssl_cert, ssl_key);
            exit(1);
        }
    }
}

static void ssl_fini()
{
    zcc::openssl_SSL_CTX_free(ssl_proxy_ctx);
    zcc::openssl_SSL_CTX_free(ssl_dest_ctx);
    zcc::openssl_fini();
}

void * do_after_accept(void *arg)
{
    int proxy_fd = (int)(long)(arg);
    SSL * proxy_ssl = 0;
    int dest_fd = -1;
    SSL *dest_ssl = 0;
    bool err = false;

    do {
        if (proxy_ssl) {
            proxy_ssl = zcc::openssl_SSL_create(ssl_proxy_ctx, proxy_fd);
            if (!zcc::openssl_timed_accept(proxy_ssl, 10 * 1000)) {
                printf("ERR openssl_accept error\n");
                err = true;
                break;
            }
        }
        dest_fd = zcc::connect(dest_address);
        if (dest_fd == -1) {
            printf("ERR can not connect %s\n", dest_address);
            err = true;
            break;
        }
        dest_ssl = zcc::openssl_SSL_create(ssl_dest_ctx, dest_fd);
        if (!zcc::openssl_timed_connect(dest_ssl, 10 * 1000)) {
            printf("ERR openssl_accept error\n");
            err = true;
            break;
        }
    } while(0);

    if (err) {
        if (proxy_ssl) {
            zcc::openssl_SSL_free(proxy_ssl);
        }
        if (proxy_fd != -1) {
            close(proxy_fd);
        }
        if (dest_ssl) {
            zcc::openssl_SSL_free(dest_ssl);
        }
        if (dest_fd != -1) {
            close(dest_fd);
        }
        return 0;
    }
    zcc::coroutine_go_iopipe(proxy_fd, proxy_ssl, dest_fd, dest_ssl, after_close, 0);
    return 0;
}


static void *do_listen(void * arg)
{
    int sock_type;
    int listen_fd = zcc::listen(proxy_address, &sock_type);
    if (listen_fd < 0) {
        printf("ERR: can not open %s (%m), proxy_address\n", proxy_address);
        exit(1);
    }
    while(1) {
        if (___stop) {
            break;
        }
        int fd = zcc::easy_accept(listen_fd, sock_type);
        if (fd < 0) {
            printf("accept error (%m)");
            exit(1);
        }
        zcc::coroutine_go(do_after_accept, (void *)(long)(fd));
    }

    close(listen_fd);
    return arg;
}

int main(int argc, char **argv)
{
    parameters_do(argc, argv);

    ssl_do();

    zcc::coroutine_base_init();

    zcc::coroutine_go(do_listen, 0);

    zcc::coroutine_base_loop();
    zcc::coroutine_base_fini();

    ssl_fini();

    return 0;
}
