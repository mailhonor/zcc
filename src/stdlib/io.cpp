/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-11-29
 * ================================
 */

#include "zcc.h"
#include <poll.h>
#include <sys/file.h>
#include <sys/ioctl.h>

static inline int ___zcc_flock(int fd, int operation)
{
    return flock(fd, operation);
}

namespace zcc
{

bool is_rwable(int fd, bool *rable, bool *wable)
{
    struct pollfd pollfd;
    int flags = 0, revs;

    pollfd.fd = fd;
    if (rable) {
        flags |= POLLIN;
    }
    if (wable) {
        flags |= POLLOUT;
    }
    pollfd.events = flags;
    for (;;) {
        switch (poll(&pollfd, 1, 0)) {
        case -1:
            if (errno != EINTR) {
                return false;
            }
            continue;
        case 0:
            return false;
        default:
            revs = pollfd.revents;
            if (rable) {
                if (revs & POLLIN) {
                    *rable = true;
                } else {
                    *rable = false;
                }
            }
            if (wable) {
                if (revs & POLLOUT) {
                    *wable = true;
                } else {
                    *wable = false;
                }
            }
            return true;
            if (revs & POLLNVAL) {
                return false;
            }
            if (revs & (POLLERR | POLLHUP | POLLRDHUP)) {
                /* return false; */
            }
        }
    }

    return true;
}

bool is_readable(int fd, bool *rable)
{
    return is_rwable(fd, rable, 0);
}

bool is_writeable(int fd, bool *wable)
{
    return is_rwable(fd, 0, wable);
}

bool flock(int fd, int flags)
{
    int ret;
    while ((ret = ___zcc_flock(fd, flags)) < 0 && errno == EINTR)
        msleep(1);
    return (ret==0);
}

bool flock_share(int fd)
{
    int ret;
    while ((ret = ___zcc_flock(fd, LOCK_SH)) < 0 && errno == EINTR)
        msleep(1);
    return (ret==0);
}

bool flock_exclusive(int fd)
{
    int ret;
    while ((ret = ___zcc_flock(fd, LOCK_EX)) < 0 && errno == EINTR)
        msleep(1);
    return (ret==0);
}

bool funlock(int fd)
{
    int ret;
    while ((ret = ___zcc_flock(fd, LOCK_UN)) < 0 && errno == EINTR)
        msleep(1);
    return (ret==0);
}

bool nonblocking(int fd, bool no)
{
    int flags;

    if ((flags = fcntl(fd, F_GETFL, 0)) < 0) {
        return false;
    }

    if (fcntl(fd, F_SETFL, no ? flags | O_NONBLOCK : flags & ~O_NONBLOCK) < 0) {
        return false;
    }

    return ((flags & O_NONBLOCK) != 0);
}

bool close_on_exec(int fd, bool on)
{
    int flags;

    if ((flags = fcntl(fd, F_GETFD, 0)) < 0) {
        return false;
    }

    if (fcntl(fd, F_SETFD, on ? flags | FD_CLOEXEC : flags & ~FD_CLOEXEC) < 0) {
        return false;
    }

    return ((flags & FD_CLOEXEC) != 0);
}

int get_readable_count(int fd)
{
    int count;

    return (ioctl(fd, FIONREAD, (char *)&count) < 0 ? -1 : count);
}

ssize_t writen(int fd, const void *buf, size_t size)
{
    bool is_closed = false;
    ssize_t ret;
    size_t left;
    char *ptr;

    left = size;
    ptr = (char *)buf;

    while (left > 0) {
        if (timed_wait_writeable(fd, 3600 * 365 * 1000) < 1) {
            break;
        }
        ret = write(fd, ptr, left);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN) {
                continue;
            }
            if (errno == EPIPE) {
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
