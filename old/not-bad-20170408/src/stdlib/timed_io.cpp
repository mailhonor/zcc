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

/* read */
bool wait_readable(int fd, long timeout)
{
    struct pollfd pollfd;

    pollfd.fd = fd;
    pollfd.events = POLLIN;

    for (;;) {
        switch (poll(&pollfd, 1, timeout)) {
        case -1:
            if (errno != EINTR) {
                return false;
            }
            continue;
        case 0:
            /* errno = ETIMEDOUT; */
            return false;
        default:
            if (pollfd.revents & (POLLNVAL | POLLERR | POLLHUP | POLLRDHUP)) {
                return false;
            }
            return true;
        }
    }

    return true;
}

ssize_t timed_read(int fd, void *buf, size_t size, long timeout)
{
    ssize_t ret;
    long critical_time;
    long left_time;

    critical_time = timeout_set(timeout);

    for (;;) {
        left_time = timeout_left(critical_time);
        if (left_time < 1) {
            return -1;
        }
        if (!(ret = wait_readable(fd, left_time))) {
            return (-1);
        }
        if ((ret = read(fd, buf, size)) < 0) {
            if (errno == EINTR) {
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
    ssize_t ret;
    size_t left;
    char *ptr;
    long critical_time;
    long left_time;

    left = size;
    ptr = (char *)buf;

    critical_time = timeout_set(timeout);

    while (left > 0) {
        left_time = timeout_left(critical_time);
        if (left_time < 1) {
            return -1;
        }
        if (!(wait_readable(fd, left_time))) {
            return -1;
        }
        ret = read(fd, ptr, left);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        } else if (ret == 0) {
            return -1;
        } else {
            left -= ret;
            ptr += ret;
        }
    }

    return size;
}

/* write */
bool wait_writeable(int fd, long timeout)
{
    struct pollfd pollfd;

    pollfd.fd = fd;
    pollfd.events = POLLOUT;

    for (;;) {
        switch (poll(&pollfd, 1, timeout)) {
        case -1:
            if (errno != EINTR) {
                return false;
            }
            continue;
        case 0:
            /* errno = ETIMEDOUT; */
            return false;
        default:
            if (pollfd.revents & (POLLNVAL | POLLERR | POLLHUP | POLLRDHUP)) {
                return false;
            }
            return true;
        }
    }

    return true;
}

ssize_t timed_write(int fd, const void *buf, size_t size, long timeout)
{
    size_t ret;
    long critical_time;
    long left_time;

    critical_time = timeout_set(timeout);

    for (;;) {
        left_time = timeout_left(critical_time);
        if (left_time < 1) {
            return -1;
        }
        if (!(wait_writeable(fd, left_time))) {
            return -1;
        }
        if ((ret = write(fd, buf, size)) < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        return (ret);
    }

    return 0;
}

ssize_t timed_writen(int fd, const void *buf, size_t size, long timeout)
{
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
            return -1;
        }
        if ((ret = wait_writeable(fd, left_time)) < 0) {
            return -1;
        }
        ret = write(fd, ptr, left);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        } else if (ret == 0) {
            return -1;
        } else {
            left -= ret;
            ptr += ret;
        }
    }

    return size;
}
}

