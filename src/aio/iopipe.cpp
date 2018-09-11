/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
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

#pragma pack(push, 1)

/* {{{ declare, macro */
#define var_epoll_event_count  4096
#define var_iopipe_rbuf_size 4096
#define var_iopipe_retry_size 10
#define ZIOPIPE_BASE_LOCK(iopb)		if (pthread_mutex_lock(&(iopb->locker))) { zcc_fatal("mutex:%m"); }
#define ZIOPIPE_BASE_UNLOCK(iopb)	if (pthread_mutex_unlock(&(iopb->locker))) { zcc_fatal("mutex:%m"); }

typedef struct iopipe_t iopipe_t;
typedef struct iopipe_part_t iopipe_part_t;
typedef struct iopipe_linker_t iopipe_linker_t;
typedef struct iopipe_retry_t iopipe_retry_t;

struct iopipe_part_t {
    unsigned char is_client_or_server:1;
    unsigned char error_or_closed:1;
    unsigned char event_in:1;
    int rbuf_p1:16;
    int rbuf_p2:16;
    int fd;
    char rbuf[var_iopipe_rbuf_size+1];
    SSL *ssl;
};

struct iopipe_t {
    iopipe_part_t client;
    iopipe_part_t server;
    iopipe_after_close_fn_t after_close;
    void *context;
    long timeout;
    iopipe_t *prev;
    iopipe_t *next;
    short int retry_idx;
    unsigned char error_queue_in:1;
};

struct iopipe_retry_t {
    iopipe_t *head;
    iopipe_t *tail;
    int count;
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
    struct epoll_event epoll_event_list[var_epoll_event_count];
    iopipe_t *iopipe_error_vector[var_epoll_event_count];
    pthread_mutex_t locker;
    iopipe_linker_t *enter_list_head;
    iopipe_linker_t *enter_list_tail;
    iopipe_retry_t retry_vector[var_iopipe_retry_size];
    long retry_stamp;
    short int retry_idx;
    iopipe_t eventfd_iop;
    long after_peer_closed_timeout;
    int count;
};

/* }}} */

/* {{{ event set/unset */
static void iopipe_set_event(iopipe_base_t * iopb, iopipe_part_t * part)
{
    if (part->event_in) {
        return;
    }
    struct epoll_event epev;
    epev.events = EPOLLET | EPOLLIN | EPOLLOUT;
    epev.data.ptr = part;
    if (epoll_ctl(iopb->epoll_fd, EPOLL_CTL_ADD, part->fd, &epev) == -1) {
        zcc_fatal("fd %d: ADD error: %m", part->fd);
    }
    part->event_in = 1;
}

static void iopipe_unset_event(iopipe_base_t * iopb, iopipe_part_t * part)
{
    if (!(part->event_in)) {
        return;
    }
    if (epoll_ctl(iopb->epoll_fd, EPOLL_CTL_DEL, part->fd, NULL) == -1) {
        zcc_fatal("fd %d: DEL error: %m", part->fd);
    }
    part->event_in = 0;
}
/* }}} */

/* {{{ iopipe_try_release */
static inline void iopipe_part_release(iopipe_base_t *iopb, iopipe_part_t *part)
{
    if (part->error_or_closed && (part->fd !=-1 )) {
        iopipe_unset_event(iopb, part);
        if (part->ssl) {
            openssl_SSL_free(part->ssl);
            part->ssl = 0;
        }
        close(part->fd);
        part->fd = -1;
    }
}

static void iopipe_try_release(iopipe_base_t *iopb, iopipe_t *iop)
{
    iopipe_part_t *part_client = &(iop->client);
    iopipe_part_t *part_server = &(iop->server);
    bool need_free = false;
    if ((part_client->error_or_closed==1) && (part_server->error_or_closed==1)){
        need_free = true;
    }
    if ((part_client->error_or_closed==1) && (part_client->rbuf_p2 == part_client->rbuf_p1)) {
        need_free = true;
    }
    if ((part_server->error_or_closed==1) && (part_server->rbuf_p2 == part_server->rbuf_p1)) {
        need_free = true;
    }
    if ((!need_free) && iop->timeout && (timeout_set(0) > iop->timeout)) {
        need_free = true;
    }
    if (need_free) {
        part_client->error_or_closed = 1;
        part_server->error_or_closed = 1;
    }
    iopipe_part_release(iopb, part_client);
    iopipe_part_release(iopb, part_server);

    if (!need_free) {
        return;
    }
    
    zcc_mlink_detach(iopb->retry_vector[iop->retry_idx].head,iopb->retry_vector[iop->retry_idx].tail,iop,prev,next);
    iopb->retry_vector[iop->retry_idx].count --;
    iopb->count --;
    if (iop->after_close) {
        iop->after_close(iop->context);
    }
    free(iop);
}
/* }}} */

