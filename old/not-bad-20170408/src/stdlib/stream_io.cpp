/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-12-02
 * ================================
 */

#include "zcc.h"

namespace zcc
{

iostream::iostream(int fd)
{
    _fd = fd;
}

iostream::~iostream()
{
    flush();
}

ssize_t iostream::read_fn(void *buf, size_t size, long timeout)
{
    return timed_read(_fd, buf, size, timeout);
}

ssize_t iostream::write_fn(const void *buf, size_t size, long timeout)
{
    return timed_write(_fd, buf, size, timeout);
}

int iostream::get_fd()
{
    return _fd;
}

}
