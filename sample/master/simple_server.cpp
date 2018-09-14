/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-11-24
 * ================================
 */

#include "zcc.h"
#include <time.h>

static void after_write(zcc::async_io &aio);
static void service_error(zcc::async_io &aio)
{
    int fd = aio.get_fd();
    zcc_info("%d: error or idle too long", fd);
    delete &aio;
    close(fd);
}

static void after_read(zcc::async_io &aio)
{
    int ret, fd, len;
    char rbuf[102400];
    char *p;

    ret = aio.get_result();
    fd = aio.get_fd();
    if (ret < 1) {
        return service_error(aio);
    }
    aio.fetch_rbuf(rbuf, ret);

    if (ret > 3 && !strncasecmp(rbuf, "exit", 4)) {
        delete &aio;
        close(fd);
        if (!strncmp(rbuf, "EXIT", 4)) {
            zcc::master_event_server::stop_notify();
        }
        return;
    }

    rbuf[ret] = 0;
    p = strchr(rbuf, '\r');
    if (p) {
        *p = 0;
    }
    p = strchr(rbuf, '\n');
    if (p) {
        *p = 0;
    }
    len = strlen(rbuf);

    aio.cache_write("your input:   ", 12);
    aio.cache_write(rbuf, len);
    aio.cache_write("\n", 1);
    aio.cache_flush(after_write, 1000);
}

static void after_write(zcc::async_io &aio)
{
    int ret;
    printf("before_write\n");

    ret = aio.get_result();

    if (ret < 1) {
        return service_error(aio);
    }
    aio.read_line(1024, after_read, 10 * 1000);
}

class myserver: public zcc::master_event_server
{
public:
    inline myserver() {};
    inline ~myserver() {};
    void simple_service(int fd);
};

void myserver::simple_service(int fd)
{
    time_t t = time(0);
    zcc::async_io *aio = new zcc::async_io;
    aio->bind(fd);
    aio->cache_printf_1024("welcome aio: %s\n", ctime(&t));
    aio->cache_flush(after_write, 1 * 1000);
}

int main(int argc, char **argv)
{
    zcc::master_event_server ms1;
    myserver ms;
    ms.run(argc, argv);
    return 0;
}