/* {{{ read/write loop */
static inline int iopipe_ssl_write(iopipe_part_t * part, void *buf, int len)
{
    int wlen = -1, status;
    if (part->error_or_closed) {
        return -1;
    }
    while(1) {
        wlen = SSL_write(part->ssl, buf, len);
        if (wlen > 0) {
            break;
        }
        status = SSL_get_error(part->ssl, wlen);
        if (status == SSL_ERROR_WANT_WRITE) {
            wlen = -1;
        } else if (status == SSL_ERROR_WANT_READ) {
            wlen = -1;
        } else {
            part->error_or_closed = 1;
        }
        break;
    }
    return wlen;
}

static inline int iopipe_ssl_read(iopipe_part_t * part, void *buf, int len)
{
    int rlen = -1, status;
    if (part->error_or_closed) {
        return -1;
    }
    while(1) {
        rlen = SSL_read(part->ssl, buf, len);
        if (rlen > 0) {
            break;
        }
        status = SSL_get_error(part->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE) {
            rlen = -1;
        } else if (status == SSL_ERROR_WANT_READ) {
            rlen = -1;
        } else {
            part->error_or_closed = 1;
            break;
        }
        break;
    }
    return rlen;
}

static inline ssize_t iopipe_sys_write(iopipe_part_t * part, const void *buf, size_t count)
{
    int wlen = -1, eo;
    if (part->error_or_closed) {
        return -1;
    }
    while(1) {
        errno = 0;
        wlen = write(part->fd, buf, count);
        if (wlen > 0) {
            break;
        }
        if (wlen == 0) {
            wlen = -1;
            break;
        }
        eo = errno;
        if (eo == EINTR) {
            continue;
        }
        if (eo == EAGAIN) {
            break;
        }
        part->error_or_closed = 1;
        break;
    }

    return wlen;
}

static inline ssize_t iopipe_sys_read(iopipe_part_t * part, void *buf, size_t count)
{
    int rlen = -1, eo;
    if (part->error_or_closed) {
        return -1;
    }
    while(1) {
        errno = 0;
        rlen = read(part->fd, buf, count);
        if (rlen == 0) {
            part->error_or_closed = 1;
        }
        if (rlen >= 0) {
            break;
        }
        eo = errno;
        if (eo == EINTR) {
            continue;
        }
        if (eo == EAGAIN) {
            break;
        }
        part->error_or_closed = 1;
        break;
    }
    return rlen;
}

static void iopipe_read_write_loop(iopipe_base_t *iopb, iopipe_part_t *part, iopipe_part_t *part_a)
{
    int rbuf_len, wrote_len;
    while(1) {
        rbuf_len = part->rbuf_p2 - part->rbuf_p1;
        if (rbuf_len == 0) {
            part->rbuf_p1 = part->rbuf_p2 = 0;
            if (part->ssl) {
                rbuf_len = iopipe_ssl_read(part, part->rbuf, var_iopipe_rbuf_size);
            } else {
                rbuf_len = (int)iopipe_sys_read(part, part->rbuf, var_iopipe_rbuf_size);
            }

            if (rbuf_len > 0) {
                part->rbuf_p2 = rbuf_len;
                continue;
            } else {
                break;
            }
        } else {
            if (part_a->ssl) {
                wrote_len = iopipe_ssl_write(part_a, part->rbuf + part->rbuf_p1, rbuf_len);
            } else {
                wrote_len = (int)iopipe_sys_write(part_a, part->rbuf + part->rbuf_p1, (size_t)rbuf_len);
            }
            if (wrote_len > 0) {
                if (rbuf_len == wrote_len) {
                    part->rbuf_p1 += wrote_len;
                }
                continue;
            } else {
                break;
            }
        }
    }
}
/* }}} */

