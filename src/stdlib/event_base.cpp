/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-01-22
 * ================================
 */

#include "zcc.h"
#include <pthread.h>

namespace zcc
{

event_base default_evbase;

#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/un.h>

#pragma pack(push, 1)
/* {{{ global vars, declare, macro, etc */
typedef struct event_io_t event_io_t;
typedef struct async_io_t async_io_t;
typedef struct event_timer_t event_timer_t;
typedef struct event_base_t event_base_t;

#define lock_evbase(eb)   {if(((event_base_t *)(eb))->plocker_flag){ \
    zcc_pthread_lock(&(((event_base_t *)(eb))->plocker));}}
#define unlock_evbase(eb)   {if(((event_base_t *)(eb))->plocker_flag){ \
    zcc_pthread_unlock(&(((event_base_t *)(eb))->plocker));}}

#define event_io_get_data(o)      ((event_io_t *)(o))
#define async_io_get_data(o)      ((async_io_t *)(o))
#define event_timer_get_data(o)   ((event_timer_t *)(o))
#define event_base_get_data(o)    ((event_base_t *)(o))

ssize_t syscall_read(int fd, void *buf, size_t count);
ssize_t syscall_write(int fd, const void *buf, size_t count);
static int async_io_event_set(async_io_t * aio_data, int ev_type, long timeout);
static void async_io_ready_do(async_io_t * aio_data);

#define async_io_append_queue(eb_data, aio_data) \
    zcc_mlink_append(eb_data->queue_head, eb_data->queue_tail, &(aio_data->rbnode_time), rbtree_left, rbtree_right)

#define var_event_none                      0X00U
#define var_event_read                      0X01U
#define var_event_write                     0X02U
#define var_event_rdwr                      0X03U
#define var_event_rdhup                     0X10U
#define var_event_hup                       0X20U
#define var_event_error                     0X40U
#define var_event_errors                    0X70U
#define var_event_timeout                   0X80U
#define var_event_exception                 0XF0U
#define var_event_type_event                0X01
#define var_event_type_aio                  0X02

#define var_async_io_magic                       0XF0U
#define var_async_io_type_none                   0X00U
#define var_async_io_read                        0X10U
#define var_async_io_type_read                   0X11U
#define var_async_io_type_read_n                 0X12U
#define var_async_io_type_read_delimeter         0X13U
#define var_async_io_type_read_size_data         0X14U
#define var_async_io_write                       0X20U
#define var_async_io_type_write                  0X21U
#define var_async_io_type_sleep                  0X31U
#define var_async_io_type_ssl_init               0X41U

#define var_async_io_event_disable               0
#define var_async_io_event_enable                1
#define var_async_io_event_reserve               2

#define var_epoll_vecotr_count              4096
/* }}} */

/* {{{ struct event_io_t, async_io_t, event_timer_t, event_timer_t, event_base_t */
typedef void (*event_io_cb_t) (event_io &ev);
struct event_io_t {
    unsigned char aio_type:3;
    unsigned char is_local:1;
    unsigned char events;
    unsigned char revents;
    int fd;
    event_io_cb_t callback;
    void *context;
    event_base *evbase;
};

typedef struct aio_rwbuf_list_t aio_rwbuf_list_t;
const int var_async_io_rwbuf_size  = 4096;
struct aio_rwbuf_t {
    aio_rwbuf_t *next;
    unsigned int long_flag:1;
    unsigned int p1:15;
    unsigned int p2:15;
    char data[var_async_io_rwbuf_size];
};

typedef struct aio_rwbuf_longbuf_t aio_rwbuf_longbuf_t;
struct aio_rwbuf_longbuf_t {
    size_t p1;
    size_t p2;
    char *data;
};
struct aio_rwbuf_list_t {
    int len;
    aio_rwbuf_t *head;
    aio_rwbuf_t *tail;
};

typedef void (*async_io_cb_t) (async_io &as);
struct async_io_t {
    unsigned char aio_type:4;
    unsigned char is_local:1;
    unsigned char in_timeout:1;
    unsigned char want_read:1;
    unsigned char is_size_data:1;
    unsigned char events;
    unsigned char revents;
    unsigned char action_type;
    char delimiter;
    int fd;
    int read_magic_len;
    int ret;
    async_io_cb_t callback;
    aio_rwbuf_list_t read_cache;
    aio_rwbuf_list_t write_cache;
    long timeout;
    rbtree_node_t rbnode_time;
    unsigned char ssl_server_or_client:1;
    unsigned char ssl_session_init:1;
    unsigned char ssl_read_want_read:1;
    unsigned char ssl_read_want_write:1;
    unsigned char ssl_write_want_read:1;
    unsigned char ssl_write_want_write:1;
    unsigned char ssl_error_or_closed:1;
    SSL *ssl;
    void *context;
    event_base *evbase;
};

typedef void (*event_timer_cb_t) (event_timer &);
struct event_timer_t {
    long timeout;
    event_timer_cb_t callback;
    void *context;
    rbtree_node_t rbnode_time;
    event_base *evbase;
    unsigned char in_time:1;
    unsigned char is_local:1;
};

struct event_base_t {
    int epoll_fd;
    struct epoll_event *epool_event_vector;
    rbtree_t async_io_timeout_tree;
    rbtree_t event_timer_timeout_tree;
    event_io *eventfd_event;
    void *context;

    rbtree_node_t *queue_head;
    rbtree_node_t *queue_tail;

    pthread_mutex_t plocker;
    bool plocker_flag;
};

/* }}} */

