/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2016-11-30
 * ================================
 */

#include "zcc.h"
#include <poll.h>

namespace zcc
{

/* read */
/* readable means: 1, have readable data.
 *                 2, peer closed.
 * when receive POLLRDHUP, maybe have some readable data.
 */
int timed_wait_readable(int fd, long timeout)
{
    struct pollfd pollfd;
    long critical_time, left_time;

    if (timeout < 1) {
        timeout = var_max_timeout;
    }
    critical_time = timeout_set(timeout);
    for (;;) {
        left_time = timeout_left(critical_time);
        if (left_time < 0) {
            return 0;
        }
        bool ccc = false;
        if (left_time > 1000 * 3600) {
            left_time = 1000 * 3600;
            ccc = true;
        }
        pollfd.fd = fd;
        pollfd.events = POLLIN;
        switch (poll(&pollfd, 1, left_time)) {
        case -1:
            if (errno != EINTR) {
                zcc_fatal("poll error (%m)");
            }
            continue;
        case 0:
            if (ccc) {
                continue;
            }
            return 0;
        default:
            if (pollfd.revents & POLLIN) {
                return 1;
            }
            if (pollfd.revents & (POLLNVAL | POLLERR | POLLHUP)) {
                return -1;
            }
            if (pollfd.revents & POLLRDHUP) {
                return -1;
            }
            return -1;
        }
    }

    return 1;
}

ssize_t timed_read(int fd, void *buf, size_t size, long timeout)
{
    ssize_t ret;
    long critical_time, left_time;

    if (timeout < 1) {
        timeout = var_max_timeout;
    }
    critical_time = timeout_set(timeout);

    for (;;) {
        left_time = timeout_left(critical_time);
        if (left_time < 1) {
            return -1;
        }
        if (timed_wait_readable(fd, left_time) < 1) {
            return (-1);
        }
        if ((ret = read(fd, buf, size)) < 0) {
            int ec = errno;
            if (ec == EINTR) {
                continue;
            }
            if (ec == EAGAIN) {
                continue;
            }
            return -1;
        }
        return (ret);
    }

    return 0;
}

ssize_t timed_readn(int fd, void *buf, size_t size, long timeout)
{
    bool is_closed = false;
    ssize_t ret;
    size_t left;
    char *ptr;
    long critical_time, left_time;

    left = size;
    ptr = (char *)buf;

    if (timeout < 1) {
        timeout = var_max_timeout;
    }
    critical_time = timeout_set(timeout);

    while (left > 0) {
        left_time = timeout_left(critical_time);
        if (left_time < 1) {
            break;
        }
        if (timed_wait_readable(fd, left_time) < 1) {
            break;
        }
        ret = read(fd, ptr, left);
        if (ret < 0) {
            int ec = errno;
            if (ec == EINTR) {
                continue;
            }
            if (ec == EAGAIN) {
                continue;
            }
            break;
        } else if (ret == 0) {
            is_closed = true;
            break;
        } else {
            left -= ret;
            ptr += ret;
        }
    }

    if (size > left) {
        return size - left;
    }
    if (is_closed) {
        return 0;
    }
    return -1;
}

/* write */
int timed_wait_writeable(int fd, long timeout)
{
    struct pollfd pollfd;
    long critical_time, left_time;

    if (timeout < 1) {
        timeout = var_max_timeout;
    }
    critical_time = timeout_set(timeout);
    for (;;) {
        left_time = timeout_left(critical_time);
        if (left_time < 0) {
            return 0;
        }
        bool ccc = false;
        if (left_time > 1000 * 3600) {
            left_time = 1000 * 3600;
            ccc = true;
        }

        pollfd.fd = fd;
        pollfd.events = POLLOUT;
        switch (poll(&pollfd, 1, left_time)) {
        case -1:
            if (errno != EINTR) {
                zcc_fatal("poll error (%m)");
            }
            continue;
        case 0:
            if (ccc) {
                continue;
            }
            return 0;
        default:
            if (pollfd.revents & POLLOUT) {
                return 1;
            }
            return -1;
        }
    }

    return 0;
}

ssize_t timed_write(int fd, const void *buf, size_t size, long timeout)
{
    ssize_t ret;
    long critical_time, left_time;

    if (timeout < 1) {
        timeout = var_max_timeout;
    }
    critical_time = timeout_set(timeout);

    for (;;) {
        left_time = timeout_left(critical_time);
        if (left_time < 1) {
            return -1;
        }
        if (timed_wait_writeable(fd, left_time) < 1) {
            return -1;
        }
        if ((ret = write(fd, buf, size)) < 0) {
            int ec = errno;
            if (ec == EINTR) {
                continue;
            }
            if (ec == EAGAIN) {
                continue;
            }
            if (ec == EPIPE) {
                return 0;
            }
            return -1;
        } else if (ret == 0) {
            continue;
        }
        return (ret);
    }

    return 0;
}

ssize_t timed_writen(int fd, const void *buf, size_t size, long timeout)
{
    bool is_closed = false;
    ssize_t ret;
    size_t left;
    char *ptr;
    long critical_time;
    long left_time;

    if (timeout < 1) {
        timeout = var_max_timeout;
    }
    critical_time = timeout_set(timeout);

    left = size;
    ptr = (char *)buf;

    while (left > 0) {
        left_time = timeout_left(critical_time);
        if (left_time < 1) {
            break;
        }
        if (timed_wait_writeable(fd, left_time) < 1) {
            break;
        }
        ret = write(fd, ptr, left);
        if (ret < 0) {
            int ec = errno;
            if (ec == EINTR) {
                continue;
            }
            if (ec == EAGAIN) {
                continue;
            }
            if (ec == EPIPE) {
                is_closed = true;
                break;
            }
            break;
        } else if (ret == 0) {
            continue;
        } else {
            left -= ret;
            ptr += ret;
        }
    }

    if (size > left) {
        return size - left;
    }
    if (is_closed) {
        return 0;
    }
    return -1;
}

}

