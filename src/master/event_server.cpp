/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-11-20
 * ================================
 */

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "zcc.h"
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>

bool var_test_mode = false;

namespace zcc
{

static master_event_server *___instance;
static bool flag_init = false;
static bool flag_run = false;
static bool flag_inner_event_loop = false;
static bool flag_clear = false;
static pid_t parent_pid = 0;
static event_io *ev_status = 0;
static std::list<event_io *> event_ios;

static void load_global_config_by_dir(config &cf, const char *config_path)
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
        cf.load_by_filename(pn);
    }
    closedir(dir);
}

static char *stop_file = 0;

static void stop_file_check(event_timer &tm)
{
    if (file_get_size(stop_file) < 0) {
        var_proc_stop = true;
    } else {
        tm.start(stop_file_check, 1000 * 1);
    }
}

static void on_master_reload(event_io &ev)
{
    var_proc_stop = true;
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

static void ___fifo_server_accept(event_io &ev)
{
    int listen_fd = ev.get_fd();
    void (*cb)(int) = (void(*)(int))ev.get_context();

    if (cb) {
        cb(listen_fd);
    } else {
        ___instance->simple_service(listen_fd);
    }
}

master_event_server::master_event_server()
{
    if (!flag_init) {
        signal(SIGPIPE, SIG_IGN);
        flag_init = true;
        flag_run = false;
        parent_pid = getppid();
        ev_status = 0;
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
    std_list_walk_begin(event_ios, eio) {
        int fd = eio->get_fd();
        delete eio;
        close(fd);
    } std_list_walk_end;
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
    
    while(splitor.tok(" \t,;\r\n")) {
        if (splitor.size() > 1000) {
            zcc_fatal("alone_register: url too long");
        }
        memcpy(service_buf, splitor.ptr(), splitor.size());
        service_buf[splitor.size()] = 0;
        p = strstr(service_buf, "://");
        if (p) {
            *p=0;
            service = service_buf;
            url = p+3;
        } else {
            service = blank_buffer;
            url = service_buf;
        }

        int fd_type;
        int fd = listen(url,  &fd_type);
        if (fd < 0) {
            zcc_fatal("alone_register: open %s(%m)", alone_url);
        }
        close_on_exec(fd);
        nonblocking(fd);
        service_register(service, fd, fd_type);
    }
}

void master_event_server::master_register(char *master_url)
{
    zcc::argv service_argv;
    service_argv.split(master_url, ",");
    for (size_t i = 0; i < service_argv.size(); i++) {
        char *service_name, *typefd;
        zcc::argv stfd;
        stfd.split(service_argv[i], ":");
        if (stfd.size() == 1) {
            service_name = blank_buffer;
            typefd = stfd[0];
        } else {
            service_name = stfd[0];
            typefd = stfd[1];
        }
        int fdtype = typefd[0];
        switch(fdtype) {
            case var_tcp_listen_type_inet:
            case var_tcp_listen_type_unix:
            case var_tcp_listen_type_fifo:
                break;
            default:
                zcc_fatal("master_event_server: unknown service type %c", fdtype);
                break;
        }
        int fd = atoi(typefd+1);
        if (fd < var_master_server_listen_fd) {
            zcc_fatal("master_event_server: fd(%s) is invalid", typefd+1);
        }
        close_on_exec(fd);
        nonblocking(fd);
        service_register(service_name, fd, fdtype);
    }
}

void master_event_server::before_service()
{
}

void master_event_server::event_loop()
{
    flag_inner_event_loop = true;
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
    ev->bind(fd, eb);
    ev->set_context((void *)service);
    if (fd_type == var_tcp_listen_type_inet) {
        ev->enable_read(___inet_server_accept);
    } else if (fd_type == var_tcp_listen_type_unix) {
        ev->enable_read(___unix_server_accept);
    } else if (fd_type == var_tcp_listen_type_fifo) {
        ev->enable_read(___fifo_server_accept);
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
    if (flag_run) {
        zcc_fatal("master_event_server:: only run one time");
    }
    flag_run = true;
    ___instance = this;
    char *attr;

    main_parameter_run(argc, argv);

    attr = default_config.get_str("server-config-path", "");
    if (!empty(attr)) {
        config cf;
        load_global_config_by_dir(cf, attr);
        cf.load_another(default_config);
        default_config.load_another(cf);
    }

    var_masterlog_listen = default_config.get_str("master-log-listen", "");
    stop_file = default_config.get_str("stop-file", "");

    log_use_by(argv[0], default_config.get_str("server-log"));

    if (default_config.get_bool("MASTER", false)) {
        ev_status = new event_io();
        close_on_exec(var_master_master_status_fd);
        nonblocking(var_master_master_status_fd);
        ev_status->bind(var_master_master_status_fd);
        ev_status->enable_read(on_master_reload);
        ev_status->set_context(this);
        if (!empty(stop_file)) {
            event_timer *stop_file_timer = new event_timer();
            stop_file_timer->bind();
            stop_file_timer->start(stop_file_check, 1000 * 1);
        }
    }

    before_service();

    if (!default_config.get_bool("MASTER", false)) {
        alone_register(default_config.get_str("server-service", ""));
    } else {
        master_register(default_config.get_str("server-service", ""));
    }

    attr = default_config.get_str("sever_user", "");
    if (!empty(attr)) {
        if(!chroot_user(0, attr)) {
            zcc_fatal("ERR chroot_user %s", attr);
        }
    }

    while (1) {
        if (var_proc_stop) {
            break;
        }
        default_evbase.dispatch();
        if (var_proc_stop) {
            break;
        }
        if (!flag_inner_event_loop) {
            event_loop();
        }
    }
    clear();

    before_exit();
}

}