/* {{{ iopipe_enter_list_checker */
static inline void iopipe_enter_list_checker(iopipe_base_t *iopb)
{
    while (iopb->enter_list_head) {
        ZIOPIPE_BASE_LOCK(iopb);
        iopipe_linker_t *linker = iopb->enter_list_head;
        if (!linker) {
            ZIOPIPE_BASE_UNLOCK(iopb);
            break;
        }
        zcc_mlink_detach(iopb->enter_list_head, iopb->enter_list_tail, linker, prev, next);
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
        iopipe_t *iop = (iopipe_t *) linker;
        memset(iop, 0, sizeof(iopipe_t));

        iop->client.fd = cfd;
        iop->client.ssl = cssl;
        iop->client.is_client_or_server = 0;

        iop->server.fd = sfd;
        iop->server.ssl = sssl;
        iop->server.is_client_or_server = 1;

        iop->after_close = after_close;
        iop->context = context;

        iopipe_set_event(iopb, &(iop->client));
        iopipe_set_event(iopb, &(iop->server));

        do {
            int min_count = 1024 * 1024 * 100;
            int idx = 0;
            iopipe_retry_t *rt = iopb->retry_vector + 0;
            for (int i = 0; i < var_iopipe_retry_size; i++) {
                if (iopb->retry_vector[i].count < min_count) {
                    rt = iopb->retry_vector + i;
                    min_count = rt->count;
                    idx = i;
                }
            }
            iop->retry_idx = idx;
            zcc_mlink_append(rt->head, rt->tail, iop, prev, next);
            rt->count ++;
            iopb->count ++;
        } while(0);
    }
}
/* }}} */

/* {{{ retry and timeout */
static inline void iopipe_retry_vector(iopipe_base_t *iopb)
{
    if (timeout_set(0) - iopb->retry_stamp < 100) {
        return;
    }
    int idx = iopb->retry_idx;
    iopb->retry_idx++;
    if (iopb->retry_idx == var_iopipe_retry_size) {
        iopb->retry_idx = 0;
    }
    iopipe_t *iop, *iop_next;
    for (iop = iopb->retry_vector[idx].head; iop; iop = iop_next) {
        iop_next = iop->next;
        iopipe_part_t *part_client = &(iop->client);
        iopipe_part_t *part_server = &(iop->server);
        if (part_server->error_or_closed==0) {
            iopipe_read_write_loop(iopb, part_client, part_server);
        }
        if (part_client->error_or_closed==0) {
            iopipe_read_write_loop(iopb, part_server, part_client);
        }
        if ((part_client->error_or_closed) || (part_server->error_or_closed)) {
            if (iop->timeout == 0) {
                iop->timeout = timeout_set(0) + iopb->after_peer_closed_timeout;
            }
        }
        iopipe_try_release(iopb, iop);
    }
    iopb->retry_stamp = timeout_set(0);
}
/* }}} */

/* {{{ iopipe_base_create */
static iopipe_base_t *iopipe_base_create(void)
{
    iopipe_base_t *iopb;
    int efd;

    iopb = (iopipe_base_t *) calloc(1, sizeof(iopipe_base_t));

    pthread_mutex_init(&(iopb->locker), 0);

    iopb->epoll_fd = epoll_create(1024);
    close_on_exec(iopb->epoll_fd);

    efd = eventfd(0, 0);
    nonblocking(efd);
    close_on_exec(efd);

    iopb->eventfd_iop.client.fd = efd;
    iopb->eventfd_iop.client.is_client_or_server = 0;
    iopipe_set_event(iopb, &(iopb->eventfd_iop.client));

    iopb->after_peer_closed_timeout = 10 * 1000;

    return iopb;
}

static void iopipe_base_free(iopipe_base_t * iopb)
{
    close(iopb->epoll_fd);
    close(iopb->eventfd_iop.client.fd);

    iopipe_linker_t *hn, *h;
    for (h = iopb->enter_list_head;h;h=hn) {
        hn = h->next;
        openssl_SSL_free(h->cssl);
        close(h->cfd);
        openssl_SSL_free(h->sssl);
        close(h->sfd);
    }

    for (int idx = 0; idx < var_iopipe_retry_size; idx ++) {
        iopipe_t *iop, *iop_next;
        for (iop = iopb->retry_vector[idx].head; iop; iop = iop_next) {
            iop_next = iop->next;
            iop->client.error_or_closed = 0;
            iop->server.error_or_closed = 0;
            iopipe_try_release(iopb, iop);
        }
    }

    pthread_mutex_destroy(&(iopb->locker));
    free(iopb);
}
/* }}} */

