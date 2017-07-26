/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-04-05
 * ================================
 */

/*
ab -n 1000 -c 1000 http://127.0.0.1:8080/
*/

#include "zcc.h"
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static int times = 0;
static int times_count = 0;
static int sock;
static int sock_type;
const char *file_output = "./index.html";

class myhttpd:public zcc::httpd
{
public:
    void handler();
};

void myhttpd::handler()
{
    response_file_by_absolute_path(file_output);
}

static void ___usage()
{
    printf("USAGE: %s --l listen_address [ -f file_output(index.html)] [ -times stop_after_loop_times ]\n", zcc::var_progname);
    exit(1);
}

static void *do_http(void *arg)
{
    int fd = (int)(long)arg;
    myhttpd httpd;
    httpd.bind(fd);
    httpd.run();
    close(fd);
    return 0;
}

static void *accept_incoming(void *arg)
{
    while (1) {
        int fd = zcc::accept(sock, sock_type);
        if (fd < 0) {
            break;
        }
        if (times) {
            if (times_count++ == times) {
                zcc::var_proc_stop = true;
            }
        }
        if (zcc::var_proc_stop) {
            close(fd);
            break;
        }
        zcc::coroutine_go(do_http, (void *)(long)fd);
    }
    close(sock);
    return arg;
}

int main(int argc, char **argv)
{
    zcc_main_parameter_begin() {
        if (!optval) {
            ___usage();
        }
        if (!strcmp(optname, "-f")) {
            file_output = optval;
            opti += 2;
            continue;
        }
        if (!strcmp(optname, "-times")) {
            times = atoi(optval);
            opti += 2;
            continue;
        }
    } zcc_main_parameter_end;

    if (zcc::empty(zcc::var_listen_address)) {
        ___usage();
    }

    zcc::coroutine_base_init();
    sock = zcc::listen(zcc::var_listen_address, &sock_type);
    if (sock < 0) {
        printf("open %s error (%m)\n", zcc::var_listen_address);
        exit(1);
    }
    zcc::coroutine_go(accept_incoming, 0);
    zcc::coroutine_base_loop();
    zcc::coroutine_base_fini();
    return 0;
}
