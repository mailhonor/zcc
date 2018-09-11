/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
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
#include <dirent.h>
#include <utime.h>

namespace zcc
{

/* io */
int syscall_pipe(int pipefd[2])
{
    return ::syscall(__NR_pipe, pipefd);
}

int syscall_pipe2(int pipefd[2], int flags)
{
    return ::syscall(__NR_pipe2, pipefd, flags);
}

int syscall_dup(int oldfd)
{
    return ::syscall(__NR_dup, oldfd);
}

int syscall_dup2(int oldfd, int newfd)
{
    return ::syscall(__NR_dup2, oldfd, newfd);
}

int syscall_dup3(int oldfd, int newfd, int flags)
{
    return ::syscall(__NR_dup3, oldfd, newfd, flags);
}

int syscall_socketpair(int domain, int type, int protocol, int sv[2])
{
    return ::syscall(__NR_socketpair, domain, type, protocol, sv);
}

int syscall_socket(int domain, int type, int protocol)
{
    return ::syscall(__NR_socket, domain, type, protocol);
}

int syscall_accept(int fd, struct sockaddr *addr, socklen_t *len)
{
    return ::syscall(__NR_accept, fd, addr, len);
}

int syscall_connect(int socket, const struct sockaddr *address, socklen_t address_len)
{
    return ::syscall(__NR_connect, socket, address, address_len);
}

int syscall_close(int fd)
{
    return ::syscall(__NR_close, fd);
}

ssize_t syscall_read(int fildes, void *buf, size_t nbyte)
{
    return ::syscall(__NR_read, fildes, buf, nbyte);
}

ssize_t syscall_readv(int fd, const struct iovec *iov, int iovcnt)
{
    return ::syscall(__NR_readv, fd, iov, iovcnt);
}

ssize_t syscall_write(int fildes, const void *buf, size_t nbyte)
{
    return ::syscall(__NR_write, fildes, buf, nbyte);
}

ssize_t syscall_writev(int fd, const struct iovec *iov, int iovcnt)
{
    return ::syscall(__NR_writev, fd, iov, iovcnt);
}

ssize_t syscall_sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
    return ::syscall(__NR_sendto, socket, message, length, flags, dest_addr, dest_len);
}

ssize_t syscall_recvfrom(int socket, void *buffer, size_t length, int flags, struct sockaddr *address, socklen_t *address_len)
{
    return ::syscall(__NR_recvfrom, socket, buffer, length, flags, address, address_len);
}

size_t syscall_send(int socket, const void *buffer, size_t length, int flags)
{
    return ::syscall(__NR_sendto, socket, buffer, length, flags, 0, 0);
}

ssize_t syscall_recv(int socket, void *buffer, size_t length, int flags)
{
    return ::syscall(__NR_recvfrom, socket, buffer, length, flags, 0, 0);
}

int syscall_poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
    return ::syscall(__NR_poll, fds, nfds, timeout);
}

int syscall_setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len)
{
    return ::syscall(__NR_setsockopt, socket, level, option_name, option_value, option_len);
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
                ret = ::syscall(__NR_fcntl, fildes, cmd, param);
                break;
            }
		case F_GETFD:
		case F_GETFL:
		case F_GETOWN:
            {
                ret = ::syscall(__NR_fcntl, fildes, cmd);
                break;
            }
		case F_GETLK:
		case F_SETLK:
		case F_SETLKW:
            {
                /* struct flock *param = va_arg(args,struct flock *); */
                void *param = va_arg(args, void *);
                ret = ::syscall(__NR_fcntl, fildes, cmd, param);
                break;
            }
	}
	va_end(args);
	return ret;
}

pid_t syscall_gettid(void)
{
    return ::syscall(__NR_gettid);
}

int syscall_open(const char *pathname, int flags, ...)
{
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    return ::syscall(__NR_open, pathname, flags, mode);
}

int syscall_openat(int dirfd, const char *pathname, int flags, ...)
{
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    return ::syscall(__NR_openat, dirfd, pathname, flags, mode);
}

int syscall_creat(const char *pathname, mode_t mode)
{
    return ::syscall(__NR_creat, pathname, mode);
}

off_t syscall_lseek(int fd, off_t offset, int whence)
{
    return ::syscall(__NR_lseek, fd, offset, whence);
}

int syscall_fdatasync(int fd)
{
    return ::syscall(__NR_fdatasync, fd);
}

int syscall_fsync(int fd)
{
    return ::syscall(__NR_fsync, fd);
}

int syscall_rename(const char *oldpath, const char *newpath)
{
    return ::syscall(__NR_rename, oldpath, newpath);
}

int syscall_truncate(const char *path, off_t length)
{
    return ::syscall(__NR_truncate, path, length);
}

int syscall_ftruncate(int fd, off_t length)
{
    return ::syscall(__NR_truncate, fd, length);
}

int syscall_rmdir(const char *pathname)
{
    return ::syscall(__NR_rmdir, pathname);
}

int syscall_mkdir(const char *pathname, mode_t mode)
{
    return ::syscall(__NR_mkdir, pathname, mode);
}

int syscall_getdents(unsigned int fd, void *dirp, unsigned int count)
{
    return ::syscall(__NR_getdents, fd, dirp, count);
}

int syscall_stat(const char *pathname, struct stat *buf)
{
    return ::syscall(__NR_stat, pathname, buf);
}

int syscall_fstat(int fd, struct stat *buf)
{
    return ::syscall(__NR_fstat, fd, buf);
}

int syscall_lstat(const char *pathname, struct stat *buf)
{
    return ::syscall(__NR_lstat, pathname, buf);
}

int syscall_link(const char *oldpath, const char *newpath)
{
    return ::syscall(__NR_link, oldpath, newpath);
}

int syscall_symlink(const char *target, const char *linkpath)
{
    return ::syscall(__NR_symlink, target, linkpath);
}

ssize_t syscall_readlink(const char *pathname, char *buf, size_t bufsiz)
{
    return ::syscall(__NR_readlink, pathname, buf, bufsiz);
}

int syscall_unlink(const char *pathname)
{
    return ::syscall(__NR_unlink, pathname);
}

int syscall_chmod(const char *pathname, mode_t mode)
{
    return ::syscall(__NR_chmod, pathname, mode);
}

int syscall_fchmod(int fd, mode_t mode)
{
    return ::syscall(__NR_fchmod, fd, mode);
}

int syscall_chown(const char *pathname, uid_t owner, gid_t group)
{
    return ::syscall(__NR_chown, pathname, owner, group);
}

int syscall_fchown(int fd, uid_t owner, gid_t group)
{
    return ::syscall(__NR_fchown, fd, owner, group);
}

int syscall_lchown(const char *pathname, uid_t owner, gid_t group)
{
    return ::syscall(__NR_lchown, pathname, owner, group);
}

int syscall_utime(const char *filename, const struct utimbuf *times)
{
    return ::syscall(__NR_utime, filename, times);
}

int syscall_utimes(const char *filename, const struct timeval times[2])
{
    return ::syscall(__NR_utimes, filename, times);
}

#if 0
int syscall_futimes(int fd, const struct timeval tv[2])
{
    return ::syscall(__NR_futimes, fd, tv);
}

int syscall_lutimes(const char *filename, const struct timeval tv[2])
{
    return ::syscall(__NR_lutimes, filename, tv);
}
#endif

}