/* {{{ iopipe_base_run */
static int iopipe_base_run(iopipe_base_t * iopb)
{
#define  ___no_indent_while_beign   while(1) {
#define  ___no_indent_while_end     }

    int efd = iopb->eventfd_iop.client.fd;

    ___no_indent_while_beign;

    iopipe_enter_list_checker(iopb);

    iopipe_retry_vector(iopb);

    int nfds = epoll_wait(iopb->epoll_fd, iopb->epoll_event_list, var_epoll_event_count, 100);
    if (nfds == -1) {
        if (errno != EINTR) {
            zcc_fatal("iopipe_base_run: epoll_wait: %m");
        }
        continue;
    }

    int iopipe_error_count = 0;
    for (int i = 0; i < nfds; i++) {
        iopipe_t *iop;
        iopipe_part_t *part_client, *part_server;
        struct epoll_event *epev = iopb->epoll_event_list + i;
        unsigned int events = epev->events;
        iopipe_part_t *part = (iopipe_part_t *) (epev->data.ptr);
        int is_client_or_server = part->is_client_or_server;
        if (!is_client_or_server) {
            iop = zcc_container_of(part, iopipe_t, client);
        } else {
            iop = zcc_container_of(part, iopipe_t, server);
        }

        part_client = &(iop->client);
        part_server = &(iop->server);

        if (part_client->fd == efd) {
            if (!(events & EPOLLOUT)) {
                uint64_t u;
                if (read(efd, &u, sizeof(uint64_t))) {
                }
            }
            continue;
        }

        if (part_server->error_or_closed==0) {
            iopipe_read_write_loop(iopb, part_client, part_server);
        }
        if (part_client->error_or_closed==0) {
            iopipe_read_write_loop(iopb, part_server, part_client);
        }

        if (part_client->error_or_closed || part_server->error_or_closed) {
            if (iop->error_queue_in == 0) {
                iop->error_queue_in = 1;
                iopb->iopipe_error_vector[iopipe_error_count++] = iop;
            }
        }
    }

    for (int i=0;i<iopipe_error_count;i++) {
        iopipe_t *iop = iopb->iopipe_error_vector[i];
        if (iop->timeout == 0) {
            iop->timeout = timeout_set(0) + iopb->after_peer_closed_timeout;
        }
        iop->error_queue_in = 0;
        iopipe_try_release(iopb, iop);
    }

    if (iopb->break_flag) {
        return 0;
    }

    ___no_indent_while_end;

    return 0;
}
/* }}} */

/* {{{ enter */
static void iopipe_enter(iopipe_base_t * iopb, int client_fd, SSL *client_ssl, int server_fd, SSL *server_ssl, iopipe_after_close_fn_t after_close, const void *context)
{
    iopipe_linker_t *linker, *h, *t;

    nonblocking(client_fd);
    nonblocking(server_fd, 1);

    linker = (iopipe_linker_t *) malloc(sizeof(iopipe_t));

    memset(linker, 0, sizeof(iopipe_linker_t));
    linker->cfd = client_fd;
    linker->sfd = server_fd;
    linker->cssl = client_ssl;
    linker->sssl = server_ssl;
    linker->after_close = after_close;
    linker->context = (void *)context;

    ZIOPIPE_BASE_LOCK(iopb);
    h = (iopipe_linker_t *) (iopb->enter_list_head);
    t = (iopipe_linker_t *) (iopb->enter_list_tail);
    zcc_mlink_append(h, t, linker, prev, next);
    iopb->enter_list_head = h;
    iopb->enter_list_tail = t;
    ZIOPIPE_BASE_UNLOCK(iopb);

    uint64_t u = 1;
    if (write(iopb->eventfd_iop.client.fd, &u, sizeof(uint64_t))) {
    }
}

/* }}} */

/* {{{ class method */
iopipe::iopipe()
{
    ___data = iopipe_base_create();
}

iopipe::~iopipe()
{
    iopipe_base_free((iopipe_base_t *)___data);
}

void iopipe::set_after_peer_closed_timeout(long timeout)
{
    if (timeout > 0){
        ((iopipe_base_t *)___data)->after_peer_closed_timeout = timeout;
    }
}

void iopipe::run()
{
    iopipe_base_run((iopipe_base_t *)___data);
}

void iopipe::stop_notify()
{
    ((iopipe_base_t *)___data)->break_flag = 1;
}

size_t iopipe::get_count()
{
    return ((iopipe_base_t *)___data)->count;
}

void iopipe::enter(int client_fd, SSL *client_ssl, int server_fd, SSL *server_ssl)
{
    iopipe_enter((iopipe_base_t *)___data, client_fd, client_ssl, server_fd, server_ssl, 0, 0);
}

void iopipe::enter(int client_fd, SSL *client_ssl, int server_fd, SSL *server_ssl, iopipe_after_close_fn_t after_close, void *context)
{
    iopipe_enter((iopipe_base_t *)___data, client_fd, client_ssl, server_fd, server_ssl, after_close, context);
}
/* }}} */

#pragma pack(pop)
}

/* Local variables:
* End:
* vim600: fdm=marker
*/
