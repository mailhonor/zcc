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

#include <list>
#include <map>

static int (*___zcc_flock)(int fd, int operation) = flock;

static void ___usage()
{
    printf("USAGE: %s\n", zcc::var_progname);
    exit(1);
}

namespace zcc
{

#define zdebug(fmt, args...) {if(debug_mode){zcc_info(fmt, ##args);}}

class server_info;
class listen_pair;
class child_status;

class server_info
{
public:
    server_info(){ proc_limit = proc_count = 0; listen_on = wait_on = false; }
    ~server_info() { }
    void disable_listen();
    void enable_listen();
    std::string config_fn;
    std::string cmd;
    std::string module;
    int proc_limit;
    int proc_count;
    bool listen_on;
    bool wait_on;
    std::map<int, listen_pair *> listens;
    event_timer wait_timer;
};

class listen_pair
{
public:

    listen_pair();
    ~listen_pair();
    void set_unused();
    bool used;
    std::string service_name;
    int fd;
    int iuf;
    event_io listen_ev;
    server_info *server;
};

class child_status
{
public:
    child_status();
    ~child_status();
    server_info *server;
    int fd;
    event_io status_ev;
    long stamp;
};

static bool debug_mode = 0;
static int sighup_on = 0;
std::string config_path;

static int ___reload_sig = SIGHUP;
static int master_status_fd[2];
static std::map<std::string, listen_pair *> listen_pair_map;
static std::map<int, child_status *> child_status_map;
static std::list<server_info *> server_list;

static void start_one_child(event_io &ev);

static bool ___load_server_config_flag = false;
static bool ___event_loop_flag = true;

/* ########################################################### */
void server_info::disable_listen()
{
    zdebug("master: disable_listen server:%s, listen_on:%d", cmd.c_str(), listen_on?1:0);
    if (!listen_on) {
        return;
    }
    listen_on = false;

    std::map<int, listen_pair *>::iterator it;
    for (it = listens.begin(); it != listens.end(); it++) {
        it->second->listen_ev.disable();
    }
}

void server_info::enable_listen()
{
    zdebug("master: enable_listen server:%s, listen_one:%d", cmd.c_str(), listen_on?1:0);
    if (listen_on) {
        return;
    }
    listen_on = true;

    std::map<int, listen_pair *>::iterator it;
    for (it = listens.begin(); it != listens.end(); it++) {
        listen_pair *lp = it->second;
        lp->listen_ev.set_context(lp);
        lp->listen_ev.enable_read(start_one_child);
    }
}

/* ########################################################### */
listen_pair::listen_pair()
{
    used = false;
    fd = -1;
}

listen_pair::~listen_pair()
{
    listen_ev.fini();
    if (fd != -1) {
        close(fd);
    }
}
void listen_pair::set_unused()
{
    used = false;
    listen_ev.fini();
    server = 0;
}

/* ########################################################### */
child_status::child_status()
{
}

child_status::~child_status()
{
    status_ev.fini();
    close(fd);
}

/* ########################################################### */
static void child_exception_over(event_timer &tm)
{
    zdebug("master: child_exception_over");
    server_info *server = (server_info *)(tm.get_context());
    server->enable_listen();
}

static void child_strike(event_io &ev)
{
    child_status *cs = (child_status *)(ev.get_context());
    server_info *server = cs->server;
    zdebug("master: child_strike, cmd:%s", server->cmd.c_str());
    if (timeout_set(0) - cs->stamp < 100) {
        if (!server->wait_on) {
            zdebug("master: child_strike, cmd:%s, exception", server->cmd.c_str());
            server->wait_timer.set_context(server);
            server->wait_timer.start(child_exception_over, 100);
            server->wait_on = true;
        }
    }

    ev.fini();
    free(cs);
    server->proc_count--;
    if ((server->proc_count  < server->proc_limit) && (server->wait_on==false)) {
        server->enable_listen();
    }
}

static void start_one_child(event_io &ev)
{
    listen_pair *lp = (listen_pair *)(ev.get_context());
    server_info *server = lp->server;
    zdebug("master: start_one_child, cmd:%s", server->cmd.c_str());

    if (ev.exception()) {
        msleep(100);
        return;
    }
    if (server->wait_on) {
        server->disable_listen();
        return;
    }
    if (server->proc_count >= server->proc_limit) {
        server->disable_listen();
        return;
    }

    int status_fd[2];
    if (pipe(status_fd) == -1) {
        zcc_fatal("master: pipe (%m)");
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
            server->disable_listen();
        }
        close(status_fd[0]);
        child_status *cs = new child_status();
        child_status_map[status_fd[1]] = cs;
        cs->server = server;
        cs->fd = status_fd[1];
        cs->status_ev.init(status_fd[1]);
        cs->status_ev.set_context(cs);
        cs->status_ev.enable_read(child_strike);
        cs->stamp = timeout_set(0);
        return;
    } else {
        argv exec_argv;
        char buf[4100];

        close(status_fd[1]);
        dup2(status_fd[0], var_master_server_status_fd);
        close(status_fd[0]);

        close(master_status_fd[0]);
        dup2(master_status_fd[1], var_master_master_status_fd);
        close(master_status_fd[1]);

        exec_argv.push_back(server->cmd);
        exec_argv.push_back("-M");

        if (!config_path.empty()) {
            exec_argv.push_back("-global_config");
            exec_argv.push_back(config_path.c_str());

            if (!server->config_fn.empty()) {
                exec_argv.push_back("--c");
                snprintf(buf, 4096, "%s/%s", config_path.c_str(), server->config_fn.c_str());
                exec_argv.push_back(buf);
            }
        }
        if (!server->module.empty()) {
            exec_argv.push_back("--module");
            exec_argv.push_back(server->module);
        }

        int fdnext = var_master_server_listen_fd;
        std::map<int, listen_pair *>::iterator sit;
        for (sit = server->listens.begin(); sit != server->listens.end(); sit++) {
            char iuffd[111];
            listen_pair *lp2 = sit->second;
            dup2(lp2->fd, fdnext);
            close(lp2->fd);
            snprintf(iuffd, 100, "-s%s", lp->service_name.c_str());
            exec_argv.push_back(iuffd);
            sprintf(iuffd, "%c%d", lp2->iuf, fdnext);
            exec_argv.push_back(iuffd);
            fdnext++;
        }

        execvp(server->cmd.c_str(), (char **)(memdup(exec_argv.data(), (exec_argv.size() + 1) * sizeof(char *))));

        zcc_fatal("master: start child error: %m");
    }
}

