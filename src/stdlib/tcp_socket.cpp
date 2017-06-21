/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-12-01
 * ================================
 */

#include "zcc.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

static int (*___zcc_accept)(int sockfd, struct sockaddr *addr, socklen_t *addrlen) = accept;
static int (*___zcc_listen)(int sockfd, int backlog) = listen;
static int (*___zcc_connect)(int sockfd, const struct sockaddr *addr, socklen_t addrlen) = connect;

namespace zcc
{

/* accept */

static int ___sane_accept(int sock, struct sockaddr *sa, socklen_t * len)
{
    static int accept_ok_errors[] = {
        EAGAIN,
        ECONNREFUSED,
        ECONNRESET,
        EHOSTDOWN,
        EHOSTUNREACH,
        EINTR,
        ENETDOWN,
        ENETUNREACH,
        ENOTCONN,
        EWOULDBLOCK,
        ENOBUFS,                /* HPUX11 */
        ECONNABORTED,
        0,
    };
    int count;
    int err;
    int fd;

    if ((fd = ___zcc_accept(sock, sa, len)) < 0) {
        for (count = 0; (err = accept_ok_errors[count]) != 0; count++) {
            if (errno == err) {
                errno = EAGAIN;
                break;
            }
        }
    } else if (sa && (sa->sa_family == AF_INET || sa->sa_family == AF_INET6)) {
        int on = 1;
        (void)setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof(on));
    }

    return (fd);
}

int unix_accept(int fd)
{
    return (___sane_accept(fd, (struct sockaddr *)0, (socklen_t *) 0));
}

int inet_accept(int fd)
{
    struct sockaddr_storage ss;
    socklen_t ss_len = sizeof(ss);

    return (___sane_accept(fd, (struct sockaddr *)&ss, &ss_len));
}

/* listen */
int unix_listen(char *addr, int backlog)
{
    struct sockaddr_un sun;
    int len = strlen(addr);
    int sock = -1;
    int errno2;

    if (len >= (int)sizeof(sun.sun_path)) {
        errno = ENAMETOOLONG;
        return -1;
    }

    memset((char *)&sun, 0, sizeof(struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    memcpy(sun.sun_path, addr, len + 1);

    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    if (unlink(addr) < 0 && errno != ENOENT) {
        zcc_fatal("unlink: %s(%m)", addr);
    }

    if (bind(sock, (struct sockaddr *)&sun, sizeof(struct sockaddr_un)) < 0) {
        goto err;
    }

    nonblocking(sock);
    if (___zcc_listen(sock, backlog) < 0) {
        goto err;
    }

    return (sock);

err:
    errno2 = errno;
    if (sock > -1) {
        close(sock);
    }
    errno = errno2;

    return -1;
}

int inet_listen(const char *sip, int port, int backlog)
{
    int sock;
    int on = 1;
    struct sockaddr_in addr;
    int errno2;
    struct linger linger;

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = (empty(sip) ? INADDR_ANY : inet_addr(sip));

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) {
        goto err;
    }

    linger.l_onoff = 0;
    linger.l_linger = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER, (char *)&linger, sizeof(linger)) < 0) {
        goto err;
    }

    if (bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
        goto err;
    }

    nonblocking(sock);
    if (___zcc_listen(sock, backlog) < 0) {
        goto err;
    }

    return (sock);

err:
    errno2 = errno;
    close(sock);
    errno = errno2;

    return -1;
}

int listen(const char *netpath, int backlog, int *type)
{
    char _netpath[1024], *path, *host, *p;
    int fd, port, tp;

    strncpy(_netpath, netpath, 1000);
    _netpath[1000] = 0;

    p = strchr(_netpath, ':');
    if (p) {
        *p++ = 0;
        if (!strcmp(_netpath, "inet")) {
            tp = var_tcp_listen_type_inet;
            host = p;
            p = strchr(host, ':');
            if (!p) {
                errno = EINVAL;
                return -1;
            } else {
                port = atoi(p+1);
            }
        } else if (!strcmp(_netpath, "unix")) {
            tp = var_tcp_listen_type_unix;
            path  = p;
        } else if (!strcmp(_netpath, "fifo")) {
            tp = var_tcp_listen_type_fifo;
            path  = p;
        } else {
            tp = var_tcp_listen_type_inet;
            host = _netpath;
            port = atoi(p);
        }
    } else {
        tp = var_tcp_listen_type_unix;
        path = _netpath;
    }
    if ((tp == var_tcp_listen_type_inet) && (port <1)){
        errno = EINVAL;
        return -1;
    }
    if (tp == var_tcp_listen_type_inet) {
        fd = inet_listen(host, port, backlog);
    } else if (tp == var_tcp_listen_type_unix) {
        fd = unix_listen(path, backlog);
    } else if (tp == var_tcp_listen_type_fifo) {
        fd = fifo_listen(path);
    }
    if (type) {
        *type = tp;
    }

    return fd;
}

/* fifo listen */
int fifo_listen(const char *path)
{
    int fd;
    int errno2;

    fd = -1;
    if ((mkfifo(path, 0666) < 0) && (errno != EEXIST)) {
        goto err;
    }
    if ((fd = open(path, O_RDWR | O_NONBLOCK, 0)) < 0) {
        goto err;
    }

    return (fd);

err:
    errno2 = errno;
    if (fd != -1) {
        close(fd);
    }
    errno = errno2;

    return -1;
}

/* connect */
static int ___sane_connect(int sock, struct sockaddr *sa, int len)
{
    if (sa->sa_family == AF_INET) {
        int on = 1;
        (void)setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof(on));
    }

    if ((___zcc_connect(sock, sa, len) < 0) && (errno != EINPROGRESS)) {
        return (-1);
    }

    return (0);
}

int unix_connect(const char *addr)
{
    struct sockaddr_un sun;
    int len = strlen(addr);
    int sock;
    int errno2;

    if (len >= (int)sizeof(sun.sun_path)) {
        errno = ENAMETOOLONG;
        return -1;
    }

    memset((char *)&sun, 0, sizeof(sun));
    sun.sun_family = AF_UNIX;
    memcpy(sun.sun_path, addr, len + 1);

    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        return (-1);
    }

    if (___sane_connect(sock, (struct sockaddr *)&sun, sizeof(sun)) < 0) {
        errno2 = errno;
        close(sock);
        errno = errno2;
        return (-1);
    }

    return (sock);
}

int inet_connect(const char *dip, int port)
{
    int sock;
    struct sockaddr_in addr;
    int errno2;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return (-1);
    }

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(dip);

    if (___sane_connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
        errno2 = errno;
        close(sock);
        errno = errno2;
        return (-1);
    }

    return (sock);
}

int host_connect(const char *host, int port)
{
    char ip_list[16 * 128];
    int sock, count, i;

    count = get_hostaddr(host, ip_list, 128);
    if (count < 1) {
        return -1;
    }
    for (i = 0; i < count; i++) {
        sock = inet_connect(ip_list + i*16, port);
        if (sock < 1) {
            return -1;
        }
        return sock;
    }

    return -1;
}

int connect(const char *netpath)
{
    char _netpath[1024 + 1];
    char *p;
    int port;
    int fd;

    fd = 0;
    strncpy(_netpath, netpath, 1000);
    _netpath[1024] = 0;

    p = strchr(_netpath, ':');
    if (p) {
        *p = 0;
        port = atoi(p + 1);
        fd = host_connect(_netpath, port);
    } else {
        fd = unix_connect(_netpath);
    }

    return fd;
}

}
