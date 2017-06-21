/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-10
 * ================================
 */

#include "zcc.h"
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>

static void ___usage(const char *arg)
{
    (void)arg;
    printf("USAGE: %s\n", zcc::var_progname);
}

namespace zcc
{

void (*master_load_server_config_fn)(vector<config *> &cfs) = 0;
#define zdebug(fmt, args...) {if(debug_mode){log_info(fmt, ##args);}}

typedef struct server_entry_t server_entry_t;
typedef struct listen_pair_t listen_pair_t;
typedef struct child_status_t child_status_t;

struct server_entry_t {
    char *config_fn;
    char *cmd;
    char *module;
    int proc_limit;
    int proc_count;
    dict *listens;
    bool listen_on;
    bool wait_on;
    event_timer wait_timer;
};

struct listen_pair_t {
    bool used;
    char *service_name;
    int fd;
    int iuf;
    event_io listen_ev;
    server_entry_t *server;
};

struct child_status_t {
    server_entry_t *server;
    int fd;
    event_io status_ev;
    long stamp;
};
static bool debug_mode = 0;
static int sighup_on = 0;
static char *config_path = 0;

static int master_status_fd[2];
static dict *listen_pair_dict;
static dict *child_status_dict;
static vector<server_entry_t *> *server_vector;


static void disable_server_listen(server_entry_t *server)
{
    zdebug("master: disable_server_listen server:%s, listen_one:%d", server->cmd, server->listen_on);
    if (!server->listen_on) {
        return;
    }
    server->listen_on = false;

    for(dict::node *n = server->listens->first_node(); n ; n = n->next()) {
        listen_pair_t *lp = (listen_pair_t *)(n->value());
        lp->listen_ev.disable();
    }
}

static void start_one_child(event_io &ev);
static void enable_server_listen(server_entry_t *server)
{
    zdebug("master: enable_server_listen server:%s, listen_one:%d", server->cmd, server->listen_on);
    if (server->listen_on) {
        return;
    }
    server->listen_on = true;

    for(dict::node *n = server->listens->first_node(); n ; n = n->next()) {
        listen_pair_t *lp = (listen_pair_t *)(n->value());
        lp->listen_ev.set_context(lp);
        lp->listen_ev.enable_read(start_one_child);
    }
}

static void child_exception_over(event_timer &tm)
{
    zdebug("master: child_exception_over");
    server_entry_t *server = (server_entry_t *)(tm.get_context());
    enable_server_listen(server);
}

static void child_strike(event_io &ev)
{
    child_status_t *cs = (child_status_t *)(ev.get_context());
    server_entry_t *server = cs->server;
    zdebug("master: child_strike, cmd:%s", server->cmd);
    if (timeout_set(0) - cs->stamp < 100) {
        if (!server->wait_on) {
            zdebug("master: child_strike, cmd:%s, exception", server->cmd);
            server->wait_timer.set_context(server);
            server->wait_timer.start(child_exception_over, 100);
            server->wait_on = true;
        }
    }

    ev.fini();
    free(cs);
    server->proc_count--;
    if ((server->proc_count  < server->proc_limit) && (server->wait_on==false)) {
        enable_server_listen(server);
    }
}

static void start_one_child(event_io &ev)
{
    char intbuf[32];
    listen_pair_t *lp = (listen_pair_t *)(ev.get_context());
    server_entry_t *server = lp->server;
    zdebug("master: start_one_child, cmd:%s", server->cmd);

    if (ev.exception()) {
        msleep(100);
        return;
    }
    if (server->wait_on) {
        disable_server_listen(server);
        return;
    }
    if (server->proc_count >= server->proc_limit) {
        disable_server_listen(server);
        return;
    }

    int status_fd[2];
    if (pipe(status_fd) == -1) {
        log_fatal("master: pipe (%m)");
    }
    pid_t pid = fork();
    if (pid == -1) {
        close(status_fd[0]);
        close(status_fd[1]);
        msleep(100);
        return;
    } else if (pid) {
        server->proc_count++;
        if (server->proc_count >= server->proc_limit) {
            disable_server_listen(server);
        }
        close(status_fd[0]);
        child_status_t *cs = (child_status_t *)calloc(1, sizeof(child_status_t));
        sprintf(intbuf, "%d", status_fd[1]);
        child_status_dict->update(intbuf, cs);
        cs->server = server;
        cs->fd = status_fd[1];
        cs->status_ev.init(status_fd[1]);
        cs->status_ev.set_context(cs);
        cs->status_ev.enable_read(child_strike);
        cs->stamp = timeout_set(0);
        return;
    } else {
        argv exec_argv;
        exec_argv.init(128);
        char buf[4100];

        close(status_fd[1]);
        dup2(status_fd[0], var_master_server_status_fd);
        close(status_fd[0]);

        close(master_status_fd[0]);
        dup2(master_status_fd[1], var_master_master_status_fd);
        close(master_status_fd[1]);

        exec_argv.add(server->cmd);
        exec_argv.add("-M");

        if (config_path) {
            snprintf(buf, 4096, "%s/main.cf", config_path);
            if (file_get_size(buf) > -1) {
                exec_argv.add("--c");
                exec_argv.add(buf);
            }
            if (server->config_fn) {
                exec_argv.add("--c");
                snprintf(buf, 4096, "%s/%s", config_path, server->config_fn);
                exec_argv.add(buf);
            }
        }
        if (!empty(server->module)) {
            exec_argv.add("--module");
            exec_argv.add(server->module);
        }

        int fdnext = var_master_server_listen_fd;
        for(dict::node *n = server->listens->first_node(); n ; n = n->next()) {
            char iuffd[111];
            listen_pair_t *lp2 = (listen_pair_t *)(n->value());
            dup2(lp2->fd, fdnext);
            close(lp2->fd);
            snprintf(iuffd, 100, "-s%s", lp->service_name);
            exec_argv.add(iuffd);
            sprintf(iuffd, "%c%d", lp2->iuf, fdnext);
            exec_argv.add(iuffd);
            fdnext++;
        }

        execvp(server->cmd, (char **)(memdup(exec_argv.data(), (exec_argv.size() + 1) * sizeof(char *))));

        log_fatal("master: start child error: %m");
    }
}

static void remove_old_child(void)
{
    child_status_t *cs;
    dict::node *n;
    while((n = child_status_dict->first_node())) {
        cs = (child_status_t *)(n->value());
        cs->status_ev.fini();
        close(cs->fd);
        free(cs);
        child_status_dict->erase(n);
    }
    delete child_status_dict;
    child_status_dict = new dict();
}

static void remove_server_entry(void)
{
    server_entry_t *sv;

    for(size_t i = 0; i < server_vector->size(); i++) {
        sv = (server_entry_t *) (*server_vector)[i];
        free(sv->config_fn);
        free(sv->cmd);
        free(sv->module);
        delete sv->listens;
        sv->wait_timer.fini();
        free(sv);
    }

    delete server_vector;
    server_vector = new vector<server_entry_t *>();
}

static void set_listen_unused(void)
{
    for (dict::node *n=listen_pair_dict->first_node();n;n=n->next()) {
        listen_pair_t *lp = (listen_pair_t *)(n->value());
        lp->used = 0;
        lp->listen_ev.fini();
        lp->server = 0;
    };
}

static void prepare_server_by_config(config *cf)
{
    char *cmd, *listen, *fn, *module;
    server_entry_t *server;
    cmd = cf->get_str("zcmd", "");
    listen = cf->get_str("zlisten", "");
    if (empty(cmd) || empty(listen)) {
        return;
    }
    fn = cf->get_str("z___Z_0428_fn", 0);
    module = cf->get_str("zmodule", 0);
    server = (server_entry_t *)calloc(1, sizeof(server_entry_t));
    server_vector->push_back(server);
    server->listen_on = true;
    server->wait_timer.init();
    server->config_fn = (fn ? strdup(fn) : 0);
    server->cmd = strdup(cmd);
    server->module = strdup(module);
    server->proc_limit = cf->get_int("zproc_limit", 1, 1, 1000);
    server->proc_count = 0;
    server->listens = new dict();

    char *service, lbuf[1024];
    listen_pair_t *lp;
    argv listens;
    listens.split(listen, ";, \t");
    for (size_t i = 0; i < listens.size(); i++) {
        service = (char *)listens[i];
        snprintf(lbuf, 1000, "%s", service);
        service = lbuf;
        listen = strstr(service, "://");
        if (!listen) {
            listen = service;
            service = blank_buffer;
        } else {
            *listen = 0;
            listen += 3;
        }

        if (!listen_pair_dict->find(listen, (char **)&lp)) {
            lp = (listen_pair_t *)calloc(1, sizeof(listen_pair_t));
            listen_pair_dict->update(listen, lp);
            lp->fd = zcc::listen(listen, 5, (int *)&(lp->iuf));
            if (lp->fd < 0) {
                log_fatal("master: open %s error\n");
            }
            close_on_exec(lp->fd);
        }
        if (lp->used) {
            log_fatal("master: open %s twice", listen);
        }
        lp->used = true;
        free(lp->service_name);
        lp->service_name = strdup(service);
        lp->server = server;
        lp->listen_ev.init(lp->fd);
        lp->listen_ev.set_context(lp);
        lp->listen_ev.enable_read(start_one_child);
        char intbuf[32];
        sprintf(intbuf, "%d", lp->fd);
        server->listens->update(intbuf, lp);
    }
}

void master_load_server_config_by_dir(const char *config_path, vector<config *> &cfs)
{
    DIR *dir;
    struct dirent ent, *ent_list;
    char *fn, *p;
    char pn[4100];
    config *cf;

    dir = opendir(config_path);
    if (!dir) {
        log_fatal("master: open %s/(%m)", config_path);
    }

    while ((!readdir_r(dir, &ent, &ent_list)) && (ent_list)) {
        fn = ent.d_name;
        if (fn[0] == '.') {
            continue;
        }
        p = strrchr(fn, '.');
        if ((!p) || (strcasecmp(p + 1, "cf"))) {
            continue;
        }
        if (!strcasecmp(fn, "main.cf")) {
            continue;
        }

        cf = new config();
        snprintf(pn, 4096, "%s/%s", config_path, fn);
        cf->load_from_filename(pn);
        cf->update("z___Z_0428_fn", fn);
        cfs.push_back(cf);
    }
    closedir(dir);
}

static void reload_config(void)
{
    config *cf;
    vector<config *> cfs;

    if (master_load_server_config_fn) {
        master_load_server_config_fn(cfs);
    } else {
        master_load_server_config_by_dir(config_path, cfs);
    }

    for (size_t i = 0; i < cfs.size(); i++) {
        cf = (config *)(cfs[i]);
        prepare_server_by_config(cf);
        delete cf;
    }
}

static void release_unused_listen(void)
{
    vector<dict::node *> vec;
    listen_pair_t *lp;

    for (dict::node *n = listen_pair_dict->first_node(); n; n = n->next()) {
        lp = (listen_pair_t *)(n->value());
        if (lp->used) {
            continue;
        }
        vec.push_back(n);
    }

    for (size_t i = 0; i < vec.size(); i++) {
        dict::node *n = (dict::node *)vec[i];
        lp = (listen_pair_t *)(n->value());
        close(lp->fd);
        free(lp->service_name);
        free(lp);
        listen_pair_dict->erase(n);
    }
}

static void reload_server(void)
{
    remove_old_child();
    remove_server_entry();
    set_listen_unused();
    reload_config();
    release_unused_listen();
}

static int master_lock_pfile(char *lock_file)
{
    int lock_fd;
    char pid_buf[33];

    lock_fd = open(lock_file, O_RDWR | O_CREAT, 0666);
    if (lock_fd < 0) {
        return 0;
    }
    close_on_exec(lock_fd, 1);

    if (flock(lock_fd, LOCK_EX | LOCK_NB)) {
        close(lock_fd);
        return 0;
    } else {
        sprintf(pid_buf, "%d          ", getpid());
        if (write(lock_fd, pid_buf, 10) != 10) {
            close(lock_fd);
            return 0;
        }
        return 1;
    }

    return 0;
}

static void sighup_handler(int sig)
{
    sighup_on = sig;
}

static void init_all(int argc, char **argv)
{
    int len;
    char *lock_file = 0;
    int try_lock = 0;

    ZCC_PARAMETER_BEGIN() {
        if (!strcmp(optname, "-t")) {
            try_lock = 1;
            opti+=1;
            continue;
        }
        if (!strcmp(optname, "-d")) {
            debug_mode = 1;
            opti+=1;
            continue;
        }
        if (!optval) {
            ___usage(optname);
        }
        if (!strcmp(optname, "-c")) {
            config_path = optval;
            opti+=2;
            continue;
        }
        if (!strcmp(optname, "-p")) {
            lock_file = optval;
            opti+=2;
            continue;
        }
    } ZCC_PARAMETER_END;

    if (empty(lock_file)) {
        lock_file = (char *)"master.pid";
    }
    if (try_lock) {
        if (master_lock_pfile(lock_file)) {
            exit(0);
        } else {
            exit(1);
        }
    }

    if (!master_lock_pfile(lock_file)) {
        exit(1);
    }

    if (config_path) {
        config_path = strdup(config_path);
        len = strlen(config_path);
        if (len < 1) {
            log_fatal("master: config dir is blank");
        }
        if (config_path[len - 1] == '/') {
            config_path[len - 1] = 0;
        }
    } else if (master_load_server_config_fn == 0) {
        log_fatal("master: need service config path");
    }

    /* SIG */
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    /* SIG RELOAD */
    struct sigaction sig;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = 0;
    sig.sa_handler = sighup_handler;
    if (sigaction(SIGHUP, &sig, (struct sigaction *)0) < 0) {
        log_fatal("%s: sigaction(%d) : %m", __FUNCTION__, SIGHUP);
    }

    /* MASTER STATUS */
    if (pipe(master_status_fd) == -1) {
        log_fatal("master: pipe : %m");
    }
    close_on_exec(master_status_fd[0], 1);


    /* VAR */
    listen_pair_dict = new dict();
    child_status_dict = new dict();
    server_vector = new vector<server_entry_t *>();
}

static void fini_all(void)
{
    remove_old_child();
    delete child_status_dict;

    remove_server_entry();
    delete server_vector;

    set_listen_unused();
    release_unused_listen();
    delete listen_pair_dict;

    free(config_path);

}

int master_main(int argc, char **argv)
{
    init_all(argc, argv);
    reload_server();
    sighup_on = 0;
    long t1 = timeout_set(0);
    while (1) {
        default_evbase.dispatch();
        if (sighup_on) {
            zdebug("master_reload");
            sighup_on = 0;
            reload_server();
        }
        if (timeout_set(0) - t1 > 60 * 1000) {
            break;
        }
    }
    fini_all();

    return 0;
}

}
