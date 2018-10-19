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

namespace zcc
{

static bool flag_init = false;
static bool flag_run = false;

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

static void *monitor_reload_signal(void *arg)
{
    timed_wait_readable(var_master_master_status_fd, 0);
    var_proc_stop = true;
    coroutine_base_stop_notify();
    return 0;
}

static char *stop_file = 0;
static void *stop_file_check(void *arg)
{
    while(!var_proc_stop) {
        msleep(1000);
        if (file_get_size(stop_file) < 0) {
            break;
        }
    }
    var_proc_stop = true;
    coroutine_base_stop_notify();
    return 0;
}

master_coroutine_server::master_coroutine_server()
{
    if (!flag_init) {
        signal(SIGPIPE, SIG_IGN);
        flag_init = true;
        flag_run = false;
    }
}

master_coroutine_server::~master_coroutine_server()
{
}

void master_coroutine_server::stop_notify()
{
    var_proc_stop = true;
}

void master_coroutine_server::alone_register(char *alone_url)
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
        int fd = listen(url, &fd_type, 1000);
        if (fd < 0) {
            zcc_fatal("alone_register: open %s(%m)", alone_url);
        }
        close_on_exec(fd);
        coroutine_enable_fd(fd);
        nonblocking(fd);
        service_register(service, fd, fd_type);
    }
}

void master_coroutine_server::master_register(char *master_url)
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
                zcc_fatal("master_coroutine_server: unknown service type %c", fdtype);
                break;
        }
        int fd = atoi(typefd+1);
        if (fd < var_master_server_listen_fd) {
            zcc_fatal("master_coroutine_server: fd(%s) is invalid", typefd+1);
        }
        close_on_exec(fd);
        coroutine_enable_fd(fd);
        nonblocking(fd);
        service_register(service_name, fd, fdtype);
    }
}

void master_coroutine_server::before_service()
{
}

void master_coroutine_server::before_exit()
{
}

void master_coroutine_server::run(int argc, char ** argv)
{
    if (flag_run) {
        zcc_fatal("master_coroutine_server:: only run one time");
    }
    flag_run = true;
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

    coroutine_base_init();

    if (default_config.get_bool("MASTER", false)) {
        close_on_exec(var_master_master_status_fd);
        nonblocking(var_master_master_status_fd);
        coroutine_enable_fd(var_master_master_status_fd);
        coroutine_go(monitor_reload_signal, 0);
        if (!empty(stop_file)) {
            coroutine_go(stop_file_check, 0);
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

    coroutine_base_loop();
    coroutine_base_fini();

    before_exit();
}

}
