/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-20
 * ================================
 */

#include "zcc.h"
#include <signal.h>

static void ___usage(const char *arg)
{
    (void)arg;
    printf("do not run this command by hand\n");
    exit(1);
}

namespace zcc
{

bool master_server::flag_reloading = false;
bool master_server::flag_stopping = false;

static bool flag_init = false;
static bool flag_test_mode = false;
static bool flag_run = false;
static bool flag_inner_event_loop = false;
static bool flag_clear = false;
static pid_t parent_pid = 0;
static event_io *ev_status = 0;
static event_timer *reload_timer = 0;
static list<event_io *> event_ios;

static void reloading_to_stopping(event_timer &tm)
{
    master_server::flag_stopping = true;
}

static void on_master_reload(event_io &ev)
{
    master_server *ms = (master_server *)ev.get_context();
    master_server::flag_reloading = true;
    if (getppid() == parent_pid) {
        ms->before_reload();
        reload_timer->start(reloading_to_stopping, 10 * 1000);
    } else {
        ms->stop_notify();
    }
}

static void ___inet_server_accept(event_io &ev)
{
    int fd, listen_fd = ev.get_fd();
    void (*cb)(int, event_io &) = (void(*)(int, event_io &))ev.get_context();

    fd = inet_accept(listen_fd);
    if (fd < 0) {
        if (errno != EAGAIN) {
            log_fatal("inet_server_accept: %m");
        }
        return;
    }
    nonblocking(fd);
    close_on_exec(fd);
    cb(fd, ev);
}

static void ___unix_server_accept(event_io &ev)
{
    int fd, listen_fd = ev.get_fd();
    void (*cb)(int, event_io &) = (void(*)(int, event_io &))ev.get_context();

    fd = unix_accept(listen_fd);
    if (fd < 0) {
        if (errno != EAGAIN) {
            log_fatal("unix_server_accept: %m");
        }
        return;
    }
    nonblocking(fd);
    close_on_exec(fd);
    cb(fd, ev);
}

static void ___inet_server_accept_simple(event_io &ev)
{
    int fd, listen_fd = ev.get_fd();
    master_server * ms = (master_server *)ev.get_context();

    fd = inet_accept(listen_fd);
    if (fd < 0) {
        if (errno != EAGAIN) {
            log_fatal("inet_server_accept: %m");
        }
        return;
    }
    nonblocking(fd);
    close_on_exec(fd);
    ms->simple_service(fd);
}

static void ___unix_server_accept_simple(event_io &ev)
{
    int fd, listen_fd = ev.get_fd();
    master_server * ms = (master_server *)ev.get_context();

    fd = unix_accept(listen_fd);
    if (fd < 0) {
        if (errno != EAGAIN) {
            log_fatal("unix_server_accept: %m");
        }
        return;
    }
    nonblocking(fd);
    close_on_exec(fd);
    ms->simple_service(fd);
}

/* ##################################################### */
master_server::master_server()
{
    if (!flag_init) {
        signal(SIGPIPE, SIG_IGN);
        flag_init = true;
        flag_reloading = false;
        flag_stopping = false;
        flag_test_mode = false;
        flag_run = false;
        parent_pid = getppid();
        ev_status = 0;
        reload_timer = 0;
        /* private */
        flag_inner_event_loop = false;
        flag_clear = false;
    }
}

master_server::~master_server()
{
}

void master_server::clear()
{
    if (flag_clear) {
        return;
    }
    flag_clear = true;
    if (ev_status) {
        /* The master would receive the signal of closing ZMASTER_SERVER_STATUS_FD. */
        delete ev_status;
        ev_status = 0;
        close(var_master_server_status_fd);
        close(var_master_master_status_fd);
    }
    event_io *eio;
    zcc_list_walk_begin(event_ios, eio) {
        int fd = eio->get_fd();
        delete eio;
        close(fd);
    } zcc_list_walk_end;
    event_ios.clear();
}

void master_server::stop_notify()
{
    flag_stopping = 1;
    default_evbase.notify();
}

void master_server::test_register(char *test_url)
{
    char service_buf[1024];
    char *service, *url, *p;

    strcpy(service_buf, test_url);
    p = strstr(service_buf, "://");
    if (p) {
        *p=0;
        service = service_buf;
        url = p+1;
    } else {
        service = blank_buffer;
        url = service_buf;
    }

    int fd_type;
    int fd = listen(url, 5,  &fd_type);
    if (fd < 0) {
        log_fatal("master_server_test: open %s(%m)", test_url);
    }
    close_on_exec(fd);
    nonblocking(fd);
    service_register(service, fd, fd_type);
}

void master_server::inner_service_register(char *s, char *optval)
{
    int t = optval[0];
    switch(t) {
        case var_tcp_listen_type_inet:
        case var_tcp_listen_type_unix:
        case var_tcp_listen_type_fifo:
            break;
        default:
            log_fatal("master_server: unknown service type %c", t);
            break;
    }
    int fd = atoi(optval+1);
    if (fd < 1) {
        log_fatal("master_server: fd is invalid", optval+1);
    }
    close_on_exec(fd);
    nonblocking(fd);
    service_register(s, fd, t);
}

void master_server::before_service()
{
}

void master_server::event_loop()
{
    flag_inner_event_loop = true;
}

void master_server::before_reload()
{
}

void master_server::before_exit()
{
}

void master_server::simple_service(int fd)
{
    log_fatal("must be make a new simple_service when use simpel mode");
}

event_io *master_server::general_service_register(int fd, int fd_type
        , void (*service) (int, event_io &) , event_base &eb)
{
    event_io *ev = new event_io();
    ev->init(fd, eb);
    ev->set_context((void *)service);
    if (fd_type == var_tcp_listen_type_inet) {
        ev->enable_read(___inet_server_accept);
    } else if (fd_type == var_tcp_listen_type_unix) {
        ev->enable_read(___unix_server_accept);
    } else if (fd_type == var_tcp_listen_type_fifo) {
    }
    event_ios.push_back(ev);
    return ev;
}

void master_server::service_register(const char *service_name, int fd, int fd_type)
{
    event_io *ev = new event_io();
    ev->init(fd);
    ev->set_context(this);
    if (fd_type == var_tcp_listen_type_inet) {
        ev->enable_read(___inet_server_accept_simple);
    } else if (fd_type == var_tcp_listen_type_unix) {
        ev->enable_read(___unix_server_accept_simple);
    } else if (fd_type == var_tcp_listen_type_fifo) {
    }
    event_ios.push_back(ev);
}

void master_server::run(int argc, char ** argv)
{
    if (flag_run) {
        log_fatal("master_server:: only run one time");
    }
    flag_run = true;
    flag_test_mode = true;
    var_progname = argv[0];

    ZCC_PARAMETER_BEGIN() {
        if (!strcmp(optname, "-M")) {
            flag_test_mode = false;
            opti += 1;
            continue;
        }
        if (optval == 0) {
            ___usage(optname);
        }
        if (!strncmp(optname, "-s", 2)) {
            inner_service_register(optname+2, optval);
            opti += 2;
            continue;
        }
        if (!strncmp(optname, "-l", 2)) {
            test_register(optval);
            opti += 2;
            continue;
        }
    } ZCC_PARAMETER_END;

    if (!flag_test_mode) {
        ev_status = new event_io();
        close_on_exec(var_master_master_status_fd);
        nonblocking(var_master_master_status_fd);
        ev_status->init(var_master_master_status_fd);
        ev_status->enable_read(on_master_reload);
        ev_status->set_context(this);
    }

    before_service();

    while (1) {
        default_evbase.dispatch();
        if (!flag_inner_event_loop) {
            event_loop();
        }
        if (flag_reloading) {
            clear();
            if (getppid() != parent_pid) {
                break;
            }
        }
        if (flag_stopping) {
            clear();
            break;
        }
    }
    clear();
    before_exit();
}

}