/* {{{ event_timer */
static int event_timer_timeout_cmp(rbtree_node_t * n1, rbtree_node_t * n2)
{
    event_timer_t *t1, *t2;
    long r;

    t1 = zcc_container_of(n1, event_timer_t, rbnode_time);
    t2 = zcc_container_of(n2, event_timer_t, rbnode_time);

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

event_timer::event_timer()
{
    memset(&___data, 0, sizeof(___data));
}

event_timer::~event_timer()
{
    event_timer_t * et_data = (event_timer_t*)(___data);
    if (et_data->evbase) {
        stop();
    }
}

void event_timer::bind(event_base &evbase)
{
    event_timer_t * et_data = event_timer_get_data(this);
    et_data->evbase = &evbase;
}

void event_timer::start(event_timer_cb_t callback, long timeout)
{
    event_timer_t *timer = event_timer_get_data(this);
    event_base *eb = timer->evbase;
    event_base_t *eb_data = event_base_get_data(eb);
    rbtree_t *timer_tree = &(eb_data->event_timer_timeout_tree);
    rbtree_node_t *rn = &(timer->rbnode_time);

    if (!timer->evbase) {
        return;
    }

    lock_evbase(eb_data);
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
    unlock_evbase(eb_data);
    if (!(timer->is_local)) {
        eb->notify();
    }
}

void event_timer::stop()
{
    start(0, 0);
}

void event_timer::option_local()
{
    event_timer_t *timer = event_timer_get_data(this);
    timer->is_local = 1;
}

void *event_timer::get_context()
{
    event_timer_t *timer = event_timer_get_data(this);
    return timer->context;
}

event_base &event_timer::get_event_base()
{
    event_timer_t *timer = event_timer_get_data(this);
    return *(timer->evbase);
}

static inline long event_timer_timeout_check(event_base_t * eb_data)
{
    event_timer_t *timer;
    rbtree_node_t *rn;
    long delay = 1 * 1000;
    event_timer_cb_t callback;
    rbtree_t *timer_tree = &(eb_data->event_timer_timeout_tree);

    if (!rbtree_have_data(timer_tree)) {
        return delay;
    }
    while (1) {
        if (var_proc_stop) {
            break;
        }
        lock_evbase(eb_data);
        rn = rbtree_first(timer_tree);
        if (!rn) {
            unlock_evbase(eb_data);
            return 1 * 1000;
        }
        timer = zcc_container_of(rn, event_timer_t, rbnode_time);
        delay = timeout_left(timer->timeout);
        if (delay > 0) {
            unlock_evbase(eb_data);
            return delay;
        }
        callback = timer->callback;
        rbtree_detach(timer_tree, &(timer->rbnode_time));
        timer->in_time = 0;
        unlock_evbase(eb_data);

        if (callback) {
            (callback) (*(event_timer *)(timer));
        }
    }
    return 1 * 1000;
}
/* }}} */

/* {{{ event_io */
event_io::event_io()
{
    memset(___data, 0, sizeof(___data));
    event_io_t *eio_data = (event_io_t *)___data;
    eio_data->aio_type = var_event_type_event;
    eio_data->fd = -1;
}

event_io::~event_io()
{
    event_io_t *eio_data = (event_io_t *)___data;
    if (eio_data->fd != -1) {
        disable();
    }
}

void event_io::bind(int fd, event_base &eb)
{
    event_io_t *eio_data = (event_io_t *)___data;
    eio_data->aio_type = var_event_type_event;
    eio_data->evbase = &eb;
    eio_data->fd = fd;
}

void event_io::option_local()
{
    event_io_t *eio_data = event_io_get_data(this);
    eio_data->is_local = 1;
}

ssize_t event_io::get_result()
{
    event_io_t *eio_data = event_io_get_data(this);
    if (eio_data->revents & (var_event_error | var_event_hup)) {
        return -1;
    }
    if (eio_data->revents & (var_event_rdhup)) {
        return 0;
    }
    if (eio_data->revents & (var_event_read|var_event_write)) {
        return 1;
    }
    return 1;
}

int event_io::get_fd()
{
    event_io_t *eio_data = event_io_get_data(this);
    return eio_data->fd;
}

static void ___enable_event(event_io_t *eio_data, unsigned char events, void (*callback)(event_io&))
{
    event_base *eb = eio_data->evbase;
    event_base_t *eb_data = event_base_get_data(eb);
    int fd = eio_data->fd;
    struct epoll_event epev;
    unsigned char old_events = eio_data->events;
    unsigned int ep_events = EPOLLRDHUP;

    eio_data->revents = 0;
    eio_data->callback = callback;
    if (callback == 0) {
        events = 0;
    }
    eio_data->events = events;

    lock_evbase(eb_data);
    if (events == 0) {
        if (old_events) {
            if (epoll_ctl(eb_data->epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
                zcc_fatal("enable_event: fd %d: DEL(%m)", fd);
            }
        }
    } else if (old_events != events) {
        if (events & var_event_read) {
            ep_events |= EPOLLIN;
        }
        if (events & var_event_write) {
            ep_events |= EPOLLOUT;
        }
        epev.events = ep_events;
        epev.data.ptr = eio_data;
        if (epoll_ctl(eb_data->epoll_fd, (old_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD), fd, &epev) == -1) {
            zcc_fatal("enable_event: fd %d: %s error: %m", fd, (old_events ? "MOD" : "ADD"));
        }
    }
    unlock_evbase(eb_data);
}

void event_io::enable_read(void (*callback)(event_io &))
{
    event_io_t *eio_data = event_io_get_data(this);
    ___enable_event(eio_data, var_event_read, callback);
}

void event_io::enable_write(void (*callback)(event_io &))
{
    event_io_t *eio_data = event_io_get_data(this);
    ___enable_event(eio_data, var_event_write, callback);
}

void event_io::disable()
{
    event_io_t *eio_data = event_io_get_data(this);
    ___enable_event(eio_data, 0, 0);
}

void event_io::set_context(const void *ctx)
{
    event_io_t *eio_data = event_io_get_data(this);
    eio_data->context = const_cast<void *>(ctx);;
}

void *event_io::get_context()
{
    event_io_t *eio_data = event_io_get_data(this);
    return eio_data->context;
}

event_base &event_io::get_event_base()
{
    event_io_t *eio_data = event_io_get_data(this);
    return *(eio_data->evbase);
}

/* }}} */

/* {{{ rwbuf op */
static void ___async_io_cache_shift(async_io_t * aio_data, aio_rwbuf_list_t * ioc, void *data, int len)
{
    int rlen, i, olen = len;
    char *buf = (char *)data;
    char *cdata;
    aio_rwbuf_t *rwb;

    rwb = ioc->head;
    if (rwb && rwb->long_flag) {
        /* only write */
        aio_rwbuf_longbuf_t *lb = (aio_rwbuf_longbuf_t *)(rwb->data);
        lb->p1 += len;
        if (lb->p1 == lb->p2) {
            ioc->head = rwb->next;
            free(rwb);
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
            free(ioc->head);
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

static void ___async_io_cache_first_line(async_io_t * aio_data, aio_rwbuf_list_t * ioc, char **data, int *len)
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

static void ___async_io_cache_append(async_io_t * aio_data, aio_rwbuf_list_t * ioc, const void *data, int len)
{
    char *buf = (char *)data;
    char *cdata;
    int i, p2;
    aio_rwbuf_t *rwb;

    rwb = ioc->tail;
    p2 = 0;
    cdata = 0;
    if (rwb) {
        p2 = rwb->p2;
        cdata = rwb->data;
    }
    for (i = 0; i < len; i++) {
        if (!rwb || (p2 == var_async_io_rwbuf_size - 1)) {
            rwb = (aio_rwbuf_t *) calloc(1, sizeof(aio_rwbuf_t));
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
/* }}} */

/* {{{ ssl read/write/connect/accept/init */
static inline int async_io_ssl_read(async_io_t * aio_data, void *buf, int len)
{
    int rlen, status;

    aio_data->ssl_read_want_write = 0;
    aio_data->ssl_read_want_read = 0;

    rlen = SSL_read(aio_data->ssl, buf, len);
    if (rlen < 1) {
        status = SSL_get_error(aio_data->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE) {
            aio_data->ssl_read_want_write = 1;
            rlen = -1;
        } else if (status == SSL_ERROR_WANT_READ) {
            aio_data->ssl_read_want_read = 1;
            rlen = -1;
        } else {
            aio_data->ssl_error_or_closed = 1;
        }
    }
    return rlen;
}

static inline int async_io_ssl_write(async_io_t * aio_data, void *buf, int len)
{
    int rlen, status;

    aio_data->ssl_write_want_write = 0;
    aio_data->ssl_write_want_read = 0;

    rlen = SSL_write(aio_data->ssl, buf, len);
    if (rlen < 1) {
        status = SSL_get_error(aio_data->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE) {
            aio_data->ssl_write_want_write = 1;
            rlen = -1;
        } else if (status == SSL_ERROR_WANT_READ) {
            aio_data->ssl_write_want_read = 1;
            rlen = -1;
        } else {
            aio_data->ssl_error_or_closed = 1;
        }
    }

    return rlen;
}

static inline int async_io_ssl_connect(async_io_t * aio_data)
{
    int rlen, status;

    aio_data->ssl_write_want_write = 0;
    aio_data->ssl_write_want_read = 0;

    rlen = SSL_connect(aio_data->ssl);
    if (rlen < 1) {
        status = SSL_get_error(aio_data->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE) {
            aio_data->ssl_write_want_write = 1;
        } else if (status == SSL_ERROR_WANT_READ) {
            aio_data->ssl_write_want_read = 1;
        } else {
            aio_data->ssl_error_or_closed = 1;
        }
    }

    return rlen;
}

static inline int async_io_ssl_accept(async_io_t * aio_data)
{
    int rlen, status;

    aio_data->ssl_read_want_write = 0;
    aio_data->ssl_read_want_read = 0;

    rlen = SSL_accept(aio_data->ssl);
    if (rlen < 1) {
        status = SSL_get_error(aio_data->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE) {
            aio_data->ssl_read_want_write = 1;
        } else if (status == SSL_ERROR_WANT_READ) {
            aio_data->ssl_read_want_read = 1;
        } else {
            aio_data->ssl_error_or_closed = 1;
        }
    }

    return rlen;
}

static inline int async_io_ssl_init___inner(async_io_t * aio_data, async_io_cb_t callback, long timeout)
{
    int rlen;

    aio_data->action_type = var_async_io_type_ssl_init;
    aio_data->callback = callback;

    if (aio_data->ssl_server_or_client) {
        rlen = async_io_ssl_accept(aio_data);
    } else {
        rlen = async_io_ssl_connect(aio_data);
    }

    if (rlen >= 0) {
        aio_data->ssl_session_init = 1;
        aio_data->ret = rlen;
        async_io_event_set(aio_data, var_async_io_event_reserve, timeout);
        async_io_ready_do(aio_data);
        return 0;
    }

    async_io_event_set(aio_data, var_async_io_event_enable, timeout);
    return 0;
}

static void async_io_ssl_init(async_io_t *aio_data, SSL_CTX * ctx, async_io_cb_t callback, long timeout, bool server_or_client)
{
    event_base *eb = aio_data->evbase;
    event_base_t *eb_data = event_base_get_data(eb);

    aio_data->ssl = openssl_create_SSL(ctx, aio_data->fd);
    aio_data->ssl_server_or_client = (server_or_client?1:0);

    aio_data->action_type = var_async_io_type_ssl_init;
    aio_data->callback = callback;
    aio_data->ret = timeout;

    lock_evbase(eb_data);
    async_io_append_queue(eb_data, aio_data);
    unlock_evbase(eb_data);
    if (eb_data->plocker_flag) {
        eb->notify();
    }
}
/* }}} */

/* {{{ async_io_ready_do */
static void async_io_ready_do(async_io_t * aio_data)
{
    async_io_cb_t callback;
    int action_type;

    callback = aio_data->callback;
    action_type = aio_data->action_type;

    if (!callback) {
        zcc_fatal("aio_data: no callback");
    }

    if (action_type == var_async_io_type_sleep) {
        aio_data->ret = 1;
    }
    if (aio_data->is_local == 0) {
        async_io_event_set(aio_data, var_async_io_event_disable, 0);
    }

    aio_data->action_type = var_async_io_type_none;
    (callback) (*((async_io *)(aio_data)));
}
/* }}} */

/* {{{ async_io_event_set */
static int async_io_event_set(async_io_t * aio_data, int ev_type, long timeout)
{
    event_base *eb = aio_data->evbase;
    event_base_t *eb_data = event_base_get_data(eb);
    rbtree_t *timer_tree = &(eb_data->async_io_timeout_tree);

    /* timeout */
    do {
        rbtree_node_t *rn;
        rn = &(aio_data->rbnode_time);
        if (timeout > 0) {
            if (aio_data->in_timeout) {
                rbtree_detach(timer_tree, rn);
            }
            aio_data->timeout = timeout_set(timeout);
            rbtree_attach(timer_tree, rn);
            aio_data->in_timeout = 1;
        } else if (timeout == 0) {
            if (aio_data->in_timeout) {
                rbtree_detach(timer_tree, rn);
            }
            aio_data->in_timeout = 0;
        }
    } while(0);

    /* event */
    do {
        if (ev_type == var_async_io_event_reserve) {
            break;
        }
        struct epoll_event epev;
        unsigned char old_events = aio_data->events, events;
        unsigned int ep_events;
        int fd = aio_data->fd;
        int action_type = aio_data->action_type;
        events = 0;
        if (ev_type == var_async_io_event_enable) {
            /* compute the events */
            if (!(aio_data->ssl)) {
                if (action_type & var_async_io_read) {
                    events |= var_event_read;
                }
                if (action_type & var_async_io_write) {
                    events |= var_event_write;
                }
            } else {
                if (aio_data->ssl_read_want_read || aio_data->ssl_write_want_read) {
                    events |= var_event_read;
                }
                if (aio_data->ssl_read_want_write || aio_data->ssl_write_want_write) {
                    events |= var_event_write;
                }
            }
        }
        ep_events = EPOLLHUP | EPOLLERR;
        aio_data->events = events;

        if (events == 0) {
            if (old_events) {
                if (epoll_ctl(eb_data->epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
                    zcc_fatal("async_io_event_set: fd %d: DEL  ssl_error_or_closed: %m", fd);
                }
            }
        } else if (old_events != events) {
            if (events & var_event_read) {
                ep_events |= EPOLLIN;
            }
            if (events & var_event_write) {
                ep_events |= EPOLLOUT;
            }
            epev.events = ep_events;
            epev.data.ptr = aio_data;
            if (epoll_ctl(eb_data->epoll_fd, (old_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD), fd, &epev) == -1) {
                zcc_fatal("async_io_event_set: fd %d: %s ssl_error_or_closed: %m", fd, (old_events ? "MOD" : "ADD"));
            }
        }
    } while(0);

    return 0;
}
/* }}} */

/* {{{ async_io_read___inner */
static inline void async_io_read___inner(async_io_t * aio_data, int max_len, async_io_cb_t callback, long timeout)
{
    int magic, rlen;
    char buf[10240];

    aio_data->action_type = var_async_io_type_read;
    aio_data->callback = callback;
    aio_data->read_magic_len = max_len;

    if (max_len < 1) {
        aio_data->ret = 0;
        async_io_event_set(aio_data, var_async_io_event_reserve, timeout);
        async_io_ready_do(aio_data);
        return;
    }

    while (1) {
        if (aio_data->read_cache.len == 0) {
            magic = -1;
        } else if (aio_data->read_cache.len > max_len) {
            magic = max_len;
        } else {
            magic = aio_data->read_cache.len;
        }

        if (magic > 0) {
            aio_data->ret = magic;
            async_io_event_set(aio_data, var_async_io_event_reserve, timeout);
            async_io_ready_do(aio_data);
            return;
        }
        if (!(aio_data->ssl)) {
            rlen = syscall_read(aio_data->fd, buf, 10240);
        } else {
            rlen = async_io_ssl_read(aio_data, buf, 10240);
        }
        if (rlen == 0) {
            aio_data->ret = 0;
            async_io_event_set(aio_data, var_async_io_event_disable, 0);
            async_io_ready_do(aio_data);
            return;
        }
        if (rlen < 0) {
            break;
        }
        ___async_io_cache_append(aio_data, &(aio_data->read_cache), buf, rlen);
    }

    async_io_event_set(aio_data, var_async_io_event_enable, timeout);
}
/* }}} */

/* {{{ async_io_read_n___inner */
static inline void async_io_read_n___inner(async_io_t * aio_data, int strict_len, async_io_cb_t callback, long timeout)
{
    int magic, rlen;
    char buf[10240];

    aio_data->action_type = var_async_io_type_read_n;
    aio_data->callback = callback;
    aio_data->read_magic_len = strict_len;

    if (strict_len < 1) {
        aio_data->ret = 0;
        async_io_event_set(aio_data, var_async_io_event_reserve, timeout);
        async_io_ready_do(aio_data);
        return;
    }
    while (1) {
        if (aio_data->read_cache.len < strict_len) {
            magic = -1;
        } else {
            magic = strict_len;
        }
        if (magic > 0) {
            aio_data->ret = magic;
            async_io_event_set(aio_data, var_async_io_event_reserve, timeout);
            async_io_ready_do(aio_data);
            return;
        }
        if (!(aio_data->ssl)) {
            rlen = syscall_read(aio_data->fd, buf, 10240);
        } else {
            rlen = async_io_ssl_read(aio_data, buf, 10240);
        }
        if (rlen == 0) {
            aio_data->ret = (aio_data->read_cache.len?0:-1);
            async_io_event_set(aio_data, var_async_io_event_reserve, timeout);
            async_io_ready_do(aio_data);
            return;
        }
        if (rlen < 0) {
            break;
        }
        ___async_io_cache_append(aio_data, &(aio_data->read_cache), buf, rlen);
    }

    async_io_event_set(aio_data, var_async_io_event_enable, timeout);
}
/* }}} */

/* {{{ async_io_read_size_data___inner */
static inline int ___async_io_read_size_data_check(async_io_t * aio_data)
{
    int magic = 0, end, ch, shift = 0, ci = 0, size = 0;
    unsigned char *buf;
    char tmpbuf[10];
    aio_rwbuf_t *rwb;

    if (aio_data->read_cache.len == 0) {
        return -1;
    }

    for (rwb = aio_data->read_cache.head; rwb; rwb = rwb->next) {
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
    ___async_io_cache_shift(aio_data, &(aio_data->read_cache), tmpbuf, magic);
    return size;
}

static inline void async_io_read_size_data___inner(async_io_t * aio_data, async_io_cb_t callback, long timeout)
{
    int magic, rlen;
    char buf[10240];

    aio_data->action_type = var_async_io_type_read_n;
    aio_data->callback = callback;

    while (1) {
        if (aio_data->read_magic_len == 0) {
           rlen = ___async_io_read_size_data_check(aio_data);
           if ((rlen == -2) || (rlen == 0)) {
               aio_data->ret = (rlen?-1:0);
               async_io_event_set(aio_data, var_async_io_event_disable, timeout);
               async_io_ready_do(aio_data);
               return;
           } else if (rlen > 0) {
               aio_data->read_magic_len = rlen;
               continue;
           }
        } else {
            if (aio_data->read_cache.len < aio_data->read_magic_len) {
                magic = -1;
            } else {
                magic = aio_data->read_magic_len;
            }
            if (magic > 0) {
                aio_data->ret = magic;
                async_io_event_set(aio_data, var_async_io_event_reserve, timeout);
                async_io_ready_do(aio_data);
                return;
            }
        }
        if (!(aio_data->ssl)) {
            rlen = read(aio_data->fd, buf, 10240);
        } else {
            rlen = async_io_ssl_read(aio_data, buf, 10240);
        }
        if (rlen == 0) {
            aio_data->ret = -1;
            async_io_event_set(aio_data, var_async_io_event_reserve, timeout);
            async_io_ready_do(aio_data);
            return;
        }
        if (rlen < 1) {
            break;
        }
        ___async_io_cache_append(aio_data, &(aio_data->read_cache), buf, rlen);
    }

    async_io_event_set(aio_data, var_async_io_event_enable, timeout);
}
/* }}} */

/* {{{ async_io_read_delimiter___inner */
static inline int ___async_io_read_delimiter_check(async_io_t * aio_data, unsigned char delimiter, int max_len)
{
    int magic, i, end;
    char *buf;
    aio_rwbuf_t *rwb;

    if (aio_data->read_cache.len == 0) {
        return -1;
    }

    magic = 0;
    for (rwb = aio_data->read_cache.head; rwb; rwb = rwb->next) {
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

static inline void async_io_read_delimiter___inner(async_io_t * aio_data, char delimiter, int max_len, async_io_cb_t callback, long timeout)
{
    int magic, rlen;
    char buf[10240 + 10];
    char *data;

    aio_data->action_type = var_async_io_type_read_delimeter;
    aio_data->callback = callback;
    aio_data->read_magic_len = max_len;
    aio_data->delimiter = delimiter;

    if (max_len < 1) {
        aio_data->ret = 0;
        async_io_event_set(aio_data, var_async_io_event_reserve, timeout);
        async_io_ready_do(aio_data);
        return;
    }
    magic = ___async_io_read_delimiter_check(aio_data, (unsigned char)delimiter, max_len);
    while (1) {
        if (magic > 0) {
            aio_data->ret = magic;
            async_io_event_set(aio_data, var_async_io_event_reserve, timeout);
            async_io_ready_do(aio_data);
            return;
        }
        if (!(aio_data->ssl)) {
            rlen = syscall_read(aio_data->fd, buf, 10240);
        } else {
            rlen = async_io_ssl_read(aio_data, buf, 10240);
        }
        if (rlen == 0) {
            aio_data->ret = (magic>0?-1:0);
            async_io_event_set(aio_data, var_async_io_event_disable, 0);
            async_io_ready_do(aio_data);
            return;
        }
        if (rlen < 0) {
            break;
        }
        ___async_io_cache_append(aio_data, &(aio_data->read_cache), buf, rlen);
        data = (char *)memchr(buf, aio_data->delimiter, rlen);
        if (data) {
            magic = aio_data->read_cache.len - rlen + (data - buf + 1);
            if (magic > aio_data->read_magic_len) {
                magic = aio_data->read_magic_len;
            }
        } else {
            if (aio_data->read_magic_len <= aio_data->read_cache.len) {
                magic = aio_data->read_magic_len;
            } else {
                magic = -1;
            }
        }
    }

    async_io_event_set(aio_data, var_async_io_event_enable, timeout);
}
/* }}} */

/* {{{ async_io_write_cache_flush___inner */
static inline void async_io_write_cache_flush___inner(async_io_t * aio_data, async_io_cb_t callback, long timeout)
{
    aio_data->ret = 1;
    aio_data->action_type = var_async_io_type_write;
    aio_data->callback = callback;

    int wlen, retlen, len;
    char *data;

    while (1) {
        ___async_io_cache_first_line(aio_data, &(aio_data->write_cache), &data, &len);
        if (len == 0) {
            aio_data->ret = 1;
            async_io_event_set(aio_data, var_async_io_event_reserve, timeout);
            async_io_ready_do(aio_data);
            return;
        }
        wlen = len;
        if (!(aio_data->ssl)) {
            retlen = syscall_write(aio_data->fd, data, wlen);
            if (retlen < 0) {
                if (errno == EPIPE) {
                    aio_data->ret = -1;
                    async_io_event_set(aio_data, var_async_io_event_disable, 0);
                    async_io_ready_do(aio_data);
                    return;
                }
            }
            if (retlen == 0){
                aio_data->ret = -1;
                async_io_event_set(aio_data, var_async_io_event_disable, 0);
                async_io_ready_do(aio_data);
                return;
            }
        } else {
            retlen = async_io_ssl_write(aio_data, data, wlen);
            if ((retlen == 0) || aio_data->ssl_error_or_closed){
                aio_data->ret = -1;
                async_io_event_set(aio_data, var_async_io_event_disable, 0);
                async_io_ready_do(aio_data);
                return;
            }
        }
        if (retlen < 1) {
            break;
        }
        ___async_io_cache_shift(aio_data, &(aio_data->write_cache), 0, retlen);
    }

    async_io_event_set(aio_data, var_async_io_event_enable, timeout);
}
/* }}} */

/* {{{ async_io_sleep___inner */
static inline void async_io_sleep___inner(async_io_t * aio_data, async_io_cb_t callback, long timeout)
{
    aio_data->action_type = var_async_io_type_sleep;
    aio_data->callback = callback;
    async_io_event_set(aio_data, var_async_io_event_disable, timeout);
}
/* }}} */

/* {{{ async_io_action */
static void async_io_action(async_io_t * aio_data)
{
    unsigned char action_type = aio_data->action_type;
    if (action_type == var_async_io_type_read) {
        async_io_read___inner(aio_data, aio_data->read_magic_len, aio_data->callback, aio_data->ret);
    } else if (action_type == var_async_io_type_read_n) {
        async_io_read_n___inner(aio_data, aio_data->read_magic_len, aio_data->callback, aio_data->ret);
    } else if (action_type == var_async_io_type_read_size_data) {
        async_io_read_size_data___inner(aio_data, aio_data->callback, aio_data->ret);
    } else if (action_type == var_async_io_type_read_delimeter) {
        async_io_read_delimiter___inner(aio_data, aio_data->delimiter, aio_data->read_magic_len, aio_data->callback, aio_data->ret);
    } else if (action_type == var_async_io_type_write) {
        async_io_write_cache_flush___inner(aio_data, aio_data->callback, aio_data->ret);
    } else if (action_type == var_async_io_type_sleep) {
        async_io_sleep___inner(aio_data, aio_data->callback, aio_data->ret);
    } else if (action_type == var_async_io_type_ssl_init) {
        async_io_ssl_init___inner(aio_data, aio_data->callback, aio_data->ret);
    } else {
        zcc_fatal("evbase: unknown cb");
    }
}
/* }}} */

/* {{{ async_io construct/unconstruct  */
async_io::async_io()
{
    async_io_t * aio_data = (async_io_t *)___data;
    memset(&___data, 0, sizeof(___data));
    aio_data->fd = -1;
}

async_io::~async_io()
{
    async_io_t * aio_data = (async_io_t *)___data;
    if (aio_data->fd != -1){
        async_io_event_set(aio_data, var_async_io_event_disable, 0);

        if (aio_data->read_cache.len > 0) {
            ___async_io_cache_shift(aio_data, &(aio_data->read_cache), 0, aio_data->read_cache.len);
        }
        if (aio_data->write_cache.len > 0) {
            ___async_io_cache_shift(aio_data, &(aio_data->write_cache), 0, aio_data->write_cache.len);
        }
        if (aio_data->ssl) {
            openssl_SSL_free(aio_data->ssl);
            aio_data->ssl = 0;
        }

        memset(&___data, 0, sizeof(___data));
        aio_data->fd = -1;
    }
}
/* }}} */

/* {{{ async_io::bind, option_local, get_result, get_fd, context, get_cache_size, get_event_base */
void async_io::bind(int fd, event_base &eb)
{
    async_io_t * aio_data = (async_io_t *)___data;
    aio_data->aio_type = var_event_type_aio;
    aio_data->fd = fd;
    aio_data->evbase = &eb;
}

void async_io::option_local()
{
    async_io_t * aio_data = (async_io_t *)___data;
    aio_data->is_local = 1;
}

int async_io::get_result()
{
    async_io_t * aio_data = (async_io_t *)___data;
    return aio_data->ret;
}

int async_io::get_fd()
{
    async_io_t * aio_data = (async_io_t *)___data;
    return aio_data->fd;
}

void async_io::set_context(const void *ctx)
{
    async_io_t * aio_data = (async_io_t *)___data;
    aio_data->context = const_cast<void *>(ctx);
}

size_t async_io::get_cache_size()
{
    async_io_t * aio_data = (async_io_t *)___data;
    return aio_data->write_cache.len;
}

void *async_io::get_context()
{
    async_io_t * aio_data = (async_io_t *)___data;
    return aio_data->context;
}
/* }}} */

/* {{{  async_io::tls_connect, tls_accept */
void async_io::tls_connect(SSL_CTX * ctx, async_io_cb_t callback, long timeout)
{
    async_io_t * aio_data = (async_io_t *)___data;
    async_io_ssl_init(aio_data, ctx, callback, timeout, false);
}

void async_io::tls_accept(SSL_CTX * ctx, async_io_cb_t callback, long timeout)
{
    async_io_t * aio_data = (async_io_t *)___data;
    async_io_ssl_init(aio_data, ctx, callback, timeout, true);
}

event_base &async_io::get_event_base()
{
    async_io_t * aio_data = (async_io_t *)___data;
    return *(aio_data->evbase);
}
/* }}} */

/* {{{ async_io::detach_SSL */
SSL *async_io::detach_SSL()
{
    async_io_t * aio_data = (async_io_t *)___data;
    SSL *r = aio_data->ssl;
    aio_data->ssl = 0;
    aio_data->ssl_server_or_client = 0;
    aio_data->ssl_session_init = 0;
    aio_data->ssl_read_want_read = 0;
    aio_data->ssl_read_want_write = 0;
    aio_data->ssl_write_want_read = 0;
    aio_data->ssl_write_want_write = 0;
    aio_data->ssl_error_or_closed = 0;
    return r;
}
/* }}} */

/* {{{ async_io::fetch_rbuf */
void async_io::fetch_rbuf(char *buf, int len)
{
    async_io_t * aio_data = (async_io_t *)___data;
    ___async_io_cache_shift(aio_data, &(aio_data->read_cache), buf, len);
}

void async_io::fetch_rbuf(std::string &dest, int len)
{
    async_io_t * aio_data = (async_io_t *)___data;
    dest.clear();
    dest.append(len+1, 0);
    char *buf = const_cast<char *>(dest.c_str());
    ___async_io_cache_shift(aio_data, &(aio_data->read_cache), buf, len);
    dest.resize(len);
}
/* }}} */

/* {{{ async_io::read */
void async_io::read(size_t max_len, async_io_cb_t callback, long timeout)
{
    async_io_t * aio_data = (async_io_t *)___data;
    event_base *eb = aio_data->evbase;
    event_base_t *eb_data = event_base_get_data(eb);

    aio_data->action_type = var_async_io_type_read;
    aio_data->callback = callback;
    aio_data->read_magic_len = max_len;
    aio_data->ret = timeout;

    lock_evbase(eb_data);
    async_io_append_queue(eb_data, aio_data);
    unlock_evbase(eb_data);
    if (eb_data->plocker_flag) {
        eb->notify();
    }
}

void async_io::readn(size_t strict_len, async_io_cb_t callback, long timeout)
{
    async_io_t * aio_data = (async_io_t *)___data;
    event_base *eb = aio_data->evbase;
    event_base_t *eb_data = event_base_get_data(eb);

    aio_data->action_type = var_async_io_type_read_n;
    aio_data->callback = callback;
    aio_data->read_magic_len = strict_len;
    aio_data->ret = timeout;

    lock_evbase(eb_data);
    async_io_append_queue(eb_data, aio_data);
    unlock_evbase(eb_data);
    if (eb_data->plocker_flag) {
        eb->notify();
    }
}

void async_io::read_size_data(async_io_cb_t callback, long timeout)
{
    async_io_t * aio_data = (async_io_t *)___data;
    event_base *eb = aio_data->evbase;
    event_base_t *eb_data = event_base_get_data(eb);

    aio_data->delimiter = 0;
    aio_data->action_type = var_async_io_type_read_size_data;
    aio_data->callback = callback;
    aio_data->read_magic_len = 0;
    aio_data->ret = timeout;

    lock_evbase(eb_data);
    async_io_append_queue(eb_data, aio_data);
    unlock_evbase(eb_data);
    if (eb_data->plocker_flag) {
        eb->notify();
    }
}

void async_io::read_delimiter(int delimiter, size_t max_len, async_io_cb_t callback, long timeout)
{
    async_io_t * aio_data = (async_io_t *)___data;
    event_base *eb = aio_data->evbase;
    event_base_t *eb_data = event_base_get_data(eb);

    aio_data->action_type = var_async_io_type_read_delimeter;
    aio_data->delimiter = delimiter;
    aio_data->read_magic_len = max_len;
    aio_data->callback = callback;
    aio_data->ret = timeout;

    lock_evbase(eb_data);
    async_io_append_queue(eb_data, aio_data);
    unlock_evbase(eb_data);
    if (eb_data->plocker_flag) {
        eb->notify();
    }
}

void async_io::read_line(size_t max_len, void (*callback)(async_io &), long timeout)
{
    read_delimiter('\n', max_len, callback, timeout);
}
/* }}} */

/* {{{ async_io::cache_write_do */
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
    async_io_t * aio_data = (async_io_t *)___data;
    ___async_io_cache_append(aio_data, &(aio_data->write_cache), buf, len);
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

    if (len < 1) {
        return;
    }
    async_io_t * aio_data = (async_io_t *)___data;
 
    rwb = (aio_rwbuf_t *) calloc(1, sizeof(aio_rwbuf_t));
    rwb->next = 0;
    rwb->long_flag = 1;
    rwb->p1 = 0;
    rwb->p2 = 0;

    aio_rwbuf_longbuf_t *lb = (aio_rwbuf_longbuf_t *)(rwb->data);
    lb->data = (char *)buf;
    lb->p1 = 0;
    lb->p2 = len - 1;

    if (aio_data->write_cache.head) {
        aio_data->write_cache.head->next = rwb;
        aio_data->write_cache.tail = rwb;
    } else {
        aio_data->write_cache.head = rwb;
        aio_data->write_cache.tail = rwb;
    }
}

void async_io::cache_flush(async_io_cb_t callback, long timeout)
{
    async_io_t * aio_data = (async_io_t *)___data;
    event_base *eb = aio_data->evbase;
    event_base_t *eb_data = event_base_get_data(eb);

    aio_data->action_type = var_async_io_type_write;
    aio_data->callback = callback;
    aio_data->ret = timeout;

    lock_evbase(eb_data);
    async_io_append_queue(eb_data, aio_data);
    unlock_evbase(eb_data);
    if (eb_data->plocker_flag) {
        eb->notify();
    }
}
/* }}} */

/* {{{ async_io::sleep */
void async_io::sleep(async_io_cb_t callback, long timeout)
{
    async_io_t * aio_data = (async_io_t *)___data;
    event_base *eb = aio_data->evbase;
    event_base_t *eb_data = event_base_get_data(eb);

    aio_data->action_type = var_async_io_type_sleep;
    aio_data->callback = callback;
    aio_data->ret = timeout;

    lock_evbase(eb_data);
    async_io_append_queue(eb_data, aio_data);
    unlock_evbase(eb_data);
    if (eb_data->plocker_flag) {
        eb->notify();
    }
}
/* }}} */

/* {{{ async_io_queue_checker */
static inline void async_io_queue_checker(event_base_t * eb_data)
{
    async_io_t *aio_data;
    rbtree_node_t *rn;

    if (!eb_data->queue_head) {
        return;
    }

    while (1) {
        if (var_proc_stop) {
            break;
        }
        lock_evbase(eb_data);
        rn = eb_data->queue_head;
        if (!rn) {
            unlock_evbase(eb_data);
            return;
        }
        aio_data = zcc_container_of(rn, async_io_t, rbnode_time);
        zcc_mlink_detach(eb_data->queue_head, eb_data->queue_tail, rn, rbtree_left, rbtree_right);
        unlock_evbase(eb_data);
        async_io_action(aio_data);
    }
}
/* }}} */

/* {{{ async_io_timeout_cmp */
static int async_io_timeout_cmp(rbtree_node_t * n1, rbtree_node_t * n2)
{
    async_io_t *e1, *e2;
    int r;

    e1 = zcc_container_of(n1, async_io_t, rbnode_time);
    e2 = zcc_container_of(n2, async_io_t, rbnode_time);
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
/* }}} */

/* {{{ async_io_timeout_check */
long async_io_timeout_check(event_base_t * eb_data)
{
    async_io_t *aio_data;
    async_io_cb_t callback;
    rbtree_node_t *rn;
    long delay = 1 * 1000;

    if (!rbtree_have_data(&(eb_data->async_io_timeout_tree))) {
        return delay;
    }

    while (1) {
        if (var_proc_stop) {
            break;
        }
        lock_evbase(eb_data);
        rn = rbtree_first(&(eb_data->async_io_timeout_tree));
        if (!rn) {
            unlock_evbase(eb_data);
            return 1 * 1000;
        }
        aio_data = zcc_container_of(rn, async_io_t, rbnode_time);
        delay = timeout_left(aio_data->timeout);
        if (delay > 0) {
            unlock_evbase(eb_data);
            return delay;
        }
        callback = aio_data->callback;
        aio_data->revents = 0;
        aio_data->ret = -2;
        if (aio_data->action_type == var_async_io_type_sleep) {
            aio_data->ret = 1;
        }
        rbtree_detach(&(eb_data->async_io_timeout_tree), rn);
        aio_data->in_timeout = 0;
        unlock_evbase(eb_data);

        if (callback) {
            (callback) (*((async_io *)(aio_data)));
        } else {
            zcc_fatal("aio_data: not found callback");
        }
    }
    return 1 * 1000;
}
/* }}} */

/* {{{ async_io_list_do for others */
void async_io_list_append(async_io **list_head, async_io **list_tail, async_io *aio)
{
    rbtree_node_t *lh = (rbtree_node_t *)(*list_head), *lt = (rbtree_node_t *)(*list_tail);
    async_io_t *aio_data = async_io_get_data(aio);
    rbtree_node_t *a = &(aio_data->rbnode_time);
    zcc_mlink_append(lh, lt, a, rbtree_left, rbtree_right);
    *list_head = (async_io *)lh;
    *list_tail = (async_io *)lt;
}

void async_io_list_detach(async_io **list_head, async_io **list_tail, async_io *aio)
{
    rbtree_node_t *lh = (rbtree_node_t *)(*list_head), *lt = (rbtree_node_t *)(*list_tail);
    async_io_t *aio_data = async_io_get_data(aio);
    rbtree_node_t *a = &(aio_data->rbnode_time);
    zcc_mlink_detach(lh, lt, a, rbtree_left, rbtree_right);
    *list_head = (async_io *)lh;
    *list_tail = (async_io *)lt;
}
/* }}} */

/* {{{ event_base */
static void evbase_notify_reader(event_io &eio)
{
    uint64_t u;
    syscall_read(eio.get_fd(), &u, sizeof(uint64_t));
}

event_base::event_base()
{
    event_base_t *eb_data = (event_base_t *)___data;
    int eventfd_fd;

    memset(eb_data, 0, sizeof(___data));

    rbtree_init(&(eb_data->event_timer_timeout_tree), event_timer_timeout_cmp);
    rbtree_init(&(eb_data->async_io_timeout_tree), async_io_timeout_cmp);

    eb_data->epoll_fd = epoll_create(1024);
    close_on_exec(eb_data->epoll_fd, true);
    eb_data->epool_event_vector = (struct epoll_event *)malloc(sizeof(struct epoll_event) * var_epoll_vecotr_count);

    eventfd_fd = eventfd(0, 0);
    close_on_exec(eventfd_fd, true);
    nonblocking(eventfd_fd, 1);

    eb_data->eventfd_event = new event_io();
    eb_data->eventfd_event->bind(eventfd_fd, *this);
    eb_data->eventfd_event->enable_read(evbase_notify_reader);

    pthread_mutex_init(&(eb_data->plocker), 0);
}

event_base::~event_base()
{
    event_base_t *eb_data = (event_base_t *)___data;
    int eventfd_fd = eb_data->eventfd_event->get_fd();
    delete eb_data->eventfd_event;
    close(eventfd_fd);
    close(eb_data->epoll_fd);
    free(eb_data->epool_event_vector);
    pthread_mutex_destroy(&(eb_data->plocker));
    eb_data->plocker_flag = true;
}

void event_base::notify()
{
    uint64_t u = 1;
    event_base_t *eb_data = (event_base_t *)___data;
    syscall_write(eb_data->eventfd_event->get_fd(), &u, sizeof(uint64_t));
}

void event_base::option_local()
{
    event_base_t *eb_data = (event_base_t *)___data;
    eb_data->plocker_flag = false;
}

void event_base::dispatch(long default_delay)
{
    event_base_t *eb_data = (event_base_t *)___data;
    int nfds;
    struct epoll_event *epev;
    event_io_t *eio_data;
    async_io_t *aio_data;
    long delay = 1 * 1000, delay_tmp;


    if (eb_data->queue_head) {
        async_io_queue_checker(eb_data);
    }

    if (rbtree_have_data(&(eb_data->event_timer_timeout_tree))) {
        delay_tmp = event_timer_timeout_check(eb_data);
        if (delay_tmp < delay) {
            delay = delay_tmp;
        }
    }
    if (rbtree_have_data(&(eb_data->async_io_timeout_tree))) {
        delay_tmp = async_io_timeout_check(eb_data);
        if (delay_tmp < delay) {
            delay = delay_tmp;
        }
    }

    if (default_delay < 1) {
        default_delay = 10;
    }
    if (delay > default_delay) {
        delay = default_delay;
    }
    nfds = epoll_wait(eb_data->epoll_fd, eb_data->epool_event_vector, var_epoll_vecotr_count, delay);
    if (nfds == -1) {
        if (errno != EINTR) {
            zcc_fatal("event_base::dispath: epoll_wait: %m");
        }
        return;
    }

    for (int i = 0; i < nfds; i++) {
        if (var_proc_stop) {
            return;
        }
        epev = eb_data->epool_event_vector + i;
        unsigned int events = epev->events;
        unsigned char revents = 0;
        if (events & EPOLLHUP) {
            revents |= var_event_hup;
        }
        if (events & EPOLLRDHUP) {
            revents |= var_event_rdhup;
        }
        if (events & EPOLLERR) {
            revents |= var_event_error;
        }
        if (events & EPOLLIN) {
            revents |= var_event_read;
        }
        if (events & EPOLLOUT) {
            revents |= var_event_write;
        }
        aio_data = (async_io_t *) (epev->data.ptr);
        if (aio_data->aio_type == var_event_type_event) {
            eio_data = (event_io_t *) aio_data;
            eio_data->revents = revents;
            event_io_cb_t callback = eio_data->callback;

            if (callback) {
                (callback) (*((event_io *)(eio_data)));
            } else {
                zcc_fatal("event_io: not found callback");
            }

        } else if (1 || (aio_data->aio_type == var_event_type_aio)) {
            aio_data->revents = revents;
            async_io_action(aio_data);
        }
    }
    return;
}

/* }}} */

#pragma pack(pop)

}

#ifdef __ZCC_SIZEOF_PROBE__
int main()
{ 
    _ZCC_SIZEOF_DEBUG(zcc::event_timer, zcc::event_timer_t);
    _ZCC_SIZEOF_DEBUG(zcc::event_io, zcc::event_io_t);
    _ZCC_SIZEOF_DEBUG(zcc::async_io, zcc::async_io_t);
    _ZCC_SIZEOF_DEBUG(zcc::event_base, zcc::event_base_t);
    return 0;
}
#endif

/* Local variables:
* End:
* vim600: fdm=marker
*/
