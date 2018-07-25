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

namespace zcc
{

class fd_attrs
{
public:
    fd_attrs();
    ~fd_attrs();
    void prepare_get_data(struct pollfd *pf);
    ssize_t try_read(char *buf, size_t size);
    bool strict_write(char *buf, size_t size);
    SSL *ssl;
    int fd;
    bool want_read;
    bool want_write;
    bool error_or_closed;
};

fd_attrs::fd_attrs()
{
    ssl = 0;
    fd = -1;
    want_read = want_write = false;
    error_or_closed = false;
}

fd_attrs::~fd_attrs()
{
    if (ssl) {
        openssl_SSL_free(ssl);
        ssl = 0;
    }
    if (fd != -1) {
        close(fd);
        fd = -1;
    }
}

void fd_attrs::prepare_get_data(struct pollfd *pf)
{
    pf->fd = fd;
    if (want_read) {
        pf->events = POLLIN;
    } else if (want_write) {
        pf->events = POLLOUT;
    } else {
        zcc_fatal("unknown want event");
    }
    pf->revents = 0;
}

ssize_t fd_attrs::try_read(char *buf, size_t size)
{
    int ret;
    want_read = want_write = false;
    if (ssl) {
        ret = SSL_read(ssl, buf, size);
        if (ret > 0) {
            return ret;
        }
        int rr = SSL_get_error(ssl, ret);
        if (rr == SSL_ERROR_WANT_READ) {
            want_read = true;
        } else if (rr == SSL_ERROR_WANT_WRITE) {
            want_write = true;
        } else {
            error_or_closed = true;
        }
        return ret;
    } else {
        ret = read(fd, buf, size);
        if (ret == 0) {
            error_or_closed = true;
        }
        if (ret >= 0) {
            return ret;
        }
        if (errno != EAGAIN && errno != EINTR) {
            error_or_closed = true;
            return -1;
        }
        want_read = true;
        return -1;
    }
}

bool fd_attrs::strict_write(char *buf, size_t size)
{
    int ret;
    size_t wrotelen = 0;
    while(wrotelen < size) {
        if (ssl) {
            ret = SSL_write(ssl, buf + wrotelen, size - wrotelen);
            if (ret > 0) {
                wrotelen += ret;
                continue;
            }
            int rr = SSL_get_error(ssl, ret);
            if (rr == SSL_ERROR_WANT_READ) {
                timed_wait_readable(fd, 10 * 1000);
                continue;
            } else if (rr == SSL_ERROR_WANT_WRITE) {
                timed_wait_writeable(fd, 10 * 1000);
                continue;
            } else {
                error_or_closed = true;
                return false;
            }
        } else {
            ret = write(fd, buf + wrotelen, size - wrotelen);
            if (ret >= 0) {
                wrotelen += ret;
                continue;
            }
            if (errno != EAGAIN && errno != EINTR) {
                error_or_closed = true;
                return false;
            }
            timed_wait_writeable(fd, 10 * 1000);
            continue;
        }
    }
    return true;
}

class fd_attrs_go {
public:
    fd_attrs fass0;
    fd_attrs fass1;
    void (*after_close)(void *ctx);
    void *ctx;
    char rbuf[4096+1];
};

static void *coroutine_go_iopipe_go(void *ctx)
{
    fd_attrs_go *fgo = (fd_attrs_go *)ctx;
    bool need_stop = false;
    bool no_pool = false;
    struct pollfd pollfds[2];
    fd_attrs &fass0 = fgo->fass0;
    fd_attrs &fass1 = fgo->fass1;
    fd_attrs *fass_w;
    char *rbuf = fgo->rbuf;
    int rlen, poll_ret;
    while(1) {
        rlen = 0;
        fass_w = 0;
        if (no_pool) {
            poll_ret = 1;
        } else {
            fass0.prepare_get_data(pollfds);
            fass1.prepare_get_data(pollfds+1);
            poll_ret = poll(pollfds, 2, 10 * 1000);
        }
        switch (poll_ret) {
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
                rlen = fass0.try_read(rbuf, 4096);
                if (rlen > 0) {
                    pollfds[0].revents = POLLIN;
                    pollfds[1].revents = 0;
                    no_pool = true;
                    fass_w = &fass1;
                    break;
                }
                if (rlen == 0) {
                    need_stop = true;
                    break;
                }
            }
            if ((rlen <= 0) && (pollfds[1].revents & (POLLIN | POLLOUT))) {
                rlen = fass1.try_read(rbuf, 4096);
                if (rlen > 0) {
                    fass_w = &fass0;
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
            no_pool = false;
            break;
        }
        if (need_stop) {
            break;
        }

        if (fass0.error_or_closed || fass1.error_or_closed) {
            break;
        }

        if (rlen < 0) {
            continue;
        }
        if (fass_w && (!fass_w->strict_write(rbuf, rlen))) {
            break;
        }

        fass0.want_read = true;
        fass0.want_write = false;
        fass1.want_read = true;
        fass1.want_write = false;
    }
    if (fgo->after_close) {
        fgo->after_close(fgo->ctx);
    }
    delete fgo;
    return 0;
}

void coroutine_go_iopipe(int fd1, SSL *ssl1, int fd2, SSL *ssl2, void (*after_close)(void *ctx), void *ctx)
{
    fd_attrs_go *fgo = new fd_attrs_go();
    fgo->fass0.fd = fd1;
    fgo->fass0.ssl = ssl1;
    fgo->fass0.want_read = true;
    fgo->fass1.fd = fd2;
    fgo->fass1.ssl = ssl2;
    fgo->fass1.want_read = true;
    fgo->after_close = after_close;
    fgo->ctx = ctx;
    coroutine_go(coroutine_go_iopipe_go, fgo, 4096);
}

}
