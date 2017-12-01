/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-10
 * ================================
 */

#include "zcc.h"
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>

namespace zcc
{

#define zdebug(fmt, args...) {if(var_log_debug_enable){zcc_info(fmt, ##args);}}

static void set_signal_handler();

/* LOG ############################################################ */

/* log {{{ */
static autobuffer var_masterlog_path;
static bool ___log_stop = false;
static bool var_masterlog_enable = false;
static char *var_masterlog_service = 0;
static const int var_masterlog_timeunit_day = 1;
static const int var_masterlog_timeunit_hour = 1;
static int var_masterlog_timeunit = 1;
static int var_masterlog_sock = -1;
static std::list<char *> var_masterlog_content_list;
static pthread_mutex_t var_masterlog_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t var_masterlog_cond = PTHREAD_COND_INITIALIZER;
struct log_stream{
    long hour_id;
    int fd;
    std::string cache;
};
typedef struct log_stream log_stream;
static std::map<std::string, log_stream *> var_masterlog_streams;
static std::map<std::string, log_stream *>::iterator var_masterlog_streams_iterator;

static void log_write_file(int fd, const char *content, size_t clen)
{
    size_t wrotelen = 0;
    while (clen > wrotelen) {
        int ret = write(fd, content + wrotelen, clen - wrotelen);
        if (ret >= 0) {
            wrotelen += ret;
            continue;
        }
        if (errno == EINTR) {
            continue;
        }
        break;
    }
}

static void log_save_content(char *logcontent) 
{
    char fpath[4096];
    log_stream *stream;
    char *fileid = logcontent;
    char *p = strchr(fileid, ',');
    if (!p) {
        return;
    }
    *p = 0;
    char *identity = p + 1;
    p = strchr(identity, ',');
    if (!p) {
        return;
    }
    *p = 0;
    char *pids = p + 1;
    p = strchr(pids, ',');
    if (!p) {
        return;
    }
    *p = 0;
    char *content = p + 1;

    time_t tl = time(0);
    struct tm tm;
    if (!localtime_r(&tl, &tm)) {
        return;
    }
    long hour_id;
    if (var_masterlog_timeunit == 1) {
        hour_id = tm.tm_year * (100 * 100 * 100) + tm.tm_mon * (100 * 100) + tm.tm_mday * 100 + tm.tm_hour;
    } else {
        hour_id = tm.tm_year * (100 * 100 * 100) + tm.tm_mon * (100 * 100) + tm.tm_mday * 100;
    }
    var_masterlog_streams_iterator = var_masterlog_streams.find(fileid);
    if (var_masterlog_streams_iterator == var_masterlog_streams.end()) {
        stream = new log_stream();
        stream->fd = -1;
        stream->hour_id = 0;
        stream->cache.reserve(1024 * 100);
        var_masterlog_streams[fileid] = stream;
    } else {
        stream = var_masterlog_streams_iterator->second;
    }
    if (stream->hour_id != hour_id) {
        if (stream->fd != -1) {
            if (stream->cache.size()) {
                log_write_file(stream->fd, stream->cache.c_str(), stream->cache.size());
            }
            close(stream->fd);
            stream->fd = -1;
        }
        stream->cache.clear();
    }
    stream->hour_id = hour_id;
    if (stream->fd == -1) {
        if (var_masterlog_timeunit == 1) {
            snprintf(fpath, 4096, "%s/%s-%d%02d%02d.log", var_masterlog_path.data, fileid, tm.tm_year+1900, tm.tm_mon + 1, tm.tm_mday); 
        }else {
            snprintf(fpath, 4096, "%s/%s-%d%02d%02d%02d.log", var_masterlog_path.data, fileid, tm.tm_year+1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour); 
        }
        while (((stream->fd = open(fpath, O_CREAT|O_APPEND|O_RDWR, 0666))==-1) && (errno == EINTR)) {
            msleep(100);
        }
        if (stream->fd == -1) {
            zcc_info("can not open %s (%m)", fpath);
            return;
        }
    }
    sprintf_1024(stream->cache, "%02d:%02d:%02d %s[%s] ", tm.tm_hour, tm.tm_min, tm.tm_sec, identity, pids);
    stream->cache.append(content);
    stream->cache.append("\n");
    if (stream->cache.size() > 1024 * (100 -1)) {
        log_write_file(stream->fd, stream->cache.c_str(), stream->cache.size());
        stream->cache.clear();
    }
}

static void log_flush_all()
{
    std_map_walk_begin(var_masterlog_streams, fileid, stream) {
        if ((stream->fd != -1) && (stream->cache.size())) {
            log_write_file(stream->fd, stream->cache.c_str(), stream->cache.size());
        }
        stream->cache.clear();
    } std_map_walk_end;

    if (___log_stop) {
        std_map_walk_begin(var_masterlog_streams, fileid, stream) {
            if (stream->fd != -1) {
                close(stream->fd);
            }
            delete stream;
        } std_map_walk_end;
        var_masterlog_streams.clear();
    }
}

static void *log_save_pthread(void *arg)
{
    set_signal_handler();
    struct timespec cond_timeout;
    long last_stamp = 0;
    char *logcontent;
    while(1) {
        cond_timeout.tv_sec = time(0) + 1;
        cond_timeout.tv_nsec = 300;
        pthread_cond_timedwait(&var_masterlog_cond, &var_masterlog_mutex, &cond_timeout);
        zcc_pthread_unlock(&var_masterlog_mutex);

        while (!var_masterlog_content_list.empty()) {
            zcc_pthread_lock(&var_masterlog_mutex);
            logcontent = var_masterlog_content_list.front();
            var_masterlog_content_list.pop_front();
            zcc_pthread_unlock(&var_masterlog_mutex);
            log_save_content(logcontent);
            free(logcontent);
        }

        if (time(0) > last_stamp) {
            log_flush_all();
            last_stamp = time(0) + 1;
        }
        if (___log_stop) {
            log_flush_all();
            break;
        }
    }
    return 0;
}

static void log_pthread_init()
{
    char *p;
    argv sv;
    sv.split(var_masterlog_service, ",");
    sv.push_back("");
    sv.push_back("");
    sv.push_back("");

    char *listen_path = sv[0];
    if (empty(listen_path)) {
        zcc_fatal("ERR -log-service %s, no listen", var_masterlog_service);
    }
    var_masterlog_listen = listen_path;

    var_masterlog_path.data = strdup(sv[1]);
    if (strlen(var_masterlog_path.data) < 1) {
        zcc_fatal("ERR -log-service %s, no path", var_masterlog_service);
    }

    if (var_masterlog_path.data[strlen(var_masterlog_path.data)-1] == '/') {
        var_masterlog_path.data[strlen(var_masterlog_path.data)-1] = 0;
    }

    p = sv[2];
    if (!strcmp(p, "hour")) {
        var_masterlog_timeunit = var_masterlog_timeunit_hour;
    } else if (!strcmp(p, "day")) {
        var_masterlog_timeunit = var_masterlog_timeunit_day;
    } else {
        var_masterlog_timeunit = var_masterlog_timeunit_day;
    }


    struct sockaddr_un server_un;
    if ((var_masterlog_sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        zcc_fatal("create socket error (%m)");
    }

    if (strlen(listen_path) >= (int)sizeof(server_un.sun_path)) {
        zcc_fatal("socket path too long: %s", listen_path);
    }

    memset(&server_un, 0, sizeof(struct sockaddr_un));
    server_un.sun_family = AF_UNIX;
    memcpy(server_un.sun_path, listen_path, strlen(listen_path) + 1);

    if ((var_masterlog_sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        zcc_fatal("create socket error (%m)");
    }

    if (unlink(listen_path) < 0 && errno != ENOENT) {
        zcc_fatal("unlink: %s(%m)", listen_path);
    }

    if (bind(var_masterlog_sock, (struct sockaddr *)&server_un, sizeof(struct sockaddr_un)) < 0) {
        zcc_fatal("bind %s (%m)", listen_path);
    }

    pthread_t pth;
    if(pthread_create(&pth, 0, log_save_pthread, 0)) {
        zcc_fatal("pthread_create error (%m)");
    }
}

static void *log_recv_pthread(void *arg)
{
    set_signal_handler();
    char logbuf[10240 + 1];
    while(1) {
        int num = recvfrom(var_masterlog_sock, logbuf, 10240, 0, 0, 0);
        if (num == 0) {
            continue;
        }
        if (num < 0) {
            int ec = errno;
            if (ec == EINTR) {
                continue;
            }
            if (ec == EAGAIN) {
                timed_wait_readable(var_masterlog_sock, 1 * 1000);
                continue;
            }
            zcc_fatal("log sock error (%m)");
        }

        char *buf_insert = (char *)memdupnull(logbuf, num);
        zcc_pthread_lock(&var_masterlog_mutex);
        var_masterlog_content_list.push_back(buf_insert);
        zcc_pthread_unlock(&var_masterlog_mutex);
        pthread_cond_signal(&var_masterlog_cond);
        if (___log_stop) {
            break;
        }
    }

    return 0;
}
/* }}} */

/* MASTER ######################################################### */
class server_info;
class listen_pair;
class child_status;

class server_info
{
public:
    inline server_info(){ proc_limit = proc_count = 0; stamp_next_start = 0; }
    inline ~server_info() { }
    void start_all();
    std::string config_fn;
    std::string cmd;
    int proc_limit;
    int proc_count;
    long stamp_next_start;
    std::map<int, listen_pair *> listens;
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

static int sighup_reload_on = 0;
static int sigterm_stop_on = 0;
std::string config_path;

static int ___reload_sig = SIGHUP;
static int master_status_fd[2];
static std::map<std::string, listen_pair *> listen_pair_map;
static std::map<int, child_status *> child_status_map;
static std::list<server_info *> server_vector;

static void start_one_child(server_info *server);

static bool ___load_server_config_flag = false;
static bool ___event_loop_flag = true;

/* ########################################################### */
void server_info::start_all()
{
    int left = proc_limit - proc_count;
    long time_permit = timeout_set(0) - stamp_next_start;
    zdebug("master: start_all server:%s, left:%d, permit: %d", cmd.c_str(), left, (time_permit>0?1:0));

    if ((left < 1) || (time_permit <1)) {
        return;
    }

    for (int i = 0; i < left; i++) {
        start_one_child(this);
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
    if (fd != -1) {
        close(fd);
    }
}
void listen_pair::set_unused()
{
    used = false;
    server = 0;
}

/* ########################################################### */
child_status::child_status()
{
}

child_status::~child_status()
{
}

/* ########################################################### */
static void child_strike(event_io &ev)
{
    child_status *cs = (child_status *)(ev.get_context());
    server_info *server = cs->server;
    zdebug("master: child_strike, cmd:%s", server->cmd.c_str());
    if (timeout_set(0) - cs->stamp < 100) {
        server->stamp_next_start  = timeout_set(0) + 100;
    } else if (timeout_set(0) - cs->stamp > 1000) {
        server->stamp_next_start  = 0;
    }

    server->proc_count--;
    int fd = cs->fd;
    child_status_map.erase(fd);
    delete cs;
    close(fd);
}

static void start_one_child(server_info *server)
{
    zdebug("master: start_one_child, cmd:%s", server->cmd.c_str());

    if (server->proc_count >= server->proc_limit) {
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
        close(status_fd[0]);
        child_status *cs = new child_status();
        cs->server = server;
        cs->fd = status_fd[1];
        cs->status_ev.bind(status_fd[1]);
        cs->status_ev.set_context(cs);
        cs->status_ev.enable_read(child_strike);
        cs->stamp = timeout_set(0);
        child_status_map[cs->fd] = cs;
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
        exec_argv.push_back("--MASTER");

        if (!config_path.empty()) {
            exec_argv.push_back("-server-config-path");
            exec_argv.push_back(config_path.c_str());

            if (!server->config_fn.empty()) {
                exec_argv.push_back("-config");
                snprintf(buf, 4096, "%s/%s", config_path.c_str(), server->config_fn.c_str());
                exec_argv.push_back(buf);
            }
        }

        if (var_masterlog_enable) {
            exec_argv.push_back("-master-log-listen");
            exec_argv.push_back(var_masterlog_listen.c_str());
        }

        int fdnext = var_master_server_listen_fd;
        std::string service_fork;
        std_map_walk_begin(server->listens, fd, lp2) {
            dup2(lp2->fd, fdnext);
            close(lp2->fd);
            sprintf_1024(service_fork, "%s:%c%d,", lp2->service_name.c_str(), lp2->iuf, fdnext);
            fdnext++;
        } std_map_walk_end;
        if (service_fork.size() > 1) {
            service_fork.resize(service_fork.size()-1);
            exec_argv.push_back("-server-service");
            exec_argv.push_back(service_fork.c_str());
        }

        execvp(server->cmd.c_str(), (char **)(memdup(exec_argv.data(), (exec_argv.size() + 1) * sizeof(char *))));

        zcc_fatal("master: start child error: %m");
    }
}

static void remove_old_child(master &ms)
{
    std_map_walk_begin(child_status_map, pid, cs) {
        int fd = cs->fd;
        delete cs;
        close(fd);
    } std_map_walk_end;
    child_status_map.clear();
}

static void remove_server(master &ms)
{
    std_list_walk_begin(server_vector, info) {
        delete info;
    } std_list_walk_end;
    server_vector.clear();
}

static void set_listen_unused(master &ms)
{
    std_map_walk_begin(listen_pair_map, key, lp) {
        lp->set_unused();
    } std_map_walk_end;
}

static void prepare_server_by_config(config *cf)
{
    char *cmd, *listen, *fn;
    server_info *server;
    cmd = cf->get_str("server-command", "");
    listen = cf->get_str("server-service", "");
    if (empty(cmd) || empty(listen)) {
        return;
    }
    fn = cf->get_str("___Z_0428_fn", "");
    server = new server_info();
    server_vector.push_back(server);
    server->config_fn = fn;
    server->cmd = cmd;
    server->proc_limit = cf->get_int("server-proc-count", 1, 1, 1000);
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
        std::map<std::string, zcc::listen_pair*>::iterator pit = listen_pair_map.find(uri);
        if (pit == listen_pair_map.end()) {
            lp = new listen_pair();
            listen_pair_map[uri] = lp;
            lp->fd = zcc::listen(uri, (int *)&(lp->iuf));
            if (lp->fd < 0) {
                zcc_fatal("master: open %s error\n", uri);
            }
            close_on_exec(lp->fd);
        } else {
            lp = pit->second;
        }
        if (lp->used) {
            zcc_fatal("master: open %s twice", uri);
        }
        lp->used = true;
        lp->service_name = service;
        lp->server = server;
        server->listens[lp->fd] = lp;
    }
}

static void reload_config(master &ms)
{
    std::list<config *> cfs;

    ms.load_server_config(cfs);
    if (___load_server_config_flag) {
        if (config_path.empty()) {
            zcc_fatal("master: need service config path");
        }
        ms.load_server_config_from_dir(config_path.c_str(), cfs);
    }

    std_list_walk_begin(cfs, cf) {
        prepare_server_by_config(cf);
        delete cf;
    } std_list_walk_end;
}

static void release_unused_listen(master &ms)
{
    argv delete_list;
    std_map_walk_begin(listen_pair_map, key, lp) {
        if (lp->used) {
            continue;
        }
        delete_list.push_back(key);
        delete lp;
    } std_map_walk_end;
    zcc_argv_walk_begin(delete_list, key) {
        listen_pair_map.erase(key);
    } zcc_argv_walk_end;
}

static long ___next_start_all_child_stamp = 0;
static void start_all_child(master &ms)
{
    if (timeout_set(0) < ___next_start_all_child_stamp) {
        return;
    }
    std_list_walk_begin(server_vector, info) {
        info->start_all();
    } std_list_walk_end;
    ___next_start_all_child_stamp = timeout_set(0) + 100;
}

static void reload_server(master &ms)
{
    /* MASTER STATUS */
    close(master_status_fd[0]);
    close(master_status_fd[1]);
    if (pipe(master_status_fd) == -1) {
        zcc_fatal("master: pipe : %m");
    }
    close_on_exec(master_status_fd[0]);

    remove_old_child(ms);
    remove_server(ms);
    set_listen_unused(ms);
    reload_config(ms);
    release_unused_listen(ms);

    start_all_child(ms);
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

    if (::flock(lock_fd, LOCK_EX | LOCK_NB) < 0) {
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
    sighup_reload_on = sig;
}

static void sigterm_handler(int sig)
{
    sigterm_stop_on = sig;
}

static void set_signal_handler()
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    signal(SIGTERM, sigterm_handler);


    struct sigaction sig;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = 0;
    sig.sa_handler = sighup_handler;
    if (sigaction(___reload_sig, &sig, (struct sigaction *)0) < 0) {
        zcc_fatal("%s: sigaction(%d) : %m", __FUNCTION__, ___reload_sig);
    }
}

static bool ___init_flag = false;
static void init_all(master &ms, int argc, char **argv)
{
    char *lock_file = 0;
    bool try_lock = false;

    if (___init_flag) {
        zcc_fatal("master: master::run only permit be excuted once");
    }
    ___init_flag = true;

    main_parameter_run(argc, argv);

    try_lock = default_config.get_bool("try-lock", false); 
    config_path = default_config.get_str("C", "");
    lock_file = default_config.get_str("pid-file", "");
    var_masterlog_service = default_config.get_str("log-service");

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

    /* LOG SERVICE */
    if (var_masterlog_service && strlen(var_masterlog_service)) {
        var_masterlog_enable = true;
        log_pthread_init();
        pthread_t pth;
        if (pthread_create(&pth, 0, log_recv_pthread, 0)) {
            zcc_fatal("pthread_create error (%m)");
        }
    }

    /* SIG */
    set_signal_handler();

    /* MASTER STATUS */
    if (pipe(master_status_fd) == -1) {
        zcc_fatal("master: pipe : %m");
    }
    close_on_exec(master_status_fd[0]);

    /* SELF LOG */
    log_use_by(argv[0], default_config.get_str("server-log"));
}

static void fini_all(master &ms)
{
    ___log_stop = true;
    pthread_cond_signal(&var_masterlog_cond);

    remove_old_child(ms);
    remove_server(ms);
    set_listen_unused(ms);
    release_unused_listen(ms);
    msleep(1000);
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
    before_service_for_enduser();
    sighup_reload_on = 0;
    while (1) {
        if (sigterm_stop_on) {
            break;
        }
        if (___event_loop_flag) {
            event_loop();
        }
        default_evbase.dispatch(100);
        if (sighup_reload_on) {
            zdebug("master_reload");
            sighup_reload_on = 0;
            reload_server(*this);
        }
        start_all_child(*this);
    }
    fini_all(*this);
}

void master::load_server_config_from_dir(const char *config_path, std::list<config *> &cfs)
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
        cf->load_by_filename(pn);
        (*cf)["___Z_0428_fn"] =  fn;
        cfs.push_back(cf);
    }
    closedir(dir);
}

void master::load_server_config(std::list<config *> &cfs)
{
    ___load_server_config_flag = true;
}

void master::before_service()
{
}

void master::before_service_for_enduser()
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

/* Local variables:
* End:
* vim600: fdm=marker
*/
