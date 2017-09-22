/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-24
 * ================================
 */

#include "zcc.h"
#include <time.h>

void * do_echo(void *arg)
{
    time_t t = time(0);
    int fd = (int)(long)arg;
    zcc::iostream fp(fd);

    fp.printf_1024("welcome coroutine: %s\n", ctime(&t));
    fp.flush();

    zcc::string line;

    while (!fp.is_exception()) {
        line.clear();
        fp.gets(line);
        if (line.size()) {
            fp.write(line.c_str(), line.size());
        }
    }
    fp.flush();
    close(fp.get_fd());

    return 0;
}

static void * do_accept(void *arg)
{
    int sock = (int)(long)arg;
    while(1) {
        zcc::timed_wait_readable(sock, 10 * 1000);
        int fd = zcc::inet_accept(sock);
        if (fd < 0) {
            continue;
        }
        zcc::coroutine_go(do_echo, (void *)(long)fd);
    }
}

class myserver: public zcc::master_coroutine_server
{
public:
    inline myserver() {};
    inline ~myserver() {};
    void service_register(const char *service_name, int fd, int fd_type);
};

void myserver::service_register(const char *service_name, int fd, int fd_type)
{
    zcc::coroutine_go(do_accept, (void *)(long)(int)(fd));
}

int main(int argc, char **argv)
{
    myserver ms;
    ms.run(argc, argv);
    return 0;
}
