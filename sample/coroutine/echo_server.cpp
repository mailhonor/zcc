/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-06-26
 * ================================
 */

#include "zcc.h"

void ___usage(const char *p = 0)
{
    printf("USAGE: %s --l address\n", zcc::var_progname);
    exit(1);
}

void *echo_service(void *context)
{
    int fd = (int)(long)context;
    printf("fd:%d\n", fd);
    FILE *fp = fdopen(fd, "a+");
    char buf[4096 + 10];
    while(1) {
        zcc::timed_wait_readable(fd, 1000);
        printf("AAAAAAAAAAAA readable\n");
        if (fgets(buf, 4096, fp) == 0) {
        printf("AAAAAAAAAAAA readable continue\n");
            continue;
        }
        zcc::timed_wait_writeable(fd, 1000);
        zcc::timed_write(fd, buf, strlen(buf), 1000);
    }
    return context;
}


void *do_listen(void *context)
{
    if (zcc::empty(zcc::var_listen_address)) {
        ___usage();
    }
    int sock_type;
    int sock = zcc::listen("0:8899", &sock_type); 
    while(1) {
        int fd = zcc::accept(sock, sock_type);
        if (fd < 0) {
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
    zcc::var_progname = argv[0];
    zcc_main_parameter_begin() {
        if (!optval) {
            ___usage();
        }
    } zcc_main_parameter_end;
    zcc::coroutine_base_init();
    zcc::coroutine_go(do_listen, 0);
    zcc::coroutine_base_loop();
    zcc::coroutine_base_fini();
    return 0;
}
