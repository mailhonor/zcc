/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-01-22
 * ================================
 */

#include "zcc.h"
#include <pthread.h>

static ssize_t (*___Z_SYS_read)(int fd, void *buf, size_t count) = read;
static ssize_t (*___Z_SYS_write)(int fd, const void *buf, size_t count) = write;

namespace zcc
{

event_base default_evbase;

#define aio_get_base(aio)          ((aio)->evbase)
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/un.h>

#define ZAIO_CB_MAGIC                       0XF0U
#define ZAIO_CB_TYPE_NONE                   0X00U
#define ZAIO_CB_READ                        0X10U
#define ZAIO_CB_TYPE_READ                   0X11U
#define ZAIO_CB_TYPE_READ_N                 0X12U
#define ZAIO_CB_TYPE_READ_DELIMETER         0X13U
#define ZAIO_CB_TYPE_READ_SIZE_DATA         0X14U
#define ZAIO_CB_WRITE                       0X20U
#define ZAIO_CB_TYPE_WRITE                  0X21U
#define ZAIO_CB_TYPE_SLEEP                  0X31U
#define ZAIO_CB_TYPE_SSL_INIT               0X41U

#define lock_evbase(eb)   { \
    if((eb)->plock){if(pthread_mutex_lock((pthread_mutex_t *)((eb)->plock))) { log_fatal("mutex:%m"); }}}
#define unlock_evbase(eb)   { \
    if((eb)->plock){if(pthread_mutex_unlock((pthread_mutex_t *)((eb)->plock))) { log_fatal("mutex:%m"); }}}

/* ################################################################## */
/* ev/event/trigger */

/* ################################################################## */
/* aio */

#define epoll_event_count  4096
static int aio_event_set(aio_t * aio, int ev_type, long timeout);
static int ZAIO_P2_MAX = aio_rwbuf_size - 1;
static void aio_ready_do(aio_t * aio);

static int aio_timer_cmp(rbtree_node_t * n1, rbtree_node_t * n2)
{
    aio_t *e1, *e2;
    int r;

    e1 = ZCC_CONTAINER_OF(n1, aio_t, rbnode_time);
    e2 = ZCC_CONTAINER_OF(n2, aio_t, rbnode_time);
    r = e1->timeout - e2->timeout;
    if (!r) {
        r = e1->fd - e2->fd;
    }
    if (r > 0) {
        return 1;
    } else if (r < 0) {
        return -1;
    }
    return 0;
}

static inline int aio_timer_check(evbase_t * eb)
{
    aio_t *aio;
    async_io_cb_t callback;
    rbtree_node_t *rn;
    long delay = 10 * 1000;

    if (!rbtree_have_data(&(eb->aio_timer_tree))) {
        return delay;
    }

    while (1) {
        lock_evbase(eb);
        rn = rbtree_first(&(eb->aio_timer_tree));
        if (!rn) {
            unlock_evbase(eb);
            return 10 * 1000;
        }
        aio = ZCC_CONTAINER_OF(rn, aio_t, rbnode_time);
        delay = timeout_left(aio->timeout);
        if (delay > 0) {
            unlock_evbase(eb);
            return delay;
        }
        callback = aio->callback;
        aio->recv_events = 0;
        aio->ret = -2;
        if (aio->rw_type == ZAIO_CB_TYPE_SLEEP) {
            aio->ret = 1;
        }
        if (aio->in_time) {
            rbtree_detach(&(eb->aio_timer_tree), rn);
        }
        aio->in_time = 0;
        unlock_evbase(eb);

        if (callback) {
            (callback) (*ZCC_CONTAINER_OF(aio, async_io, ___data));
        } else {
            log_fatal("aio: not found callback");
        }
    }

    return 10 * 1000;
}

static inline void ___aio_cache_shift(aio_t * aio, aio_rwbuf_list_t * ioc, void *data, int len)
{
    int rlen, i, olen = len;
    char *buf = (char *)data;
    char *cdata;
    aio_rwbuf_t *rwb;
    evbase_t *eb = aio_get_base(aio);

    rwb = ioc->head;
    if (rwb && rwb->long_flag) {
        /* only write */
        aio_rwbuf_longbuf_t *lb = (aio_rwbuf_longbuf_t *)(rwb->data);
        lb->p1 += len;
        if (lb->p1 == lb->p2) {
            ioc->head = rwb->next;
            lock_evbase(eb);
            eb->aio_rwbuf_mpool->release(rwb);
            unlock_evbase(eb);
            if (ioc->head == 0) {
                ioc->tail = 0;
            }
        }
        ioc->len -= olen;
        return;
    }

    while (len > 0) {
        rwb = ioc->head;
        rlen = rwb->p2 - rwb->p1 + 1;
        if (data) {
            if (len >= rlen) {
                i = rlen;
            } else {
                i = len;
            }
            cdata = rwb->data + rwb->p1;
            while (i--) {
                *buf++ = *cdata++;
            }
        }
        if (len >= rlen) {
            rwb = rwb->next;
            lock_evbase(eb);
            eb->aio_rwbuf_mpool->release(ioc->head);
            unlock_evbase(eb);
            ioc->head = rwb;
            len -= rlen;
        } else {
            rwb->p1 += len;
            len = 0;
        }
    }
    if (ioc->head == 0) {
        ioc->tail = 0;
    }
    ioc->len -= olen;
}

static void ___aio_cache_first_line(aio_t * aio, aio_rwbuf_list_t * ioc, char **data, int *len)
{
    aio_rwbuf_t *rwb;

    rwb = ioc->head;
    if (!rwb) {
        *data = 0;
        *len = 0;
        return;
    }
    if (rwb->long_flag) {
        aio_rwbuf_longbuf_t *lb = (aio_rwbuf_longbuf_t *)(rwb->data);
        *data = lb->data + lb->p1;
        *len = lb->p2 - lb->p1 + 1;
    } else {
        *data = (rwb->data + rwb->p1);
        *len = (rwb->p2 - rwb->p1 + 1);
    }
}

