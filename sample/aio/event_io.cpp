/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-02-05
 * ================================
 */


#include "zcc.h"
#include <time.h>

static bool ___EXIT = false;

static void server_error(zcc::event_io &ev)
{
    int fd;
    zcc::stream *fp;

    fd = ev.get_fd();
    fp = (zcc::stream *)ev.get_context();

    if (ev.get_result() < 0) {
        zcc_info("fd:%d, exception", fd);
    } else {
        zcc_info("fd:%d, clsoed", fd);
    }

    delete &ev;
    if (fp) {
        delete fp;
    }
    close(fd);
}

static void server_read(zcc::event_io &ev)
{
    int fd;
    const char *r;
    zcc::stream *fp;
    std::string rbuf;

    fd = ev.get_fd();

    if (ev.get_result() < 1) {
        server_error(ev);
        return;
    }

    fp = (zcc::stream *) ev.get_context();
    if (fp->gets(rbuf) < 1) {
        server_error(ev);
        return;
    }
    r = rbuf.c_str();
    if (!strncasecmp(r, "exit", 4)) {
        delete fp;
        delete &ev;
        close(fd);
        if (!strncmp(r, "EXIT", 4)) {
            ___EXIT = true;
        }
        return;
    }

    fp->puts("your input: ");
    fp->write(rbuf.c_str(), rbuf.size());
    if (!fp->flush()) {
        server_error(ev);
        return;
    }

    ev.enable_read(server_read);
}

static void server_welcome(zcc::event_io &ev)
{
    if (ev.get_result() < 1) {
        server_error(ev);
        return;
    }
    zcc::stream *fp = new zcc::stream(ev.get_fd());
    time_t t = time(0);
    fp->printf_1024("welcome ev: %s\n", ctime(&t));
    if (!fp->flush()) {
        server_error(ev);
        return;
    }
#if 0
    delete fp;
    int fd = ev.get_fd();
    delete &ev;
    close(fd);
    return;
#endif
    ev.set_context(fp);
    ev.enable_read(server_read);
}

static void before_accept(zcc::event_io & ev)
{
    int sock = ev.get_fd();
    int fd = zcc::inet_accept(sock);
    zcc::event_io *nev = new zcc::event_io();
    nev->bind(fd);
    nev->enable_write(server_welcome);
}

static char * listen_on;
int main(int argc, char **argv)
{
    zcc::main_parameter_run(argc, argv);
    listen_on = zcc::default_config.get_str("zlisten", "0:8899");

    int sock;
    zcc::event_io *ev;

    sock = zcc::listen(listen_on, 0);
    zcc::nonblocking(sock);
    ev = new zcc::event_io();
    ev->bind(sock);
    ev->enable_read(before_accept);

    while (!___EXIT) {
        zcc::default_evbase.dispatch();
    }
    delete ev;
    close(sock);
    printf("AAA\n");

    return 0;
}
