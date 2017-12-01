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
        proxy_address = zcc::default_config.get_str("proxy");
        proxy_ssl = true;
    }

    dest_address = zcc::default_config.get_str("dest");
    if (zcc::empty(dest_address)) {
        dest_address = zcc::default_config.get_str("dest");
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

    ssl_proxy_ctx = zcc::openssl_create_SSL_CTX_server();

    ssl_dest_ctx = zcc::openssl_create_SSL_CTX_client();

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

typedef struct fd_attrs fd_attrs;
struct fd_attrs {
    int fd;
    SSL *ssl;
    bool want_read;
    bool want_write;
    bool error_or_closed;
};

static void fd_attrs_release(fd_attrs *as)
{
    if (as->ssl) {
        zcc::openssl_SSL_free(as->ssl);
        as->ssl = 0;
    }
    if (as->fd != -1) {
        close(as->fd);
        as->fd = -1;
    }
}

static bool do_join(int proxy_fd, fd_attrs *fass)
{
    memset(fass, 0, sizeof(fd_attrs) * 2);
    fass[0].fd = proxy_fd;
    fass[1].fd = -1;
    if (proxy_ssl) {
        fass[0].ssl = zcc::openssl_create_SSL(ssl_proxy_ctx, proxy_fd);
        if (!zcc::openssl_timed_accept(fass[0].ssl, 10 * 1000)) {
            printf("ERR: ssl accept error\n");
            fd_attrs_release(fass);
            return false;
        }
    }

    int dest_fd = zcc::connect(dest_address);
    if (dest_fd < 0) {
        printf("ERR: can not open %s(%m)", dest_address);
        fd_attrs_release(fass);
        return false;
    }
    fass[1].fd = dest_fd;
    if (dest_ssl) {
        fass[1].ssl = zcc::openssl_create_SSL(ssl_dest_ctx, dest_fd);
        if (!zcc::openssl_timed_connect(fass[1].ssl, 10 * 1000)) {
            printf("ERR: ssl connect error\n");
            fd_attrs_release(fass +1);
            fd_attrs_release(fass);
            return false;
        }
    }
    return true;
}

static void prepare_get_data(fd_attrs *as, struct pollfd *pf)
{
    pf->fd = as->fd;
    if (as->want_read) {
        pf->events = POLLIN;
    } else if (as->want_write) {
        pf->events = POLLOUT;
    } else {
        printf("ERR: unknown want event\n");
        exit(1);
    }
    pf->revents = 0;
}

static ssize_t fd_attrs_read(fd_attrs *as, char *buf, size_t size)
{
    int ret;
    as->want_read = as->want_write = false;
    if (as->ssl) {
        ret = SSL_read(as->ssl, buf, size);
        if (ret > 0) {
            return ret;
        }
        int rr = SSL_get_error(as->ssl, ret);
        printf("SSL_read:%d,  %d\n", ret, rr);
        if (rr == SSL_ERROR_WANT_READ) {
            as->want_read = true;
        } else if (rr == SSL_ERROR_WANT_WRITE) {
            as->want_write = true;
        } else {
            as->error_or_closed = true;
        }
        return ret;
    } else {
        ret = read(as->fd, buf, size);
        if (ret == 0) {
            as->error_or_closed = true;
        }
        if (ret >= 0) {
            return ret;
        }
        if (errno != EAGAIN && errno != EINTR) {
            as->error_or_closed = true;
            return -1;
        }
        as->want_read = true;
        return -1;
    }
}

static bool fd_attrs_write_strict(fd_attrs *as, char *buf, size_t size)
{
    int ret;
    size_t wrotelen = 0;
    while(wrotelen < size) {
        if (as->ssl) {
            ret = SSL_write(as->ssl, buf + wrotelen, size - wrotelen);
            if (ret > 0) {
                wrotelen += ret;
                continue;
            }
            int rr = SSL_get_error(as->ssl, ret);
            if (rr == SSL_ERROR_WANT_READ) {
                zcc::timed_wait_readable(as->fd, 10 * 1000);
                continue;
            } else if (rr == SSL_ERROR_WANT_WRITE) {
                zcc::timed_wait_writeable(as->fd, 10 * 1000);
                continue;
            } else {
                as->error_or_closed = true;
                return false;
            }
        } else {
            ret = write(as->fd, buf + wrotelen, size - wrotelen);
            if (ret >= 0) {
                wrotelen += ret;
                continue;
            }
            if (errno != EAGAIN && errno != EINTR) {
                as->error_or_closed = true;
                return false;
            }
            zcc::timed_wait_writeable(as->fd, 10 * 1000);
            continue;
        }
    }
    return true;
}

static void *do_after_accept(void *arg)
{
    int proxy_fd = (int)(long)(arg);
    struct fd_attrs fass[2], *fass_w = 0;
    if (!do_join(proxy_fd, fass)) {
        return arg;
    }
    fass[0].want_read = true;
    fass[1].want_read = true;
    bool need_stop = false;
    struct pollfd pollfds[2];
    char rbuf[4096 + 1];
    int rlen;
    bool no_pool = false;
    while(1) {
        printf("\n\nread write loop\n");
        rlen = 0;
        int pret;
        if (no_pool) {
            printf("no pool\n");
            pret = 1;
        } else {
            prepare_get_data(fass, pollfds);
            prepare_get_data(fass + 1, pollfds + 1);
            pret = poll(pollfds, 2, 10 * 1000);
        }
        switch (pret) {
        case -1:
            if (errno != EINTR) {
                need_stop = true;
                break;
            }
            continue;
        case 0:
            continue;
        default:
            if (pollfds[0].revents & (POLLIN | POLLOUT)) {
                printf("before read 1\n");
                rlen = fd_attrs_read(fass, rbuf, 4096);
                printf("rlen 1: %d\n", rlen);
                if (rlen > 0) {
                    pollfds[0].revents = POLLIN;
                    pollfds[1].revents = 0;
                    no_pool = true;
                    fass_w = fass + 1;
                    break;
                }
                if (rlen == 0) {
                    need_stop = true;
                    break;
                }
            }
            if ((rlen <= 0) && (pollfds[1].revents & (POLLIN | POLLOUT))) {
                printf("before read 2\n");
                rlen = fd_attrs_read(fass + 1, rbuf, 4096);
                printf("rlen 2: %d\n", rlen);
                if (rlen > 0) {
                    fass_w = fass;
                    pollfds[0].revents = 0;
                    pollfds[1].revents = POLLIN;
                    no_pool = true;
                    break;
                }
                if (rlen == 0) {
                    need_stop = true;
                    break;
                }
            }
            printf("need poll\n");
            no_pool = false;
        }
        if (need_stop) {
            break;
        }

        if (fass[0].error_or_closed || fass[1].error_or_closed) {
            break;
        }

        if (rlen < 0) {
            continue;
        }
        printf("write\n");
        if (!fd_attrs_write_strict(fass_w, rbuf, rlen)) {
            break;
        }

        printf("reset want event\n");
        fass[0].want_read = true;
        fass[0].want_write = false;
        fass[1].want_read = true;
        fass[1].want_write = false;
    }
    fd_attrs_release(fass);
    fd_attrs_release(fass + 1);
    after_close(0);

    return arg;
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
        int fd = zcc::accept(listen_fd, sock_type);
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
    zcc::var_log_fatal_catch = 1;
    zcc::var_progname = argv[0];

    parameters_do(argc, argv);

    ssl_do();

    zcc::coroutine_base_init();

    zcc::coroutine_go(do_listen, 0);


    zcc::coroutine_base_loop();
    zcc::coroutine_base_fini();

    ssl_fini();

    return 0;
}
