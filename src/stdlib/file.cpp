/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2015-12-04
 * ================================
 */

#include "zcc.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace zcc
{

ssize_t file_get_size(const char *filename)
{
    struct stat st;

    if (stat(filename, &st) == -1) {
        return -1;
    }
    return st.st_size;
}

/* ################################################################## */
/* file get/put contents */
bool file_put_contents(const char *filename, const void *data, size_t len)
{
    int fd;
    ssize_t ret;
    size_t wlen = 0;
    int errno2;

    while ((fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0777)) == -1 && errno == EINTR) {
        continue;
    }

    if (fd == -1) {
        return false;
    }

    while (len > wlen) {
        ret = write(fd, (const char *)data + wlen, len - wlen);
        if (ret > -1) {
            wlen += ret;
            continue;
        }
        errno2 = errno;
        if (errno == EINTR) {
            continue;
        }
        close(fd);
        errno = errno2;
        return false;
    }

    close(fd);

    return true;
}

ssize_t file_get_contents(const char *filename, std::string &str)
{
    int fd;
    int errno2;
    ssize_t ret;
    char buf[4096 + 1];
    size_t *rlen = 0;
    str.clear();

    while ((fd = open(filename, O_RDONLY)) == -1 && errno == EINTR) {
        continue;
    }
    if (fd == -1) {
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        return -1;
    }
    str.reserve(st.st_size);

    while(1) {
        ret = read(fd, buf, 4096);
        if (ret < 0) {
            errno2 = errno;
            if (errno == EINTR) {
                continue;
            }
            close(fd);
            errno = errno2;
            return -1;
        }
        if (ret == 0) {
            break;
        }
        str.append(buf, ret);
        rlen += ret;
    }
    close(fd);

    return (ssize_t)rlen;
}

ssize_t file_get_contents_sample(const char *filename, std::string &str)
{
    str.clear();
    ssize_t ret = file_get_contents(filename, str);
    if (ret < 0) {
        zcc_info("ERR load from %s (%m)", filename);
        exit(1);
    }
    return ret;
}

ssize_t stdin_get_contents(std::string &str)
{
    int fd = 0;
    int errno2;
    ssize_t ret;
    char buf[4096 + 1];
    size_t *rlen = 0;
    str.clear();

    while(1) {
        ret = read(fd, buf, 4096);
        if (ret < 0) {
            errno2 = errno;
            if (errno == EINTR) {
                continue;
            }
            close(fd);
            errno = errno2;
            return -1;
        }
        if (ret == 0) {
            break;
        }
        str.append(buf, ret);
        rlen += ret;
    }

    return (ssize_t)rlen;
}

/* ################################################################## */
file_mmap::file_mmap()
{
    _fd = -1;
    _data = 0;
    _size = 0;
}

file_mmap::~file_mmap()
{
    munmap();
}

bool file_mmap::mmap(const char *filename)
{
    int fd;
    size_t size;
    void *data;
    struct stat st;
    int errno2;

    munmap();

    while ((fd = open(filename, O_RDONLY)) == -1 && errno == EINTR) {
        continue;
    }
    if (fd == -1) {
        return false;
    }
    if (fstat(fd, &st) == -1) {
        errno2 = errno;
        close(fd);
        errno = errno2;
        return false;
    }
    size = st.st_size;
    data = ::mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        errno2 = errno;
        close(fd);
        errno = errno2;
        return false;
    }
    _fd = fd;
    _data = (char *)data;
    _size = size;
    return true;
}

void file_mmap::munmap()
{
    if (_fd != -1) {
        ::munmap(_data, _size);
        close(_fd);
        _fd = -1;
    }
}

}