static void remove_old_child(master &ms)
{
    child_status *cs;
    std::map<int, child_status *>::iterator it;
    for (it = child_status_map.begin(); it != child_status_map.end(); it++) {
        cs = (child_status *) it->second;
        delete cs;
    }
    child_status_map.clear();
}

static void remove_server(master &ms)
{
    std::list<server_info *>::iterator it;
    for (it = server_list.begin(); it != server_list.end(); it++) {
        delete(*it);
    }
    server_list.clear();
}

static void set_listen_unused(master &ms)
{
    std::map<std::string, listen_pair *>::iterator it;
    for (it = listen_pair_map.begin(); it != listen_pair_map.end(); it++) {
        listen_pair *lp = (listen_pair *)it->second;
        lp->set_unused();
    };
}

static void prepare_server_by_config(config *cf)
{
    char *cmd, *listen, *fn, *module;
    server_info *server;
    cmd = cf->get_str("zcmd", "");
    listen = cf->get_str("zlisten", "");
    if (empty(cmd) || empty(listen)) {
        return;
    }
    fn = cf->get_str("z___Z_0428_fn", "");
    module = cf->get_str("zmodule", "");
    server = new server_info();
    server_list.push_back(server);
    server->config_fn = fn;
    server->cmd = cmd;
    server->module = module;
    server->proc_limit = cf->get_int("zproc_limit", 1, 1, 1000);
    server->proc_count = 0;

    /* listens */
    strtok splitor;
    splitor.set_str(listen);
    while(splitor.tok(";, \t")) {
        char lbuf[1024];
        if (splitor.size() > 1000) {
            zcc_fatal("master: service url too long, %s", splitor.ptr());
        }
        memcpy(lbuf, splitor.ptr(), splitor.size());
        lbuf[splitor.size()] = 0;
        char *service = lbuf;
        char *uri = strstr(service, "://");
        if (!uri) {
            uri = service;
            service = blank_buffer;
        } else {
            *uri = 0;
            uri += 3;
        }

        listen_pair *lp;
        std::map<std::string, listen_pair *>::iterator lpit;
        lpit = listen_pair_map.find(uri);
        if (lpit == listen_pair_map.end()) {
            lp = new listen_pair();
            listen_pair_map[uri] = lp;
            lp->fd = zcc::listen(uri, 5, (int *)&(lp->iuf));
            if (lp->fd < 0) {
                zcc_fatal("master: open %s error\n", uri);
            }
            close_on_exec(lp->fd);
        } else {
            lp = lpit->second;
        }
        if (lp->used) {
            zcc_fatal("master: open %s twice", uri);
        }
        lp->used = true;
        lp->service_name = service;
        lp->server = server;
        lp->listen_ev.init(lp->fd);
        lp->listen_ev.set_context(lp);
        server->listens[lp->fd] = lp;
    }
    /* timer */
    server->wait_timer.init();

    /* */
    server->enable_listen();
}

