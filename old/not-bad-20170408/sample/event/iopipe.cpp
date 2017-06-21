/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-02-07
 * ================================
 */

#include "zcc.h"
#include <pthread.h>
#include <openssl/ssl.h>

static char *proxy_address = 0;
static bool proxy_ssl = false;

static char *dest_address = 0;
static bool dest_ssl = false;

static char *ssl_key = 0;
static char *ssl_cert = 0;

static SSL_CTX * ssl_proxy_ctx = 0;
static SSL_CTX * ssl_dest_ctx = 0;

static zcc::iopipe *iop;
static zcc::event_base *eb;

class  fd_to_fd_linker
{
public:
    inline fd_to_fd_linker() {}
    inline ~fd_to_fd_linker() {}
    zcc::async_io proxy;
    zcc::async_io dest;
};

static void ___usage(char *arg = 0)
{

    printf("USAGE: %s -proxy host:port -dest host:port\n", zcc::var_progname);
    printf("USAGE: %s -proxy host:port -ssl-dest host:port\n", zcc::var_progname);
    printf("USAGE: %s -ssl-proxy host:port -dest host:port --ssl-cert filename -ssl-key filename\n", zcc::var_progname);
    exit(1);
}

static int ___times = 0;
static int ___stop = 0;
void after_close(void *ctx)
{
    ___times++;
    if (___times == 3) {
        iop->stop_notify();
        ___stop = 1;
        eb->notify();
    }
}

static void parameters_do(int argc, char **argv)
{
    ZCC_PARAMETER_BEGIN() {
        if (optval == 0) {
            ___usage();
        }
        if (!strcmp(optname, "-proxy")) {
            proxy_address = optval;
            opti += 2;
            continue;
        }
        if (!strcmp(optname, "-ssl-proxy")) {
            proxy_address = optval;
            proxy_ssl = true;
            opti += 2;
            continue;
        }
        if (!strcmp(optname, "-dest")) {
            dest_address = optval;
            opti += 2;
            continue;
        }
        if (!strcmp(optname, "-ssl-dest")) {
            dest_address = optval;
            dest_ssl = true;
            opti += 2;
            continue;
        }
        if (!strcmp(optname, "-ssl-key")) {
            ssl_key = optval;
            opti += 2;
            continue;
        }
        if (!strcmp(optname, "-ssl-cert")) {
            ssl_cert = optval;
            opti += 2;
            continue;
        }
    }
    ZCC_PARAMETER_END;

    if (proxy_ssl && dest_ssl) {
        proxy_ssl = false;
        dest_ssl = false;
    }
    if (zcc::empty(proxy_address)) {
        printf("ERR: proxy'address is null\n");
        ___usage();
    }
    if (zcc::empty(dest_address)) {
        printf("ERR: dest'address is null\n");
        ___usage();
    }
}

static void ssl_do()
{
    zcc::openssl_init();

    ssl_proxy_ctx = zcc::openssl_create_SSL_CTX_server();

    ssl_dest_ctx = zcc::openssl_create_SSL_CTX_client();

    if (proxy_ssl) {
        if (zcc::empty(ssl_key) || zcc::empty(ssl_cert)) {
            printf("ERR: ssl-proxy mode, need --ssl-key, --ssl-cert\n");
            ___usage();
        }
        if (!zcc::openssl_SSL_CTX_set_cert(ssl_proxy_ctx, ssl_cert, ssl_key)) {
            printf("ERR: can load ssl err: %s, %s\n", ssl_cert, ssl_key);
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

static void after_connect(zcc::async_io &aio)
{
    int ret = aio.get_ret();
    fd_to_fd_linker *jctx = (fd_to_fd_linker *)aio.get_context();
    int proxy_fd = jctx->proxy.get_fd(), dest_fd = jctx->dest.get_fd();
    if (ret < 0) {
        delete jctx;
        close(proxy_fd);
        close(dest_fd);
        return;
    }

    SSL *proxy_SSL = jctx->proxy.detach_SSL();
    SSL *dest_SSL = jctx->dest.detach_SSL();
    iop->enter(proxy_fd, proxy_SSL, dest_fd, dest_SSL, after_close, 0);

    delete jctx;
}

static void after_accept(zcc::async_io &aio)
{
    int proxy_fd;
    int ret = aio.get_ret();
    fd_to_fd_linker *jctx = (fd_to_fd_linker *)aio.get_context();
    if (ret < 0) {
        proxy_fd = aio.get_fd();
        delete jctx;
        close(proxy_fd);
        return;
    }

    int dest_fd = zcc::connect(dest_address, 0);
    if (dest_fd < 0) {
        delete jctx;
        return;
    }

    jctx->dest.init(dest_fd);
    jctx->dest.set_context(jctx);
    if (dest_ssl) {
        jctx->dest.tls_connect(ssl_dest_ctx, after_connect, 10 * 1000);
        return;
    }
    after_connect(jctx->dest);
}

static void start_one(zcc::event_io &eio)
{
    int fd = eio.get_fd();
    int proxy_fd = zcc::inet_accept(fd);
    if (proxy_fd < 0) {
        return;
    }
    zcc::nonblocking(proxy_fd);
    fd_to_fd_linker *jctx = new fd_to_fd_linker();

    jctx->proxy.init(proxy_fd);
    jctx->proxy.set_context(jctx);
    if (proxy_ssl) {
        jctx->proxy.tls_accept(ssl_proxy_ctx, after_accept, 10 * 1000);
        return;
    }
    after_accept(jctx->proxy);
}

static void *accept_incoming(void *arg)
{

    int fd = zcc::listen(proxy_address, 5);
    if (fd < 0) {
        printf("ERR: can not open %s (%m)\n", proxy_address);
        exit(1);
    }

    zcc::event_io eio;
    eio.init(fd);
    eio.enable_read(start_one);

    while(1) {
        zcc::default_evbase.dispatch();
        if (___stop) {
            break;
        }
    }
    eio.fini();
    delete eb;

    return arg;
}

void * iop_run(void *arg)
{
    iop = new zcc::iopipe();
    iop->run();
    delete iop;
    return arg;
}

int main(int argc, char **argv)
{
    zcc::var_log_fatal_catch = 1;
    zcc::var_progname = argv[0];

    parameters_do(argc, argv);

    ssl_do();

    eb = new zcc::event_base();
    pthread_t pth;
#if 0
    pthread_create(&pth, 0, accept_incoming, 0);
    iop_run();
    pthread_join(pth, 0);
#else
    pthread_create(&pth, 0, iop_run, 0);
    accept_incoming(0);
    pthread_join(pth, 0);
#endif

    ssl_fini();

    return 0;
}