static inline void ___aio_cache_append(aio_t * aio, aio_rwbuf_list_t * ioc, const void *data, int len)
{
    char *buf = (char *)data;
    char *cdata;
    int i, p2;
    aio_rwbuf_t *rwb;
    evbase_t *eb = aio_get_base(aio);

    rwb = ioc->tail;
    p2 = 0;
    cdata = 0;
    if (rwb) {
        p2 = rwb->p2;
        cdata = rwb->data;
    }
    for (i = 0; i < len; i++) {
        if (!rwb || (p2 == ZAIO_P2_MAX)) {
            lock_evbase(eb);
            rwb = (aio_rwbuf_t *) eb->aio_rwbuf_mpool->require();
            unlock_evbase(eb);
            rwb->next = 0;
            rwb->p1 = 0;
            rwb->p2 = 0;
            if (ioc->tail) {
                ioc->tail->next = rwb;
                ioc->tail->p2 = p2;
            } else {
                ioc->head = rwb;
            }
            ioc->tail = rwb;
            cdata = rwb->data;
            p2 = rwb->p2;
        } else {
            p2++;
        }
        cdata[p2] = buf[i];
    }
    rwb->p2 = p2;
    ioc->len += len;
}

static inline int aio_try_ssl_read(aio_t * aio, void *buf, int len)
{
    int rlen, status;

    aio->ssl_read_want_write = 0;
    aio->ssl_read_want_read = 0;

    rlen = SSL_read(aio->ssl, buf, len);
    if (rlen < 1) {
        status = SSL_get_error(aio->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE) {
            aio->ssl_read_want_write = 1;
        } else if (status == SSL_ERROR_WANT_READ) {
            aio->ssl_read_want_read = 1;
        } else {
            aio->ssl_error = 1;
        }
        return -1;
    }
    return rlen;
}

static inline int aio_try_ssl_write(aio_t * aio, void *buf, int len)
{
    int rlen, status;

    aio->ssl_write_want_write = 0;
    aio->ssl_write_want_read = 0;

    rlen = SSL_write(aio->ssl, buf, len);
    if (rlen < 1) {
        status = SSL_get_error(aio->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE) {
            aio->ssl_write_want_write = 1;
        } else if (status == SSL_ERROR_WANT_READ) {
            aio->ssl_write_want_read = 1;
        } else {
            aio->ssl_error = 1;
        }
        return -1;
    }

    return rlen;
}

static int aio_try_ssl_connect(aio_t * aio)
{
    int rlen, status;

    aio->ssl_write_want_write = 0;
    aio->ssl_write_want_read = 0;

    rlen = SSL_connect(aio->ssl);
    if (rlen < 1) {
        status = SSL_get_error(aio->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE) {
            aio->ssl_write_want_write = 1;
        } else if (status == SSL_ERROR_WANT_READ) {
            aio->ssl_write_want_read = 1;
        } else {
            aio->ssl_error = 1;
        }
        return -1;
    }

    aio->ssl_session_init = 1;
    aio->ret = 1;
    return 1;
}

static int aio_try_ssl_accept(aio_t * aio)
{
    int rlen, status;

    aio->ssl_read_want_write = 0;
    aio->ssl_read_want_read = 0;

    rlen = SSL_accept(aio->ssl);
    if (rlen < 1) {
        status = SSL_get_error(aio->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE) {
            aio->ssl_read_want_write = 1;
        } else if (status == SSL_ERROR_WANT_READ) {
            aio->ssl_read_want_read = 1;
        } else {
            aio->ssl_error = 1;
        }
        return -1;
    }

    aio->ssl_session_init = 1;
    aio->ret = 1;
    return 1;
}

static inline int aio_ssl_init___inner(aio_t * aio, async_io_cb_t callback, long timeout)
{
    int rlen;

    aio->rw_type = ZAIO_CB_TYPE_SSL_INIT;
    aio->callback = callback;

    if (aio->ssl_server_or_client) {
        rlen = aio_try_ssl_accept(aio);
    } else {
        rlen = aio_try_ssl_connect(aio);
    }

    if (rlen > 0) {
        aio_event_set(aio, 2, timeout);
        aio_ready_do(aio);
        return 0;
    }

    aio_event_set(aio, 1, timeout);

    return 0;
}

static void aio_ready_do(aio_t * aio)
{
    async_io *asio = ZCC_CONTAINER_OF(aio, async_io, ___data);
    async_io_cb_t callback;
    int rw_type;

    callback = aio->callback;
    rw_type = aio->rw_type;

    if (!callback) {
        log_fatal("aio: not found callback");
    }

    if (rw_type == ZAIO_CB_TYPE_SLEEP) {
        aio->ret = 1;
    }
    if (aio->is_local == 0) {
        aio_event_set(aio, 0, -2);
    }

    aio->rw_type = ZAIO_CB_TYPE_NONE;
    (callback) (*asio);
}

