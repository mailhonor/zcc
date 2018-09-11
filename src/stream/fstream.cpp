/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2016-11-30
 * ================================
 */

#include "zcc.h"
#include <fcntl.h>
#include <poll.h>

namespace zcc
{

fstream::fstream()
{
    init();
}

fstream::fstream(int fd, bool auto_release)
{
    init();
    if (fd > -1) {
        open(fd, auto_release);
    } else {
        ___error = true;
    }
}

fstream::fstream(const char *path, const char *mode)
{
    init();
    open(path, mode);
}

fstream::~fstream()
{
    close();
    fini();
}

bool fstream::open(const char *pathname, const char *mode)
{
    close();
    int flags;
    if (*mode == 'r') {
        flags = O_RDONLY;
        if (mode[1] == '+') {
            flags = O_RDWR;
        }
    } else  if (*mode == 'w') {
        flags = O_WRONLY|O_TRUNC|O_CREAT;
        if (mode[1] == '+') {
            flags = O_RDWR|O_TRUNC|O_CREAT;
        }
    } else  if (*mode == 'a') {
        flags = O_WRONLY|O_TRUNC|O_CREAT|O_APPEND;
        if (mode[1] == '+') {
            flags = O_RDWR|O_TRUNC|O_CREAT|O_APPEND;
        }
    } else {
        flags = O_RDONLY;
    }
    ___fd = ::open(pathname, flags, 0666);
    if (___fd == -1) {
        ___error = true;
        return false;
    }
    ___auto_release = true;
    ___opened = true;
    return true;
}

fstream &fstream::open(int fd, bool auto_release)
{
    close();
    ___fd = fd;
    ___auto_release = auto_release;
    ___opened = true;
    return *this;
}

fstream &fstream::close()
{
    if (!___opened) {
        return *this;
    }
    flush();
    if (___auto_release) {
        ::close(___fd);
    }
    basic_stream::reset();
    ___auto_release = false;
    ___fd = -1;

    return *this;
}

int fstream::get_char_do()
{
    if (!___opened) {
        return -1;
    }
    int ret;

    if (read_buf_p1 < read_buf_p2) {
        return read_buf[read_buf_p1++];
    }

    if (___error || ___eof) {
        return -1;
    }

    if (write_buf_len > 0) {
        flush();
    }

    if (___error || ___eof) {
        return -1;
    }

    read_buf_p1 = read_buf_p2 = 0;
    while(1) {
        struct pollfd pollfd;
        pollfd.fd = ___fd;
        pollfd.events = POLLIN;
        switch (poll(&pollfd, 1, 24 * 3600 * 1000)) {
        case -1:
            if (errno != EINTR) {
                zcc_fatal("poll error (%m)");
            }
            continue;
        case 0:
            continue;
        default:
            if (pollfd.revents & POLLIN) {
                break;
            }
            ___error = true;
            return -1;
        }

        if ((ret = ::read(___fd, read_buf, stream_read_buf_size)) < 0) {
            int ec = errno;
            if (ec == EINTR) {
                continue;
            }
            if (ec == EAGAIN) {
                continue;
            }
            return -1;
        } else if (ret == 0) {
            ___eof = true;
            return -1;
        }
        read_buf_p2 = ret;
        break;
    }

    return read_buf[read_buf_p1++];
}

bool fstream::flush()
{
    if (!___opened) {
        return false;
    }
    ssize_t ret;
    char *data = write_buf;
    size_t data_len =write_buf_len;
    size_t wlen = 0;

    ___flushed = true;

    if (___error) {
        return false;
    }
    if (write_buf_len < 1) {
        return true;
    }

    struct pollfd pollfd;
    pollfd.fd = ___fd;
    pollfd.events = POLLOUT;
    while (wlen < data_len) {
        switch (poll(&pollfd, 1, 24 * 3600 * 1000)) {
        case -1:
            if (errno != EINTR) {
                zcc_fatal("poll error (%m)");
            }
            continue;
        case 0:
            continue;
        default:
            if (pollfd.revents & POLLOUT) {
                break;
            }
            ___error = true;
            return false;
        }

        if ((ret = ::write(___fd, data + wlen, data_len - wlen)) < 0) {
            int ec = errno;
            if (ec == EINTR) {
                continue;
            }
            if (ec == EAGAIN) {
                continue;
            }
            ___error = true;
            return false;
        } else if (ret == 0) {
            continue;
        }

        wlen += ret;
    }
    write_buf_len = 0;
    return true;
}

void fstream::init()
{
    ___auto_release = false;
    ___fd = -1;
}

void fstream::fini()
{
}

}
