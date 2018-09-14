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
static char *file_output;
static char *listenon;


class myhttpd:public zcc::httpd
{
public:
    void handler();
};

void myhttpd::handler()
{
    response_file(file_output);
}

static void ___usage()
{
    printf("USAGE: %s -listen listen_address [ -f file_output(index.html)] [ -times stop_after_loop_times ]\n", zcc::var_progname);
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
        zcc::timed_wait_readable(sock, 1000);
        int fd = zcc::accept(sock, sock_type);
        if (fd < 0) {
            if (errno == EAGAIN) {
                continue;
            }
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
    zcc::main_parameter_run(argc, argv);
    file_output = zcc::default_config.get_str("f", "./index.html");
    times = zcc::default_config.get_int("times", 0, 0, 100000);
    listenon = zcc::default_config.get_str("listen", "");
    if (zcc::empty(listenon)) {
        ___usage();
    }

    zcc::coroutine_base_init();
    sock = zcc::listen(listenon, &sock_type, 10000);
    if (sock < 0) {
        printf("open %s error (%m)\n", listenon);
        exit(1);
    }
    zcc::coroutine_go(accept_incoming, 0);
    zcc::coroutine_base_loop();
    zcc::coroutine_base_fini();
    return 0;
}
