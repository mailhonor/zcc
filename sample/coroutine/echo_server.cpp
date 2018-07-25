/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-06-26
 * ================================
 */

#include "zcc.h"
#include <unistd.h>
#include <fcntl.h>

void ___usage(const char *p = 0)
{
    printf("USAGE: %s -listen address\n", zcc::var_progname);
    exit(1);
}

namespace zcc
{
int syscall_fcntl(int fildes, int cmd, ...);
}

bool __nonblocking(int fd, bool no)
{
    int flags;

    if ((flags = zcc::syscall_fcntl(fd, F_GETFL, 0)) < 0) {
        return false;
    }

    if (zcc::syscall_fcntl(fd, F_SETFL, no ? flags | O_NONBLOCK : flags & ~O_NONBLOCK) < 0) {
        return false;
    }

    return ((flags & O_NONBLOCK) != 0);
}

#if 0
void *echo_service(void *context)
{
    int fd = (int)(long)context;
    __nonblocking(fd, false);
    printf("fd:%d\n", fd);
    FILE *fp = fdopen(fd, "a+");
    char buf[4096 + 10];
    while(1) {
        printf("AAAAAAAAAAAA readable\n");
        if (fgets(buf, 4096, fp) == 0) {
            printf("end\n");
            break;
        }
        zcc::timed_wait_writeable(fd, 1000);
        zcc::timed_write(fd, buf, strlen(buf), 1000);
    }
    return context;
}
#endif

void *echo_service(void *context)
{
    int fd = (int)(long)context;
    printf("fd:%d\n", fd);
    zcc::stream fp(fd);
    std::string str;
    while(1) {
        printf("AAAAAAAAAAAA readable\n");
        str.clear();
        if (fp.gets(str) < 1) {
            printf("end\n");
            break;
        }
        zcc::timed_wait_writeable(fd, 1000);
        zcc::timed_write(fd, str.c_str(), str.size(), 1000);
    }
    return context;
}
void *do_listen(void *context)
{
    int sock_type;
    int sock = zcc::listen("0:8899", &sock_type); 
    while(1) {
        if (zcc::timed_wait_readable(sock, 10 * 1000) < 0) {
            printf("error\n");
            break;
        }
        int fd = zcc::accept(sock, sock_type);
        if (fd < 0) {
            printf("fd is 0\n");
            return 0;
        }
        printf("accept ok: %d\n", fd);
        void *arg = (void *)((long)fd);
        zcc::coroutine_go(echo_service, arg);
    }
    return 0;
}

int main(int argc, char **argv)
{
    zcc::main_parameter_run(argc, argv);
    zcc::coroutine_base_init();
    zcc::coroutine_go(do_listen, 0);
    zcc::coroutine_base_loop();
    zcc::coroutine_base_fini();
    return 0;
}
