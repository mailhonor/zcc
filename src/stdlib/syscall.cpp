/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-06-26
 * ================================
 */
#include "zcc.h"
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <resolv.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

/* create fd by open,create,pipe,dup,dup2,fcntl,eventfd,signalfd,socketpair */

namespace zcc
{

/* io */
int syscall_pipe(int pipefd[2])
{
    return syscall(__NR_pipe, pipefd);
}

int syscall_pipe2(int pipefd[2], int flags)
{
    return syscall(__NR_pipe2, pipefd, flags);
}

int syscall_dup(int oldfd)
{
    return syscall(__NR_dup, oldfd);
}

int syscall_dup2(int oldfd, int newfd)
{
    return syscall(__NR_dup2, oldfd, newfd);
}

int syscall_dup3(int oldfd, int newfd, int flags)
{
    return syscall(__NR_dup3, oldfd, newfd, flags);
}

int syscall_socketpair(int domain, int type, int protocol, int sv[2])
{
    return syscall(__NR_socketpair, domain, type, protocol, sv);
}

int syscall_socket(int domain, int type, int protocol)
{
    return syscall(__NR_socket, domain, type, protocol);
}

int syscall_accept(int fd, struct sockaddr *addr, socklen_t *len)
{
    return syscall(__NR_accept, fd, addr, len);
}

int syscall_connect(int socket, const struct sockaddr *address, socklen_t address_len)
{
    return syscall(__NR_connect, socket, address, address_len);
}

int syscall_close(int fd)
{
    return syscall(__NR_close, fd);
}

ssize_t syscall_read(int fildes, void *buf, size_t nbyte)
{
    return syscall(__NR_read, fildes, buf, nbyte);
}

ssize_t syscall_write(int fildes, const void *buf, size_t nbyte)
{
    return syscall(__NR_write, fildes, buf, nbyte);
}

ssize_t syscall_sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
    return syscall(__NR_sendto, socket, message, length, flags, dest_addr, dest_len);
}

ssize_t syscall_recvfrom(int socket, void *buffer, size_t length, int flags, struct sockaddr *address, socklen_t *address_len)
{
    return syscall(__NR_recvfrom, socket, buffer, length, flags, address, address_len);
}

size_t syscall_send(int socket, const void *buffer, size_t length, int flags)
{
    return syscall(__NR_sendto, socket, buffer, length, flags, 0, 0);
}

ssize_t syscall_recv(int socket, void *buffer, size_t length, int flags)
{
    return syscall(__NR_recvfrom, socket, buffer, length, flags, 0, 0);
}

int syscall_poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
    return syscall(__NR_poll, fds, nfds, timeout);
}

int syscall_setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len)
{
    return syscall(__NR_setsockopt, socket, level, option_name, option_value, option_len);
}

int syscall_fcntl(int fildes, int cmd, ...)
{
	int ret = -1;
	va_list args;
	va_start(args,cmd);
	switch(cmd)
	{
		case F_DUPFD:
		case F_SETFD:
		case F_SETFL:
		case F_SETOWN:
            {
                int param = va_arg(args,int);
                ret = syscall(__NR_fcntl, fildes, cmd, param);
                break;
            }
		case F_GETFD:
		case F_GETFL:
		case F_GETOWN:
            {
                ret = syscall(__NR_fcntl, fildes, cmd);
                break;
            }
		case F_GETLK:
		case F_SETLK:
		case F_SETLKW:
            {
                /* struct flock *param = va_arg(args,struct flock *); */
                void *param = va_arg(args, void *);
                ret = syscall(__NR_fcntl, fildes, cmd, param);
                break;
            }
	}
	va_end(args);
	return ret;
}

pid_t gettid(void)
{
    return syscall(__NR_gettid);
}

}
