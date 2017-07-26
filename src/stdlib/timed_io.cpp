/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-11-30
 * ================================
 */

#include "zcc.h"
#include <poll.h>

namespace zcc
{

ssize_t syscall_read(int fildes, void *buf, size_t nbyte);
ssize_t syscall_write(int fildes, const void *buf, size_t nbyte);

/* read */
/* readable means: 1, have readable data.
 *                 2, peer closed.
 * when receive POLLRDHUP, maybe have some readable data.
 */
bool timed_wait_readable(int fd, long timeout, bool *error)
{
    struct pollfd pollfd;
    long critical_time, left_time;

    pollfd.fd = fd;
    pollfd.events = POLLIN;

    if (error) {
        *error = false;
    }

    critical_time = timeout_set(timeout);
    for (;;) {
        if (timeout == -1) {
            left_time = 1000 * 3600;
        } else if (timeout == 0) {
            left_time = 0;
        } else {
            left_time = timeout_left(critical_time);
        }
        if (left_time < 0) {
            return false;
        }
        switch (poll(&pollfd, 1, left_time)) {
        case -1:
            if (errno != EINTR) {
                zcc_fatal("poll error (%m)");
            }
            continue;
        case 0:
            return false;
        default:
            if (pollfd.revents & POLLIN) {
                return true;
            }
            if (pollfd.revents & (POLLNVAL | POLLERR | POLLHUP)) {
                if (error) {
                    *error = true;
                }
                return false;
            }
            if (pollfd.revents & POLLRDHUP) {
                if (error) {
                    *error = true;
                }
                return false;
            }
            if (error) {
                *error = true;
            }
            return false;
        }
    }

    return true;
}

ssize_t timed_read(int fd, void *buf, size_t size, long timeout)
{
    ssize_t ret;
    long critical_time, left_time;

    critical_time = timeout_set(timeout);

    for (;;) {
        left_time = timeout_left(critical_time);
        if (left_time < 1) {
            return -1;
        }
        if (!timed_wait_readable(fd, left_time)) {
            return (-1);
        }
        if ((ret = syscall_read(fd, buf, size)) < 0) {
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

    critical_time = timeout_set(timeout);

    while (left > 0) {
        left_time = timeout_left(critical_time);
        if (left_time < 1) {
            break;
        }
        if (!(timed_wait_readable(fd, left_time))) {
            break;
        }
        ret = syscall_read(fd, ptr, left);
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
bool timed_wait_writeable(int fd, long timeout, bool *error)
{
    struct pollfd pollfd;
    long critical_time, left_time;

    pollfd.fd = fd;
    pollfd.events = POLLOUT;
    
    if (error) {
        *error = false;
    }

    critical_time = timeout_set(timeout);
    for (;;) {
        if (timeout == -1) {
            left_time = 1000 * 3600;
        } else if (timeout == 0) {
            left_time = 0;
        } else {
            left_time = timeout_left(critical_time);
        }
        if (left_time < 0) {
            return false;
        }
        switch (poll(&pollfd, 1, left_time)) {
        case -1:
            if (errno != EINTR) {
                zcc_fatal("poll error (%m)");
            }
            continue;
        case 0:
            return false;
        default:
            if (pollfd.revents & POLLOUT) {
                return true;
            }
            return false;
            if (pollfd.revents & (POLLNVAL | POLLERR | POLLHUP)) {
                if (error) {
                    *error = true;
                }
                return false;
            }
            if (pollfd.revents & POLLRDHUP) {
                if (error) {
                    *error = true;
                }
                return false;
            }
            if (error) {
                *error = true;
            }
            return false;
        }
    }

    return true;
}

ssize_t timed_write(int fd, const void *buf, size_t size, long timeout)
{
    size_t ret;
    long critical_time, left_time;

    critical_time = timeout_set(timeout);

    for (;;) {
        left_time = timeout_left(critical_time);
        if (left_time < 1) {
            return -1;
        }
        if (!(timed_wait_writeable(fd, left_time))) {
            return -1;
        }
        if ((ret = syscall_write(fd, buf, size)) < 0) {
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
    size_t ret;
    size_t left;
    char *ptr;
    long critical_time;
    long left_time;

    critical_time = timeout_set(timeout);

    left = size;
    ptr = (char *)buf;

    while (left > 0) {
        left_time = timeout_left(critical_time);
        if (left_time < 1) {
            break;
        }
        if ((ret = timed_wait_writeable(fd, left_time)) < 0) {
            break;
        }
        ret = syscall_write(fd, ptr, left);
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