static int aio_event_set(aio_t * aio, int ev_type, long timeout)
{
    evbase_t *eb = aio_get_base(aio);
    int fd, events, old_events, e_events;
    struct epoll_event evt;
    rbtree_t *timer_tree = &(eb->aio_timer_tree);
    rbtree_node_t *rn;
    int rw_type;

    /* timeout */
    rn = &(aio->rbnode_time);
    if (timeout > 0) {
        if (aio->in_time) {
            rbtree_detach(timer_tree, rn);
        }
        aio->timeout = timeout_set(timeout);
        rbtree_attach(timer_tree, rn);
        aio->in_time = 1;
        aio->enable_time = 1;
    } else if (timeout == 0) {
        if (aio->in_time) {
            rbtree_detach(timer_tree, rn);
        }
        aio->in_time = 0;
        aio->enable_time = 0;
    } else if (timeout == -1) {
        if (aio->enable_time) {
            if (aio->in_time == 0) {
                rbtree_attach(timer_tree, rn);
            }
            aio->in_time = 1;
        }
    } else {                    /* if (timeout == -2) */
        /* inner use */
        if (aio->in_time) {
            rbtree_detach(timer_tree, rn);
        }
        aio->in_time = 0;
    }

    /* event */
    if (ev_type != 2) {
        fd = aio->fd;
        rw_type = aio->rw_type;
        events = 0;
        if (ev_type == 1) {
            /* compute the events */
            if (!(aio->ssl)) {
                if ((rw_type & ZAIO_CB_MAGIC) == ZAIO_CB_READ) {
                    events |= var_event_read;
                }
                if ((rw_type & ZAIO_CB_MAGIC) == ZAIO_CB_WRITE) {
                    events |= var_event_write;
                }
                if (aio->write_cache.len > 0) {
                    events |= var_event_write;
                }
            } else {
                if (aio->ssl_read_want_read || aio->ssl_write_want_read) {
                    events |= var_event_read;
                }
                if (aio->ssl_read_want_write || aio->ssl_write_want_write) {
                    events |= var_event_write;
                }
                if (aio->write_cache.len > 0) {
                    if ((aio->ssl_write_want_read == 0) && (aio->ssl_write_want_write == 0)) {
                        aio->ssl_write_want_write = 1;
                        events |= var_event_write;
                    }
                }
            }
        }
        e_events = EPOLLHUP | EPOLLRDHUP | EPOLLERR;
        old_events = aio->events;
        aio->events = events;

        if (events == 0) {
            if (old_events) {
                if (epoll_ctl(eb->epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
                    log_fatal("aio_event_set: fd %d: DEL  ssl_error: %m", fd);
                }
            }
        } else if (old_events != events) {
            if (events & var_event_read) {
                e_events |= EPOLLIN;
            }
            if (events & var_event_write) {
                e_events |= EPOLLOUT;
            }
            if (events & var_event_persist) {
                e_events |= EPOLLET;
            }
            evt.events = e_events;
            evt.data.ptr = aio;
            if (epoll_ctl(eb->epoll_fd, (old_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD), fd, &evt) == -1) {
                log_fatal("aio_event_set: fd %d: %s ssl_error: %m", fd, (old_events ? "MOD" : "ADD"));
            }
        }
    }

    return 0;
}

static inline int aio_read___innner(aio_t * aio, int max_len, async_io_cb_t callback, long timeout)
{
    int magic, rlen;
    char buf[10240];

    aio->rw_type = ZAIO_CB_TYPE_READ;
    aio->callback = callback;
    aio->read_magic_len = max_len;

    if (max_len < 1) {
        aio->ret = 0;
        aio_event_set(aio, 2, timeout);
        aio_ready_do(aio);
        return 0;
    }

    while (1) {
        if (aio->read_cache.len == 0) {
            magic = -1;
        } else if (aio->read_cache.len > max_len) {
            magic = max_len;
        } else {
            magic = aio->read_cache.len;
        }

        if (magic > 0) {
            aio->ret = magic;
            aio_event_set(aio, 2, timeout);
            aio_ready_do(aio);
            return 0;
        }
        if (!(aio->ssl)) {
            rlen = ___Z_SYS_read(aio->fd, buf, 10240);
        } else {
            rlen = aio_try_ssl_read(aio, buf, 10240);
        }
        if (rlen == 0) {
            aio->ret = 0;
            aio_event_set(aio, 2, timeout);
            aio_ready_do(aio);
            return 0;
        }
        if (rlen < 1) {
            break;
        }
        ___aio_cache_append(aio, &(aio->read_cache), buf, rlen);
    }

    aio_event_set(aio, 1, timeout);

    return 0;
}

static inline int aio_read_n___inner(aio_t * aio, int strict_len, async_io_cb_t callback, long timeout)
{
    int magic, rlen;
    char buf[10240];

    aio->rw_type = ZAIO_CB_TYPE_READ_N;
    aio->callback = callback;
    aio->read_magic_len = strict_len;

    if (strict_len < 1) {
        aio->ret = 0;
        aio_event_set(aio, 2, timeout);
        aio_ready_do(aio);
        return 0;
    }
    while (1) {
        if (aio->read_cache.len < strict_len) {
            magic = -1;
        } else {
            magic = strict_len;
        }
        if (magic > 0) {
            aio->ret = magic;
            aio_event_set(aio, 2, timeout);
            aio_ready_do(aio);
            return 0;
        }
        if (!(aio->ssl)) {
            rlen = ___Z_SYS_read(aio->fd, buf, 10240);
        } else {
            rlen = aio_try_ssl_read(aio, buf, 10240);
        }
        if (rlen == 0) {
            aio->ret = 0;
            aio_event_set(aio, 2, timeout);
            aio_ready_do(aio);
            return 0;
        }
        if (rlen < 0) {
            break;
        }
        ___aio_cache_append(aio, &(aio->read_cache), buf, rlen);
    }

    aio_event_set(aio, 1, timeout);

    return 0;
}

static inline int ___aio_read_size_data_check(aio_t * aio)
{
    int magic = 0, end, ch, shift = 0, ci = 0, size = 0;
    unsigned char *buf;
    char tmpbuf[10];
    aio_rwbuf_t *rwb;

    if (aio->read_cache.len == 0) {
        return -1;
    }

    for (rwb = aio->read_cache.head; rwb; rwb = rwb->next) {
        buf = (unsigned char *)(rwb->data);
        end = rwb->p2 + 1;
        for (ci = rwb->p1; ci != end; ci++) {
            magic++;
            ch = buf[ci];
            size |= ((ch & 0177) << shift);
            if (ch & 0200) {
                goto over;
            }
            if (magic > 4) {
                return -2;
            }
            shift += 7;
        }
    }
    return -1;
over:
    ___aio_cache_shift(aio, &(aio->read_cache), tmpbuf, size);
    return size;
}

static inline int aio_read_size_data___inner(aio_t * aio, async_io_cb_t callback, long timeout)
{
    int magic, rlen;
    char buf[10240];

    aio->rw_type = ZAIO_CB_TYPE_READ_N;
    aio->callback = callback;

    while (1) {
        if (aio->read_magic_len == 0) {
           rlen = ___aio_read_size_data_check(aio);
           if ((rlen == -2) || (rlen == 0)) {
               aio->ret = -1;
               aio_event_set(aio, 0, -2);
               aio_ready_do(aio);
               return 0;
           } else if (rlen > 0) {
               aio->read_magic_len = rlen;
               continue;
           }
        } else {
            if (aio->read_cache.len < aio->read_magic_len) {
                magic = -1;
            } else {
                magic = aio->read_magic_len;
            }
            if (magic > 0) {
                aio->ret = magic;
                aio_event_set(aio, 2, timeout);
                aio_ready_do(aio);
                return 0;
            }
        }
        if (!(aio->ssl)) {
            rlen = read(aio->fd, buf, 10240);
        } else {
            rlen = aio_try_ssl_read(aio, buf, 10240);
        }
        if (rlen < 1) {
            break;
        }
        ___aio_cache_append(aio, &(aio->read_cache), buf, rlen);
    }

    aio_event_set(aio, 1, timeout);

    return 0;
}

static inline int ___aio_read_delimiter_check(aio_t * aio, unsigned char delimiter, int max_len)
{
    int magic, i, end;
    char *buf;
    aio_rwbuf_t *rwb;

    if (aio->read_cache.len == 0) {
        return -1;
    }

    magic = 0;
    for (rwb = aio->read_cache.head; rwb; rwb = rwb->next) {
        buf = rwb->data;
        end = rwb->p2 + 1;
        for (i = rwb->p1; i != end; i++) {
            magic++;
            if (buf[i] == delimiter) {
                return magic;
            }
            if (magic == max_len) {
                return magic;
            }
        }
    }

    return -1;

}

static inline int aio_read_delimiter___inner(aio_t * aio, char delimiter, int max_len, async_io_cb_t callback, long timeout)
{
    int magic, rlen;
    char buf[10240 + 10];
    char *data;

    aio->rw_type = ZAIO_CB_TYPE_READ_DELIMETER;
    aio->callback = callback;
    aio->read_magic_len = max_len;
    aio->delimiter = delimiter;

    if (max_len < 1) {
        aio->ret = 0;
        aio_event_set(aio, 2, timeout);
        aio_ready_do(aio);
        return 0;
    }
    magic = ___aio_read_delimiter_check(aio, (unsigned char)delimiter, max_len);
    while (1) {
        if (magic > 0) {
            aio->ret = magic;
            aio_event_set(aio, 2, timeout);
            aio_ready_do(aio);
            return 0;
        }
        if (!(aio->ssl)) {
            rlen = ___Z_SYS_read(aio->fd, buf, 10240);
        } else {
            rlen = aio_try_ssl_read(aio, buf, 10240);
        }
        if (rlen < 1) {
            break;
        }
        ___aio_cache_append(aio, &(aio->read_cache), buf, rlen);
        data = (char *)memchr(buf, aio->delimiter, rlen);
        if (data) {
            magic = aio->read_cache.len - rlen + (data - buf + 1);
            if (magic > aio->read_magic_len) {
                magic = aio->read_magic_len;
            }
        } else {
            if (aio->read_magic_len <= aio->read_cache.len) {
                magic = aio->read_magic_len;
            } else {
                magic = -1;
            }
        }
    }

    aio_event_set(aio, 1, timeout);

    return 0;
}

static inline int aio_write_cache_flush___inner(aio_t * aio, async_io_cb_t callback, long timeout)
{
    aio->ret = 1;
    aio->rw_type = ZAIO_CB_TYPE_WRITE;
    aio->callback = callback;

    aio_event_set(aio, 1, timeout);

    return 0;
}

static inline int aio_sleep___inner(aio_t * aio, async_io_cb_t callback, long timeout)
{
    aio->rw_type = ZAIO_CB_TYPE_SLEEP;
    aio->callback = callback;
    aio_event_set(aio, 0, timeout);

    return 0;
}

static inline int aio_action_read_once(aio_t * aio, int rw_type, char *buf)
{
    int rlen, rlen_t;
    char *data;

    rlen_t = aio->read_magic_len - aio->read_cache.len;
    if (rlen_t < 10240) {
        rlen_t = 10240;
    }
    if (!(aio->ssl)) {
        rlen = ___Z_SYS_read(aio->fd, buf, rlen_t);
    } else {
        rlen = aio_try_ssl_read(aio, buf, rlen_t);
    }
    if (rlen < 1) {
        return -1;
    }
    ___aio_cache_append(aio, &(aio->read_cache), buf, rlen);

    if (rw_type == ZAIO_CB_TYPE_READ) {
        if (aio->read_cache.len >= aio->read_magic_len) {
            aio->ret = aio->read_magic_len;
        } else {
            aio->ret = aio->read_cache.len;
        }
        return 0;
    }
    if (rw_type == ZAIO_CB_TYPE_READ_N) {
        if (aio->read_cache.len >= aio->read_magic_len) {
            aio->ret = aio->read_magic_len;
            return 0;
        } else {
            return 1;
        }
    }
    if (rw_type == ZAIO_CB_TYPE_READ_DELIMETER) {
        data = (char *)memchr(buf, aio->delimiter, rlen);
        if (data) {
            //aio->ret = aio->read_cache.len - rlen + (data - buf + 1);
            aio->ret = aio->read_cache.len + (data - buf + 1) - rlen;
            if (aio->ret > aio->read_magic_len) {
                aio->ret = aio->read_magic_len;
            }
            return 0;
        } else {
            if (aio->read_magic_len <= aio->read_cache.len) {
                aio->ret = aio->read_magic_len;
                return 0;
            }
            return 1;
        }
    }

    return 0;
}

static inline int aio_action_read(aio_t * aio, int rw_type)
{
    char buf[1100000];
    int ret;

    while (1) {
        ret = aio_action_read_once(aio, rw_type, buf);
        if (ret == -1) {
            return 1;
        }
        if (ret == 0) {
            return 0;
        }
    }

    return 0;
}

static inline int aio_action_write(aio_t * aio, int rw_type)
{
    int wlen, rlen, len;
    char *data;

    while (1) {
        ___aio_cache_first_line(aio, &(aio->write_cache), &data, &len);
        if (len == 0) {
            aio->ret = 1;
            return 0;
        }
        wlen = len;
        if (!(aio->ssl)) {
            rlen = ___Z_SYS_write(aio->fd, data, wlen);
        } else {
            rlen = aio_try_ssl_write(aio, data, wlen);
        }
        if (rlen < 1) {
            return 1;
        }
        ___aio_cache_shift(aio, &(aio->write_cache), 0, rlen);
    }

    return 1;
}

static inline int aio_action(aio_t * aio)
{
    int rw_type;
    int events, transfer;
    transfer = 1;
    rw_type = aio->rw_type;
    events = aio->recv_events;

    if ((events & var_event_rdhup) && (rw_type & ZAIO_CB_TYPE_READ)) {
        /* peer closed */
        aio->ret = 0;
        transfer = 0;
        goto transfer;
    }

    if ((events & var_event_rdhup) && (rw_type & ZAIO_CB_READ) && (aio->read_cache.len < 1)) {
        /* peer closed */
        aio->ret = 0;
        transfer = 0;
        goto transfer;
    }
    if (events & var_event_exception) {
        aio->ret = -1;
        transfer = 0;
        goto transfer;
    }
    if (rw_type == ZAIO_CB_TYPE_SSL_INIT) {
        int rlen;
        if (aio->ssl_server_or_client) {
            rlen = aio_try_ssl_accept(aio);
        } else {
            rlen = aio_try_ssl_connect(aio);
        }
        if (rlen > 0) {
            aio_event_set(aio, 2, -2);
            aio_ready_do(aio);
            return 0;
        }
        transfer = 1;
        goto transfer;
    }

#define _ssl_w (((aio->ssl_write_want_write) &&(events & var_event_write))||((aio->ssl_write_want_read) &&(events & var_event_read)))
#define _ssl_r (((aio->ssl_read_want_write) &&(events & var_event_write))||((aio->ssl_read_want_read) &&(events & var_event_read)))
    if (((aio->ssl) && (_ssl_w)) || ((!(aio->ssl)) && (events & var_event_write))) {
        transfer = aio_action_write(aio, rw_type);
#if 0
        if ((rw_type & ZAIO_CB_WRITE)) {
            goto transfer;
        }
        transfer = 1;
#endif
        goto transfer;
    }

    if (((aio->ssl) && (_ssl_r)) || ((!(aio->ssl)) && (events & var_event_read))) {
        transfer = aio_action_read(aio, rw_type);
        goto transfer;
    }

    aio->ret = -1;
    transfer = 0;

transfer:
    if (aio->ssl && aio->ssl_error) {
        aio->ret = -1;
        transfer = 0;
    }

    if (transfer) {
        aio_event_set(aio, 1, -2);
        return 1;
    }

    aio_ready_do(aio);

    return 0;
}

/* ################################################################## */
/* evbase */

static void evbase_notify_reader(event_io &iev)
{
    uint64_t u;

    ___Z_SYS_read(iev.get_fd(), &u, sizeof(uint64_t));
}

static inline int evbase_queue_checker(evbase_t * eb)
{
    aio_t *aio;
    int rw_type;

    if (!eb->queue_head) {
        return 0;
    }

    while (1) {
        lock_evbase(eb);
        aio = eb->queue_head;
        if (!aio) {
            unlock_evbase(eb);
            return 0;
        }
        ZCC_MLINK_DETACH(eb->queue_head, eb->queue_tail, aio, queue_prev, queue_next);
        unlock_evbase(eb);

        rw_type = aio->rw_type;

        if (rw_type == ZAIO_CB_TYPE_READ) {
            aio_read___innner(aio, aio->read_magic_len, aio->callback, aio->ret);
        } else if (rw_type == ZAIO_CB_TYPE_READ_N) {
            aio_read_n___inner(aio, aio->read_magic_len, aio->callback, aio->ret);
        } else if (rw_type == ZAIO_CB_TYPE_READ_SIZE_DATA) {
            aio_read_size_data___inner(aio, aio->callback, aio->ret);
        } else if (rw_type == ZAIO_CB_TYPE_READ_DELIMETER) {
            aio_read_delimiter___inner(aio, aio->delimiter, aio->read_magic_len, aio->callback, aio->ret);
        } else if (rw_type == ZAIO_CB_TYPE_WRITE) {
            aio_write_cache_flush___inner(aio, aio->callback, aio->ret);
        } else if (rw_type == ZAIO_CB_TYPE_SLEEP) {
            aio_sleep___inner(aio, aio->callback, aio->ret);
        } else if (rw_type == ZAIO_CB_TYPE_SSL_INIT) {
            aio_ssl_init___inner(aio, aio->callback, aio->ret);
        } else {
            log_fatal("evbase: unknown cb");
        }
    }

    return 0;
}

/* ################################################################## */
event_io::event_io()
{
    memset(&___data, 0, sizeof(ev_t));
}

event_io::~event_io()
{
    fini();
}

void event_io::init(int fd, event_base &eb)
{
    if (!___data._init) {
        memset(&___data, 0, sizeof(ev_t));
        ___data.aio_type = var_event_type_event;
        ___data.evbase = &(eb.___data);
        ___data.fd = fd;
        ___data._init = 1;
    }
}

void event_io::fini()
{
    if (___data._init) {
        disable();
        memset(&___data, 0, sizeof(ev_t));
    }
}

void event_io::enable_event(int events, event_io_cb_t callback)
{
    ev_t *ev = &___data;
    int fd, old_events, e_events;
    struct epoll_event epv;
    evbase_t *eb;

    eb = ev->evbase;
    fd = ev->fd;
    e_events = EPOLLHUP | EPOLLRDHUP | EPOLLERR;

    ev->callback = callback;
    old_events = ev->events;
    ev->events = events;
    if (callback == 0) {
        events = 0;
    }

    lock_evbase(eb);
    if (events == 0) {
        if (old_events) {
            if (epoll_ctl(eb->epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
                log_fatal("enable_event: fd %d: DEL(%m)", fd);
            }
        }
    } else if (old_events != events) {
        if (events & var_event_read) {
            e_events |= EPOLLIN;
        }
        if (events & var_event_write) {
            e_events |= EPOLLOUT;
        }
        if (events & var_event_persist) {
            e_events |= EPOLLET;
        }
        epv.events = e_events;
        epv.data.ptr = ev;
        if (epoll_ctl(eb->epoll_fd, (old_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD), fd, &epv) == -1) {
            log_fatal("enable_event: fd %d: %s error: %m", fd, (old_events ? "MOD" : "ADD"));
        }
    }
    unlock_evbase(eb);
}

event_base *event_io::get_event_base()
{
    return ZCC_CONTAINER_OF(___data.evbase, event_base, ___data);
}

/* ################################################################## */
async_io::async_io()
{
    memset(&___data, 0, sizeof(aio_t));
    ___data.fd = -1;
}

async_io::~async_io()
{
    if (___data.fd != -1) {
        fini();
    }
}

void async_io::init(int fd, event_base &eb)
{
    aio_t * aio = &___data;
    memset(aio, 0, sizeof(aio_t));
    aio->aio_type = var_event_type_aio;
    aio->fd = fd;
    aio->evbase = &(eb.___data);
}

void async_io::fini()
{
    if (___data.fd == -1) {
        return;
    }

    aio_t * aio = &___data;
    aio_event_set(aio, 0, -2);

    if (aio->read_cache.len > 0) {
        ___aio_cache_shift(aio, &(aio->read_cache), 0, aio->read_cache.len);
    }
    if (aio->write_cache.len > 0) {
        ___aio_cache_shift(aio, &(aio->write_cache), 0, aio->write_cache.len);
    }
    if (aio->ssl) {
        SSL_shutdown(aio->ssl);
        SSL_free(aio->ssl);
        aio->ssl = 0;
    }

    memset(&___data, 0, sizeof(aio_t));
    ___data.fd = -1;
}

void async_io::ssl_init(SSL_CTX * ctx, async_io_cb_t callback, long timeout, bool server_or_client)
{
    aio_t * aio = &___data;
    evbase_t *eb = aio_get_base(aio);

    aio->ssl = openssl_create_SSL(ctx, aio->fd);
    aio->ssl_server_or_client = (server_or_client?1:0);

    aio->rw_type = ZAIO_CB_TYPE_SSL_INIT;
    aio->callback = callback;
    aio->ret = timeout;

    lock_evbase(eb);
    ZCC_MLINK_APPEND(eb->queue_head, eb->queue_tail, aio, queue_prev, queue_next);
    unlock_evbase(eb);
    if (eb->plock) {
        event_base *evb = get_event_base();
        evb->notify();
    }
}

SSL *async_io::detach_SSL()
{
    SSL *r = ___data.ssl;
    ___data.ssl = 0;
    ___data.ssl_server_or_client = 0;
    ___data.ssl_session_init = 0;
    ___data.ssl_read_want_read = 0;
    ___data.ssl_read_want_write = 0;
    ___data.ssl_write_want_read = 0;
    ___data.ssl_write_want_write = 0;
    ___data.ssl_error = 0;
    return r;
}

void async_io::fetch_rbuf(char *buf, int len)
{
    ___aio_cache_shift(&___data, &(___data.read_cache), buf, len);
}

void async_io::fetch_rbuf(string &dest, int len)
{
    dest.clear();
    dest.need_space(len);
    char *buf = const_cast<char *>(dest.c_str());
    ___aio_cache_shift(&___data, &(___data.read_cache), buf, len);
}

void async_io::read(int max_len, async_io_cb_t callback, long timeout)
{
    aio_t * aio = &___data;
    evbase_t *eb = aio_get_base(aio);

    aio->rw_type = ZAIO_CB_TYPE_READ;
    aio->callback = callback;
    aio->read_magic_len = max_len;
    aio->ret = timeout;

    lock_evbase(eb);
    ZCC_MLINK_APPEND(eb->queue_head, eb->queue_tail, aio, queue_prev, queue_next);
    unlock_evbase(eb);
    if (eb->plock) {
        event_base *evb = get_event_base();
        evb->notify();
    }
}

void async_io::readn(int strict_len, async_io_cb_t callback, long timeout)
{
    aio_t * aio = &___data;
    evbase_t *eb = aio_get_base(aio);

    aio->rw_type = ZAIO_CB_TYPE_READ_N;
    aio->callback = callback;
    aio->read_magic_len = strict_len;
    aio->ret = timeout;

    lock_evbase(eb);
    ZCC_MLINK_APPEND(eb->queue_head, eb->queue_tail, aio, queue_prev, queue_next);
    unlock_evbase(eb);
    if (eb->plock) {
        event_base *evb = get_event_base();
        evb->notify();
    }
}

void async_io::read_size_data(async_io_cb_t callback, long timeout)
{
    aio_t * aio = &___data;
    evbase_t *eb = aio_get_base(aio);

    aio->delimiter = 0;
    aio->rw_type = ZAIO_CB_TYPE_READ_SIZE_DATA;
    aio->callback = callback;
    aio->read_magic_len = 0;
    aio->ret = timeout;

    lock_evbase(eb);
    ZCC_MLINK_APPEND(eb->queue_head, eb->queue_tail, aio, queue_prev, queue_next);
    unlock_evbase(eb);
    if (eb->plock) {
        event_base *evb = get_event_base();
        evb->notify();
    }
}

void async_io::read_delimiter(int delimiter, int max_len, async_io_cb_t callback, long timeout)
{
    aio_t * aio = &___data;
    evbase_t *eb = aio_get_base(aio);

    aio->rw_type = ZAIO_CB_TYPE_READ_DELIMETER;
    aio->delimiter = delimiter;
    aio->read_magic_len = max_len;
    aio->callback = callback;
    aio->ret = timeout;

    lock_evbase(eb);
    ZCC_MLINK_APPEND(eb->queue_head, eb->queue_tail, aio, queue_prev, queue_next);
    unlock_evbase(eb);
    if (eb->plock) {
        event_base *evb = get_event_base();
        evb->notify();
    }
}

void async_io::cache_printf_1024(const char *fmt, ...)
{
    va_list ap;
    char buf[1024+1];
    int len;

    va_start(ap, fmt);
    len = vsnprintf(buf, 1024, fmt, ap);
    va_end(ap);

    cache_write(buf, len);
}

void async_io::cache_puts(const char *s)
{
    cache_write(s, strlen(s));
}

void async_io::cache_write(const void *buf, size_t len)
{
    if (len < 1) {
        return;
    }
    ___aio_cache_append(&___data, &(___data.write_cache), buf, len);
}

void async_io::cache_write_size_data(const void *buf, size_t len)
{
    char sbuf[32];
    size_t ret = size_data_put_size(len, sbuf);
    cache_write(sbuf, ret);
    cache_write(buf, len);
}
void async_io::cache_write_direct(const void *buf, size_t len)
{
    aio_rwbuf_t *rwb;
    evbase_t *eb = aio_get_base(&___data);

    if (len < 1) {
        return;
    }
 
    lock_evbase(eb);
    rwb = (aio_rwbuf_t *) eb->aio_rwbuf_mpool->require();
    unlock_evbase(eb);
    rwb->next = 0;
    rwb->long_flag = 1;
    rwb->p1 = 0;
    rwb->p2 = 0;

    aio_rwbuf_longbuf_t *lb = (aio_rwbuf_longbuf_t *)(rwb->data);
    lb->data = (char *)buf;
    lb->p1 = 0;
    lb->p2 = len - 1;

    if (___data.write_cache.head) {
        ___data.write_cache.head->next = rwb;
        ___data.write_cache.tail = rwb;
    } else {
        ___data.write_cache.head = rwb;
        ___data.write_cache.tail = rwb;
    }
}


void async_io::cache_flush(async_io_cb_t callback, long timeout)
{
    aio_t * aio = &___data;
    evbase_t *eb = aio_get_base(aio);

    aio->rw_type = ZAIO_CB_TYPE_WRITE;
    aio->callback = callback;
    aio->ret = timeout;

    lock_evbase(eb);
    ZCC_MLINK_APPEND(eb->queue_head, eb->queue_tail, aio, queue_prev, queue_next);
    unlock_evbase(eb);
    if (eb->plock) {
        event_base *evb = get_event_base();
        evb->notify();
    }
}

void async_io::sleep(async_io_cb_t callback, long timeout)
{
    aio_t * aio = &___data;
    evbase_t *eb = aio_get_base(aio);

    aio->rw_type = ZAIO_CB_TYPE_SLEEP;
    aio->callback = callback;
    aio->ret = timeout;

    lock_evbase(eb);
    ZCC_MLINK_APPEND(eb->queue_head, eb->queue_tail, aio, queue_prev, queue_next);
    unlock_evbase(eb);
    if (eb->plock) {
        event_base *evb = get_event_base();
        evb->notify();
    }
}

event_base *async_io::get_event_base()
{
    return ZCC_CONTAINER_OF(___data.evbase, event_base, ___data);
}

/* ################################################################## */
static int event_timer_cmp(rbtree_node_t * n1, rbtree_node_t * n2)
{
    evtimer_t *t1, *t2;
    long r;

    t1 = ZCC_CONTAINER_OF(n1, evtimer_t, rbnode_time);
    t2 = ZCC_CONTAINER_OF(n2, evtimer_t, rbnode_time);

    r = t1->timeout - t2->timeout;

    if (!r) {
        r = (char *)(n1) - (char *)(n2);
    }
    if (r > 0) {
        return 1;
    } else if (r < 0) {
        return -1;
    }

    return 0;
}

static int inline event_timer_check(evbase_t * eb)
{
    evtimer_t *timer;
    rbtree_node_t *rn;
    long delay;
    event_timer_cb_t callback;
    rbtree_t *timer_tree = &(eb->event_timer_tree);

    if (!rbtree_have_data(timer_tree)) {
        return delay;
    }
    while (1) {
        lock_evbase(eb);
        rn = rbtree_first(timer_tree);
        if (!rn) {
            unlock_evbase(eb);
            return 10 * 1000;
        }
        timer = ZCC_CONTAINER_OF(rn, evtimer_t, rbnode_time);
        delay = timeout_left(timer->timeout);
        if (delay > 0) {
            unlock_evbase(eb);
            return delay;
        }
        callback = timer->callback;
        rbtree_detach(timer_tree, &(timer->rbnode_time));
        timer->in_time = 0;
        unlock_evbase(eb);

        if (callback) {
            (callback) (*ZCC_CONTAINER_OF(timer, event_timer, ___data));
        }
    }

    return 10 * 1000;
}

event_timer::event_timer()
{
    memset(&___data, 0, sizeof(evtimer_t));
}

event_timer::~event_timer()
{
    fini();
}

void event_timer::init(event_base &evbase)
{
    ___data.evbase = &(evbase.___data);
    ___data.init = 1;
}

void event_timer::fini()
{
    if (___data.init) {
        stop();
    }
    memset(&___data, 0, sizeof(evtimer_t));
}

void event_timer::start(event_timer_cb_t callback, long timeout)
{
    evtimer_t *timer = &___data;
    evbase_t *eb = timer->evbase;
    rbtree_t *timer_tree = &(eb->event_timer_tree);
    rbtree_node_t *rn = &(timer->rbnode_time);

    if (!(timer->init)) {
        return;
    }

    lock_evbase(eb);
    if (timeout > 0) {
        if (timer->in_time) {
            rbtree_detach(timer_tree, rn);
        }
        timer->callback = callback;
        timer->timeout = timeout_set(timeout);
        rbtree_attach(timer_tree, rn);
        timer->in_time = 1;
    } else {
        if (timer->in_time) {
            rbtree_detach(timer_tree, rn);
        }
        timer->in_time = 0;
    }
    unlock_evbase(eb);
    if (!(___data.is_local)) {
        event_base *evb = get_event_base();
        evb->notify();
    }
}

void event_timer::stop()
{
    if(___data.init) {
        return;
    }
    start(0, 0);
}

event_base *event_timer::get_event_base()
{
    return ZCC_CONTAINER_OF(___data.evbase, event_base, ___data);
}


/* ################################################################## */

event_base *default_event_base = 0;
void default_event_base_create(void)
{
    if (!default_event_base) {
        default_event_base = new event_base();
    }
}

event_base::event_base()
{
    evbase_t *eb = &___data;
    int eventfd_fd;

    memset(eb, 0, sizeof(evbase_t));

    eb->aio_rwbuf_mpool = new mem_piece();
    eb->aio_rwbuf_mpool->init(sizeof(aio_rwbuf_t));

    rbtree_init(&(eb->event_timer_tree), event_timer_cmp);
    rbtree_init(&(eb->aio_timer_tree), aio_timer_cmp);

    eb->epoll_fd = epoll_create(1024);
    close_on_exec(eb->epoll_fd, true);

    eb->epoll_event_list = (struct epoll_event *)malloc(sizeof(struct epoll_event) * epoll_event_count);

    eventfd_fd = eventfd(0, 0);
    close_on_exec(eventfd_fd, true);
    nonblocking(eventfd_fd, 1);

    eb->eventfd_event = new event_io();
    eb->eventfd_event->init(eventfd_fd, *this);
    eb->eventfd_event->enable_event(var_event_read, evbase_notify_reader);
}

event_base::~event_base()
{
    evbase_t *eb = &___data;
    int efd = eb->eventfd_event->get_fd();
    delete eb->eventfd_event;
    close(efd);
    close(eb->epoll_fd);
    free(eb->epoll_event_list);
    delete eb->aio_rwbuf_mpool;
    if (eb->plock) {
        pthread_mutex_destroy((pthread_mutex_t *)(eb->plock));
        free(eb->plock);
    }
}

void event_base::notify()
{
    uint64_t u = 1;
    ___Z_SYS_write(___data.eventfd_event->___data.fd, &u, sizeof(uint64_t));
}

void event_base::option_plock()
{
    ___data.plock = (void *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init((pthread_mutex_t *)(___data.plock), 0);
}

void event_base::dispatch()
{
    evbase_t *eb = &___data;
    int i, nfds, events, recv_events;
    struct epoll_event *epv;
    aio_t *aio;
    ev_t *ev;
    long delay = 10 * 1000, delay_tmp;

    if ((delay < 1) || (delay > 10 * 1000)) {
        delay = 10 * 1000;
    }

    if (eb->queue_head) {
        evbase_queue_checker(eb);
    }

    if (rbtree_have_data(&(eb->event_timer_tree))) {
        delay_tmp = event_timer_check(eb);
        if (delay_tmp < delay) {
            delay = delay_tmp;
        }
    }
    if (rbtree_have_data(&(eb->aio_timer_tree))) {
        delay_tmp = aio_timer_check(eb);
        if (delay_tmp < delay) {
            delay = delay_tmp;
        }
    }

    nfds = epoll_wait(eb->epoll_fd, eb->epoll_event_list, epoll_event_count, delay);
    if (nfds == -1) {
        if (errno != EINTR) {
            log_fatal("zbase_dispatch: epoll_wait: %m");
        }
        return;
    }

    for (i = 0; i < nfds; i++) {
        epv = eb->epoll_event_list + i;
        events = epv->events;
        recv_events = 0;
        if (events & EPOLLHUP) {
            recv_events |= var_event_hup;
        }
        if (events & EPOLLRDHUP) {
            recv_events |= var_event_rdhup;
        }
        if (events & EPOLLERR) {
            recv_events |= var_event_error;
        }
        if (events & EPOLLIN) {
            recv_events |= var_event_read;
        }
        if (events & EPOLLOUT) {
            recv_events |= var_event_write;
        }
        aio = (aio_t *) (epv->data.ptr);
        if (aio->aio_type == var_event_type_event) {
            ev = (ev_t *) aio;
            ev->recv_events = recv_events;
            {
                event_io_cb_t callback = ev->callback;

                if (callback) {
                    (callback) (*ZCC_CONTAINER_OF(ev, event_io, ___data));
                } else {
                    log_fatal("ev: not found callback");
                }
                return;
            }

        } else if (1 || (aio->aio_type == var_event_type_aio)) {
            aio->recv_events = recv_events;
            aio_action(aio);
        }
    }
    return;
}


}
