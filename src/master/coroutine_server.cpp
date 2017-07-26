/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-20
 * ================================
 */

#include "zcc.h"
#include <signal.h>
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

static bool flag_init = false;
static bool flag_alone_mode = false;
static bool flag_run = false;

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

static void *monitor_reload_signal(void *arg)
{
    timed_wait_readable(var_master_master_status_fd, -1);
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
        flag_alone_mode = false;
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
        int fd = listen(url, &fd_type);
        if (fd < 0) {
            zcc_fatal("alone_register: open %s(%m)", alone_url);
        }
        close_on_exec(fd);
        nonblocking(fd);
        service_register(service, fd, fd_type);
    }
}

void master_coroutine_server::before_service()
{
}

void master_coroutine_server::before_exit()
{
}

void master_coroutine_server::run_begin(int argc, char ** argv)
{
    if (flag_run) {
        zcc_fatal("master_coroutine_server:: only run one time");
    }
    flag_run = true;
    flag_alone_mode = true;
    var_progname = argv[0];

    zcc::argv service_argv;
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
            service_argv.push_back(optname+2);
            service_argv.push_back(optval);
            opti += 2;
            continue;
        }
        if (!strcmp(optname, "-log-listen")) {
            var_masterlog_listen = optval;
            opti += 2;
            continue;
        }
        if (!strcmp(optname, "-stop-file")) {
            stop_file = optval;
            opti += 2;
            continue;
        }
    } zcc_main_parameter_end;

    if (!var_test_mode) {
        log_use_by_config(argv[0]);
    }

    coroutine_base_init();

    before_service();

    if (!flag_alone_mode) {
        close_on_exec(var_master_server_listen_fd);
        close_on_exec(var_master_master_status_fd);
        coroutine_go(monitor_reload_signal, 0);
        if (stop_file) {
            coroutine_go(stop_file_check, 0);
        }
    }

    if (!empty(var_listen_address)) {
        alone_register(var_listen_address);
    }

    for (size_t i = 0; i < service_argv.size(); i += 2) {
        char *s = service_argv[i];
        char *optval = service_argv[i+1];
        int t = optval[0];
        switch(t) {
            case var_tcp_listen_type_inet:
            case var_tcp_listen_type_unix:
            case var_tcp_listen_type_fifo:
                break;
            default:
                zcc_fatal("master_coroutine_server: unknown service type %c", t);
                break;
        }
        int fd = atoi(optval+1);
        if (fd < var_master_server_listen_fd) {
            zcc_fatal("master_coroutine_server: fd is invalid", optval+1);
        }
        close_on_exec(fd);
        nonblocking(fd);
        service_register(s, fd, t);
    }
    service_argv.clear();
}

void master_coroutine_server::run_loop()
{
    coroutine_base_loop();
    coroutine_base_fini();
}

void master_coroutine_server::run_over()
{
    before_exit();
}

void master_coroutine_server::run(int argc, char ** argv)
{
    run_begin(argc, argv);
    run_loop();
    run_over();
}

}