static void reload_config(master &ms)
{
    config *cf;
    std::vector<config *> cfs;

    ms.load_server_config(cfs);
    if (___load_server_config_flag) {
        if (config_path.empty()) {
            zcc_fatal("master: need service config path");
        }
        ms.load_server_config_from_dir(config_path.c_str(), cfs);
    }

    std::vector<config *>::iterator it;
    for (it = cfs.begin(); it != cfs.end(); it ++) {
        cf = (config *)(*it);
        prepare_server_by_config(cf);
        delete cf;
    }
}

static void release_unused_listen(master &ms)
{
    std::list<std::string> delete_list;
    std::map<std::string, listen_pair *>::iterator it;
    for (it=listen_pair_map.begin();it!=listen_pair_map.end();it++) {
        listen_pair *lp = it->second;
        if (lp->used) {
            continue;
        }
        delete_list.push_back(it->first);
        delete lp;
    }
    std::list<std::string>::iterator it2;
    for (it2=delete_list.begin();it2!=delete_list.end();it2++) {
        listen_pair_map.erase(*it2);
    }
}

static void reload_server(master &ms)
{
    remove_old_child(ms);
    remove_server(ms);
    set_listen_unused(ms);
    reload_config(ms);
    release_unused_listen(ms);
}

static bool master_lock_pfile(char *lock_file)
{
    int lock_fd;
    char pid_buf[33];

    lock_fd = open(lock_file, O_RDWR | O_CREAT, 0666);
    if (lock_fd < 0) {
        zcc_info("master: open %s(%m)", lock_file);
        return false;
    }
    close_on_exec(lock_fd);

    if (___zcc_flock(lock_fd, LOCK_EX | LOCK_NB) < 0) {
        close(lock_fd);
        return false;
    } else {
        sprintf(pid_buf, "%d          ", getpid());
        if (write(lock_fd, pid_buf, 10) != 10) {
            close(lock_fd);
            return false;
        }
        return true;
    }

    return false;
}

static void sighup_handler(int sig)
{
    sighup_on = sig;
}

static bool ___init_flag = false;
static void init_all(master &ms, int argc, char **argv)
{
    char *lock_file = 0;
    int try_lock = 0;

    if (___init_flag) {
        zcc_fatal("master: master::run only permit be excuted once");
    }
    ___init_flag = true;

    zcc_main_parameter_begin() {
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
            ___usage();
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
    } zcc_main_parameter_end;

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

    if (!config_path.empty()) {
        if (config_path[config_path.size() - 1] == '/') {
            config_path.resize(config_path.size() - 1);
        }
    }

    /* SIG */
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    /* SIG RELOAD */
    struct sigaction sig;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = 0;
    sig.sa_handler = sighup_handler;
    if (sigaction(___reload_sig, &sig, (struct sigaction *)0) < 0) {
        zcc_fatal("%s: sigaction(%d) : %m", __FUNCTION__, ___reload_sig);
    }

    /* MASTER STATUS */
    if (pipe(master_status_fd) == -1) {
        zcc_fatal("master: pipe : %m");
    }
    close_on_exec(master_status_fd[0]);
}

static void fini_all(master &ms)
{
    remove_old_child(ms);
    remove_server(ms);
    set_listen_unused(ms);
    release_unused_listen(ms);
}


master::master()
{
}

master::~master()
{
}

void master::run(int argc, char **argv)
{
    init_all(*this, argc, argv);
    reload_server(*this);
    before_service();
    sighup_on = 0;
    while (1) {
        if (___event_loop_flag) {
            event_loop();
        }
        if (var_proc_timeout) {
            if (time(0) * 1000 > var_proc_timeout) {
                break;
            }
        }
        default_evbase.dispatch();
        if (sighup_on) {
            zdebug("master_reload");
            sighup_on = 0;
            reload_server(*this);
        }
    }
    fini_all(*this);
}

void master::load_server_config_from_dir(const char *config_path, std::vector<config *> &cfs)
{
    DIR *dir;
    struct dirent ent, *ent_list;
    char *fn, *p;
    char pn[4100];
    config *cf;

    dir = opendir(config_path);
    if (!dir) {
        zcc_fatal("master: open %s/(%m)", config_path);
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

        cf = new config();
        snprintf(pn, 4096, "%s/%s", config_path, fn);
        cf->load_from_filename(pn);
        cf->update("z___Z_0428_fn", fn);
        cfs.push_back(cf);
    }
    closedir(dir);
}

void master::load_server_config(std::vector<config *> &cfs)
{
    ___load_server_config_flag = true;
}

void master::before_service()
{
}

void master::event_loop()
{
    ___event_loop_flag = false;
}

void master::set_reload_signal(int sig)
{
    ___reload_sig = sig;
}

}
