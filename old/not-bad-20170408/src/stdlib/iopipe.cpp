/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-02-06
 * ================================
 */

#include "zcc.h"

namespace zcc
{

#include <openssl/ssl.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#define ZIOPIPE_RBUF_SIZE	    	4095
#define ZIOPIPE_BASE_LOCK(iopb)		if (pthread_mutex_lock(&(iopb->locker))) { log_fatal("mutex:%m"); }
#define ZIOPIPE_BASE_UNLOCK(iopb)	if (pthread_mutex_unlock(&(iopb->locker))) { log_fatal("mutex:%m"); }

typedef struct iopipe_t iopipe_t;
typedef struct iopipe_part_t iopipe_part_t;
typedef struct iopipe_linker_t iopipe_linker_t;

struct iopipe_part_t {
    unsigned char is_client_or_server:1;
    unsigned char read_want_read:1;
    unsigned char read_want_write:1;
    unsigned char write_want_read:1;
    unsigned char write_want_write:1;
    unsigned char ssl_error:1;
    unsigned char event_in:1;
    unsigned char et_read:1;
    unsigned char et_write:1;
    unsigned char et_hup:1;
    int rbuf_p1:16;
    int rbuf_p2:16;
    int fd;
    char *rbuf;
    SSL *ssl;
};

struct iopipe_t {
    iopipe_part_t client;
    iopipe_part_t server;
    iopipe_after_close_fn_t after_close;
    void *context;
};

struct iopipe_linker_t {
    int cfd;
    int sfd;
    SSL *cssl;
    SSL *sssl;
    iopipe_linker_t *prev;
    iopipe_linker_t *next;
    iopipe_after_close_fn_t after_close;
    void *context;
};

struct iopipe_base_t {
    unsigned int break_flag:1;
    int epoll_fd;
    int epoll_event_count;
    struct epoll_event *epoll_event_list;
    pthread_mutex_t locker;
    iopipe_linker_t *set_list_head;
    iopipe_linker_t *set_list_tail;
    mem_piece *rbuf_mpool;
    mem_piece *iop_mpool;
    iopipe_t eventfd_iop;
};

static void iopipe_set_event(iopipe_base_t * iopb, iopipe_part_t * part, int enable)
{
    int fd, event_in, e_events;
    struct epoll_event evt;

    fd = part->fd;
    e_events = EPOLLHUP | EPOLLRDHUP | EPOLLERR | EPOLLET | EPOLLIN| EPOLLOUT;

    event_in = part->event_in;
    part->event_in = enable;
    if (enable == 0) {
        if (event_in) {
            if (epoll_ctl(iopb->epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
                log_fatal("iopipe_set_event: fd %d: DEL  error: %m", fd);
            }
        }
    } else if (event_in != enable) {
        evt.events = e_events;
        evt.data.ptr = part;
        if (epoll_ctl(iopb->epoll_fd, EPOLL_CTL_ADD, fd, &evt) == -1) {
            log_fatal("iopipe_set_event: fd %d: ADD error: %m", fd);
        }
    }
}

static inline int try_ssl_write(iopipe_part_t * part, void *buf, int len)
{
    int rlen, status, eo;

    rlen = SSL_write(part->ssl, buf, len);
    if (rlen < 1) {
        eo = errno;
        status = SSL_get_error(part->ssl, rlen);
       if (status == SSL_ERROR_WANT_WRITE) {
           part->write_want_write = 0;
           part->write_want_read = 0;
            part->write_want_write = 1;
        } else if (status == SSL_ERROR_WANT_READ) {
           part->write_want_write = 0;
           part->write_want_read = 0;
            part->write_want_read = 1;
        } else if (status == SSL_ERROR_SYSCALL) {
            if (eo && (eo != EAGAIN)) {
                part->ssl_error = 1;
            }
        } else {
            part->ssl_error = 1;
        }
        return -1;
    }

    return rlen;
}

static inline int try_ssl_read(iopipe_part_t * part, void *buf, int len)
{
    int rlen, status, eo;

    rlen = SSL_read(part->ssl, buf, len);
    if (rlen < 1) {
        eo = errno;
        status = SSL_get_error(part->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE) {
            part->read_want_write = 0;
            part->read_want_read = 0;
            part->read_want_write = 1;
        } else if (status == SSL_ERROR_WANT_READ) {
            part->read_want_write = 0;
            part->read_want_read = 0;
            part->read_want_read = 1;
        } else if (status == SSL_ERROR_SYSCALL) {
            if (eo && (eo != EAGAIN)) {
                part->ssl_error = 1;
            }
        } else {
            part->ssl_error = 1;
        }
        return -1;
    }
    return rlen;
}

iopipe_base_t *iopipe_base_create(void)
{
    iopipe_base_t *iopb;
    int efd;

    iopb = (iopipe_base_t *) calloc(1, sizeof(iopipe_base_t));

    pthread_mutex_init(&(iopb->locker), 0);

    iopb->iop_mpool = new mem_piece(sizeof(iopipe_t));
    iopb->rbuf_mpool = new mem_piece(ZIOPIPE_RBUF_SIZE);

    iopb->epoll_fd = epoll_create(1024);
    close_on_exec(iopb->epoll_fd);

    iopb->epoll_event_count = 4096;
    iopb->epoll_event_list = (struct epoll_event *)malloc(sizeof(struct epoll_event) * iopb->epoll_event_count);

    efd = eventfd(0, 0);
    nonblocking(efd);
    close_on_exec(efd);

    iopb->eventfd_iop.client.fd = efd;
    iopb->eventfd_iop.client.is_client_or_server = 0;
    iopipe_set_event(iopb, &(iopb->eventfd_iop.client), 1);

    return iopb;
}

void iopipe_base_notify_stop(iopipe_base_t * iopb)
{
    iopb->break_flag = 1;
}

static inline void write_read_loop(iopipe_base_t *iopb, iopipe_part_t *part, iopipe_part_t *part_a, int len)
{
    int have_data = 1, wlen, rlen;
    if (len < 1) {
        have_data = 0;
    }
    while(1) {
        if (have_data == 1) {
            if (part->ssl) {
                wlen = try_ssl_write(part, part_a->rbuf + part_a->rbuf_p1, len);
            } else {
                wlen = (int)write(part->fd, part_a->rbuf + part_a->rbuf_p1, (size_t) len);
            }
            if (wlen > 0) {
                part_a->rbuf_p1 += wlen;
                if (len == wlen) {
                    part_a->rbuf_p1 = part_a->rbuf_p2 = 0;
                    have_data = 0;
                    continue;
                }
            } else if (part->ssl) {
                if (errno == EAGAIN) {
                    if (part->write_want_write) {
                        part->et_write = 0;
                    }
                    if (part->write_want_read) {
                        part->et_read = 0;
                    }
                }
                return;
            } else {
                if (errno == EAGAIN) {
                    part->et_write = 0;
                }
                return;
            }
        } else {
            if (part_a->rbuf==0) {
                part_a->rbuf = (char *)iopb->rbuf_mpool->require();
            }
            if (part_a->ssl) {
                rlen = try_ssl_read(part_a, part_a->rbuf, ZIOPIPE_RBUF_SIZE);
            } else {
                rlen = read(part_a->fd, part_a->rbuf, ZIOPIPE_RBUF_SIZE);
            }
            if (rlen > 0) {
                part_a->rbuf_p2 = rlen;
                len = rlen;
                have_data = 1;
                continue;
            }
            iopb->rbuf_mpool->release(part_a->rbuf);
            part_a->rbuf = 0;
            if (part_a->ssl) {
                if (errno == EAGAIN) {
                    if (part_a->read_want_write) {
                        part_a->et_write = 0;
                    }
                    if (part_a->read_want_read) {
                        part_a->et_read = 0;
                    }
                }
                return;
            } else {
                if (errno == EAGAIN) {
                    part_a->et_read = 0;
                }
                return;
            }
        }
    }
}

static inline void read_write_loop(iopipe_base_t *iopb, iopipe_part_t *part, iopipe_part_t *part_a, int len)
{
    int have_data = 1, rlen, wlen;

    if (len == 0) {
        part->rbuf_p1 = part->rbuf_p2 = 0;
        have_data = 0;
    }
    if (!(part->rbuf)) {
        part->rbuf = (char *)iopb->rbuf_mpool->require();
    }

    while(1) {
        if (have_data == 0) {
            part->rbuf_p1 = 0;
            if (part->ssl) {
                rlen = try_ssl_read(part, part->rbuf, ZIOPIPE_RBUF_SIZE);
            } else {
                rlen = read(part->fd, part->rbuf, ZIOPIPE_RBUF_SIZE);
            }

            if (rlen > 0) {
                part->rbuf_p2 = rlen;
                have_data = 1;
                continue;
            } else if (part->ssl) {
                if (errno == EAGAIN) {
                    if (part->read_want_write) {
                        part->et_write = 0;
                    }
                    if (part->read_want_read) {
                        part->et_read = 0;
                    }
                }
                return;
            } else {
                if (errno == EAGAIN) {
                    part->et_read = 0;
                }
                return;
            }
        } else {
            rlen =  part->rbuf_p2 - part->rbuf_p1;
            if (part_a->ssl) {
                wlen = try_ssl_write(part_a, part->rbuf + part->rbuf_p1, rlen);
            } else {
                wlen = (int)write(part_a->fd, part->rbuf + part->rbuf_p1, (size_t) rlen);
            }
            if (wlen > 0) {
                if (rlen == wlen) {
                    part->rbuf_p1 = part->rbuf_p2 = 0;
                    have_data = 0;
                    continue;
                }
                have_data = 1;
                continue;
            }
            if (part_a->ssl) {
                if (errno == EAGAIN) {
                    if (part_a->read_want_write) {
                        part_a->et_write = 0;
                    }
                    if (part_a->read_want_read) {
                        part_a->et_read = 0;
                    }
                }
                return;
            } else {
                if (errno == EAGAIN) {
                    part_a->et_write = 0;
                }
                return;
            }
            return;
        }
    }
}

int iopipe_base_run(iopipe_base_t * iopb)
{
    int i, nfds, events, is_client_or_server, e_err;
    struct epoll_event *ev;
    iopipe_t *iop;
    iopipe_part_t *part, *part_a, *part_client, *part_server;
    int efd;
    iopipe_linker_t *linker, *h, *t;

#define  ___no_indent_while_beign   while(1) {
#define  ___no_indent_while_end     }

    efd = iopb->eventfd_iop.client.fd;

    ___no_indent_while_beign;

    while (iopb->set_list_head) {
        ZIOPIPE_BASE_LOCK(iopb);
        linker = iopb->set_list_head;
        if (!linker) {
            ZIOPIPE_BASE_UNLOCK(iopb);
            break;
        }
        h = (iopipe_linker_t *) (iopb->set_list_head);
        t = (iopipe_linker_t *) (iopb->set_list_tail);
        ZCC_MLINK_DETACH(h, t, linker, prev, next);
        iopb->set_list_head = h;
        iopb->set_list_tail = t;
        ZIOPIPE_BASE_UNLOCK(iopb);

        int cfd, sfd;
        SSL *cssl, *sssl;
        iopipe_after_close_fn_t after_close;
        void *context;
        cfd = linker->cfd;
        sfd = linker->sfd;
        cssl = linker->cssl;
        sssl = linker->sssl;
        after_close = linker->after_close;
        context = linker->context;
        iop = (iopipe_t *) linker;
        memset(iop, 0, sizeof(iopipe_t));
        iop->client.fd = cfd;
        iop->client.ssl = cssl;
        iop->client.is_client_or_server = 0;
        iop->server.fd = sfd;
        iop->server.ssl = sssl;
        iop->server.is_client_or_server = 1;
        if (cssl) {
            iop->client.read_want_read = 1;
        }
        if (sssl) {
            iop->server.read_want_read = 1;
        }
        iop->after_close = after_close;
        iop->context = context;
        iopipe_set_event(iopb, &(iop->client), 1);
        iopipe_set_event(iopb, &(iop->server), 1);
    }

    nfds = epoll_wait(iopb->epoll_fd, iopb->epoll_event_list, iopb->epoll_event_count, 10 * 1000);
    if (nfds == -1) {
        if (errno != EINTR) {
            log_fatal("iopipe_base_run: epoll_wait: %m");
        }
        return -1;
    }

    for (i = 0; i < nfds; i++) {
        ev = iopb->epoll_event_list + i;
        events = ev->events;
        part = (iopipe_part_t *) (ev->data.ptr);
        is_client_or_server = part->is_client_or_server;
        if (!is_client_or_server) {
            iop = ZCC_CONTAINER_OF(part, iopipe_t, client);
        } else {
            iop = ZCC_CONTAINER_OF(part, iopipe_t, server);
        }

        part_client = &(iop->client);
        part_server = &(iop->server);
        part = part_server;
        part_a = part_client;
        if (!is_client_or_server) {
            part = part_client;
            part_a = part_server;
        }

        if (part_client->fd == efd) {
            if (!(events & EPOLLOUT)) {
                uint64_t u;
                if (read(efd, &u, sizeof(uint64_t))) ;
            }
            continue;
        }

        e_err = 0;
        if (events & EPOLLHUP) {
            e_err = 1;
        }
        if (events & EPOLLRDHUP) {
            e_err = 1;
        }
        if (events & EPOLLERR) {
            e_err = 1;
        }
        if (part->ssl && part->ssl_error) {
            e_err = 1;
        }
        if (events & EPOLLOUT) {
            part->et_write = 1;
        }
        if (events & EPOLLIN) {
            part->et_read = 1;
        }
#define _debug_part(p)	printf("client_or_server: %ld, %d, %d, %d, %d\n", p->is_client_or_server, p->read_want_read, p->read_want_write, p->write_want_read, p->write_want_write)

#define _ssl_w(p) (((p->write_want_write) &&(p->et_write))||((p->write_want_read) &&(p->et_read)))
#define _ssl_r(p) (((p->read_want_write) &&(p->et_write))||((p->read_want_read) &&(p->et_read)))
#define _www(p) ((p->ssl) && (_ssl_w(p))) || ((!(p->ssl)) && (p->et_write))
#define _rrr(p) ((p->ssl) && (_ssl_r(p))) || ((!(p->ssl)) && (p->et_read))
#if 0
        int _rp = _rrr(part), _rpa=_rrr(part_a);
        int _wp = _www(part), _wpa=_www(part_a);
        if (_rp || (part->rbuf_p2 - part->rbuf_p1 > 0) ) {
            printf("BBB\n");
            int len = part->rbuf_p2 - part->rbuf_p1;
            if (_wpa) {
                read_write_loop(iopb, part, part_a, len);
            }
            printf("BBB over\n");
        }
        if (_wp) {
            printf("AAA\n");
            int len = part_a->rbuf_p2 - part_a->rbuf_p1;
            if ((len > 0) || _rpa) {
                write_read_loop(iopb, part, part_a, len);
            }
            printf("AAA over\n");
        }
#else
            read_write_loop(iopb, part, part_a, part->rbuf_p2 - part->rbuf_p1);

            write_read_loop(iopb, part, part_a, part_a->rbuf_p2 - part_a->rbuf_p1);
#endif
        if (e_err) {
            int len;
            len = part->rbuf_p2 > part->rbuf_p1;
            if (len > 0) {
                if (part_a->ssl) {
                    try_ssl_write(part_a, part->rbuf + part->rbuf_p1, len);
                } else {
                    if (write(part_a->fd, part->rbuf + part->rbuf_p1, len));
                }
            }
            iopipe_set_event(iopb, part_client, 0);
            iopipe_set_event(iopb, part_server, 0);
            if (part_client->rbuf) {
                iopb->rbuf_mpool->release(part_client->rbuf);
            }
            if (part_server->rbuf) {
                iopb->rbuf_mpool->release(part_server->rbuf);
            }
            if (part->ssl) {
                openssl_SSL_free(part->ssl);
            }
            if (part_a->ssl) {
                openssl_SSL_free(part_a->ssl);
            }
            close(part_client->fd);
            close(part_server->fd);
            if (iop->after_close) {
                iop->after_close(iop->context);
            }
            iopb->iop_mpool->release(iop);
            continue;
        }
    }

    if (iopb->break_flag) {
        return 0;
    }

    ___no_indent_while_end;

    return 0;
}

void iopipe_base_free(iopipe_base_t * iopb)
{
    close(iopb->epoll_fd);
    free(iopb->epoll_event_list);
    delete iopb->rbuf_mpool;
    delete iopb->iop_mpool;
    close(iopb->eventfd_iop.client.fd);

    iopipe_linker_t *hn, *h;
    for (h = iopb->set_list_head;h;h=hn) {
        hn = h->next;
        openssl_SSL_free(h->cssl);
        close(h->cfd);
        openssl_SSL_free(h->sssl);
        close(h->sfd);
    }
    pthread_mutex_destroy(&(iopb->locker));
    free(iopb);
}

void iopipe_enter(iopipe_base_t * iopb, int client_fd, SSL *client_ssl, int server_fd, SSL *server_ssl, iopipe_after_close_fn_t after_close, const void *context)
{
    uint64_t u;
    iopipe_linker_t *linker, *h, *t;

    nonblocking(client_fd);
    nonblocking(server_fd, 1);

    linker = (iopipe_linker_t *) (iopb->iop_mpool->require());

    memset(linker, 0, sizeof(iopipe_linker_t));
    linker->cfd = client_fd;
    linker->sfd = server_fd;
    linker->cssl = client_ssl;
    linker->sssl = server_ssl;
    linker->after_close = after_close;
    linker->context = (void *)context;

    ZIOPIPE_BASE_LOCK(iopb);
    h = (iopipe_linker_t *) (iopb->set_list_head);
    t = (iopipe_linker_t *) (iopb->set_list_tail);
    ZCC_MLINK_APPEND(h, t, linker, prev, next);
    iopb->set_list_head = h;
    iopb->set_list_tail = t;
    ZIOPIPE_BASE_UNLOCK(iopb);

    if (write(iopb->eventfd_iop.client.fd, &u, sizeof(uint64_t))) ;
}

/* ############################################################## */
iopipe::iopipe()
{
    ___data = iopipe_base_create();
}

iopipe::~iopipe()
{
    iopipe_base_free((iopipe_base_t *)___data);
}

void iopipe::run()
{
    iopipe_base_run((iopipe_base_t *)___data);
}

void iopipe::stop_notify()
{
    ((iopipe_base_t *)___data)->break_flag = 1;
}

void iopipe::enter(int client_fd, SSL *client_ssl, int server_fd, SSL *server_ssl)
{
    iopipe_enter((iopipe_base_t *)___data, client_fd, client_ssl, server_fd, server_ssl, 0, 0);
}

void iopipe::enter(int client_fd, SSL *client_ssl, int server_fd, SSL *server_ssl, iopipe_after_close_fn_t after_close, void *context)
{
    iopipe_enter((iopipe_base_t *)___data, client_fd, client_ssl, server_fd, server_ssl, after_close, context);
}

}
