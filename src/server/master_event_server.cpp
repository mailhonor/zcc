/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-20
 * ================================
 */

#include "zcc.h"
#include <signal.h>
#include <list>
#include <sys/types.h>
#include <dirent.h>

static void ___usage(const char *arg)
{
    (void)arg;
    printf("do not run this command by hand\n");
    exit(1);
}

namespace zcc
{

bool master_event_server::flag_reloading = false;

static master_event_server *___instance;
static bool flag_init = false;
static bool flag_alone_mode = false;
static bool flag_run = false;
static bool flag_inner_event_loop = false;
static bool flag_clear = false;
static pid_t parent_pid = 0;
static event_io *ev_status = 0;
static event_timer *reload_timer = 0;
static event_timer *stop_file_timer = 0;
static std::list<event_io *> event_ios;

static void load_global_config_by_dir(const char *config_path)
{
    DIR *dir;
    struct dirent ent, *ent_list;
    char *fn, *p;
    char pn[4100];

    dir = opendir(config_path);
    if (!dir) {
        return;
    }

    while ((!readdir_r(dir, &ent, &ent_list)) && (ent_list)) {
        fn = ent.d_name;
        if (fn[0] == '.') {
            continue;
        }
        p = strrchr(fn, '.');
        if ((!p) || (strcasecmp(p + 1, "gcf"))) {
            continue;
        }
        snprintf(pn, 4096, "%s/%s", config_path, fn);
        default_config.load_from_filename(pn);
    }
    closedir(dir);
}

static char *stop_file = 0;

static void stop_file_check(event_timer &tm)
{
    if (file_get_size(stop_file) < 0) {
        var_proc_stop = true;
    } else {
        stop_file_timer->start(stop_file_check, 1000 * 1);
    }
}

static void reloading_to_stopping(event_timer &tm)
{
    var_proc_stop = true;
}

static void on_master_reload(event_io &ev)
{
    master_event_server *ms = (master_event_server *)ev.get_context();
    master_event_server::flag_reloading = true;
    if (getppid() == parent_pid) {
        ms->before_reload();
        reload_timer = new event_timer();
        reload_timer->init();
        reload_timer->start(reloading_to_stopping, 10 * 1000);
    } else {
        ms->stop_notify();
    }
}

static void ___inet_server_accept(event_io &ev)
{
    int fd, listen_fd = ev.get_fd();
    void (*cb)(int) = (void(*)(int))ev.get_context();

    fd = inet_accept(listen_fd);
    if (fd < 0) {
        if (errno != EAGAIN) {
            zcc_fatal("inet_server_accept: %m");
        }
        return;
    }
    nonblocking(fd);
    close_on_exec(fd);
    if (cb) {
        cb(fd);
    } else {
        ___instance->simple_service(fd);
    }
}

static void ___unix_server_accept(event_io &ev)
{
    int fd, listen_fd = ev.get_fd();
    void (*cb)(int) = (void(*)(int))ev.get_context();

    fd = unix_accept(listen_fd);
    if (fd < 0) {
        if (errno != EAGAIN) {
            zcc_fatal("unix_server_accept: %m");
        }
        return;
    }
    nonblocking(fd);
    close_on_exec(fd);
    if (cb) {
        cb(fd);
    } else {
        ___instance->simple_service(fd);
    }
}

master_event_server::master_event_server()
{
    if (!flag_init) {
        signal(SIGPIPE, SIG_IGN);
        flag_init = true;
        flag_reloading = false;
        flag_alone_mode = false;
        flag_run = false;
        parent_pid = getppid();
        ev_status = 0;
        reload_timer = 0;
        /* private */
        flag_inner_event_loop = false;
        flag_clear = false;
    }
}

master_event_server::~master_event_server()
{
}

void master_event_server::clear()
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
    if (reload_timer) {
        delete reload_timer;
        reload_timer = 0;
    }
    if (stop_file_timer) {
        delete stop_file_timer;
        stop_file_timer = 0;
    }
    for (std::list<event_io *>::iterator it = event_ios.begin(); it != event_ios.end(); it++) {
        int fd = (*it)->get_fd();
        delete *it;
        close(fd);
    }
    event_ios.clear();
}

void master_event_server::stop_notify()
{
    var_proc_stop = true;
    default_evbase.notify();
}

void master_event_server::alone_register(char *alone_url)
{
    char service_buf[1024];
    char *service, *url, *p;
    strtok splitor(alone_url);
    
    while(splitor.tok(" \t,;\\r\n")) {
        if (splitor.size() > 1000) {
            zcc_fatal("alone_register: url too long");
        }
        memcpy(service_buf, splitor.ptr(), splitor.size());
        service_buf[splitor.size()] = 0;
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
            zcc_fatal("alone_register: open %s(%m)", alone_url);
        }
        close_on_exec(fd);
        nonblocking(fd);
        service_register(service, fd, fd_type);
    }
}

void master_event_server::inner_service_register(char *s, char *optval)
{
    int t = optval[0];
    switch(t) {
        case var_tcp_listen_type_inet:
        case var_tcp_listen_type_unix:
        case var_tcp_listen_type_fifo:
            break;
        default:
            zcc_fatal("master_event_server: unknown service type %c", t);
            break;
    }
    int fd = atoi(optval+1);
    if (fd < 1) {
        zcc_fatal("master_event_server: fd is invalid", optval+1);
    }
    close_on_exec(fd);
    nonblocking(fd);
    service_register(s, fd, t);
}

void master_event_server::before_service()
{
}

void master_event_server::event_loop()
{
    flag_inner_event_loop = true;
}

void master_event_server::before_reload()
{
}

void master_event_server::before_exit()
{
}

void master_event_server::simple_service(int fd)
{
    zcc_fatal("must be make a new simple_service when use simpe mode");
}

event_io *master_event_server::general_service_register(int fd, int fd_type
        , void (*service) (int) , event_base &eb)
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

void master_event_server::service_register(const char *service_name, int fd, int fd_type)
{
    general_service_register(fd, fd_type, 0);
}

void master_event_server::run(int argc, char ** argv)
{
    var_proc_stop_handler = true;
    if (flag_run) {
        zcc_fatal("master_event_server:: only run one time");
    }
    flag_run = true;
    ___instance = this;
    flag_alone_mode = true;
    var_progname = argv[0];

    zcc_main_parameter_begin() {
        if (!strcmp(optname, "-M")) {
            flag_alone_mode = false;
            opti += 1;
            continue;
        }
        if (optval == 0) {
            ___usage(optname);
        }
        if (!strcmp(optname, "-global_config")) {
            load_global_config_by_dir(optval);
            opti += 2;
            continue;
        }
        if (!strncmp(optname, "-s", 2)) {
            inner_service_register(optname+2, optval);
            opti += 2;
            continue;
        }
        if (!strcmp(optname, "-stop-file")) {
            stop_file = optval;
            opti += 2;
            continue;
        }
    } zcc_main_parameter_end;

    if (!empty(var_listen_address)) {
        alone_register(var_listen_address);
    }

    if (!flag_alone_mode) {
        ev_status = new event_io();
        close_on_exec(var_master_master_status_fd);
        nonblocking(var_master_master_status_fd);
        ev_status->init(var_master_master_status_fd);
        ev_status->enable_read(on_master_reload);
        ev_status->set_context(this);
        if (stop_file) {
            stop_file_timer = new event_timer();
            stop_file_timer->init();
            stop_file_timer->start(stop_file_check, 1000 * 1);
        }
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
        if (var_proc_stop) {
            clear();
            break;
        }
    }
    clear();
    before_exit();
}

}
