/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-03-15
 * ================================
 */

#include "zcc.h"
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sqlite3.h>

namespace zcc
{

static void after_response(async_io &aio);

static char *sqlite3_proxy_filename = 0;
static int sqlite3_fd = 0;
static sqlite3 *sqlite3_handler = 0;
static pthread_mutex_t sqlite3_mutex = PTHREAD_MUTEX_INITIALIZER;

/* {{{ proxy */
static std::list<async_io *> proxy_list;
static pthread_mutex_t proxy_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t proxy_cond = PTHREAD_COND_INITIALIZER;

static void proxy_exec(async_io &aio)
{
    int err = 0;
    char *err_sql = 0;
    char *sql = (char *)aio.get_context();
    sql += sizeof(int) + 1;
    do {
        if (sqlite3_exec(sqlite3_handler, "BEGIN;", NULL, NULL, &err_sql) != SQLITE_OK) {
            err = 1;
            break;
        }
        if (sqlite3_exec(sqlite3_handler, sql, NULL, NULL, &err_sql) != SQLITE_OK) {
            err = 1;
            break;
        }
        if (sqlite3_exec(sqlite3_handler, "COMMIT;", NULL, NULL, &err_sql) != SQLITE_OK) {
            err = 1;
            break;
        }
    } while (0);
    if (err) {
        char *err_sql2 = 0;
        if (sqlite3_exec(sqlite3_handler, "ROLLBACK;", NULL, NULL, &err_sql2) != SQLITE_OK) {
            zcc_fatal("FATAL sqlite3 rollback");
        }
    }

    std::string obuf;
    obuf.clear();
    if (err) {
        obuf.push_back('E');
        obuf.append(err_sql);
    } else {
        obuf.push_back('O');
    }
    aio.cache_write_size_data(obuf.c_str(), obuf.size());
    aio.cache_flush(after_response, 0);
}

static void proxy_query(async_io &aio)
{
    int err = 0, status;
    sqlite3_stmt *sql_stmt = 0;
    int ncolumn, coi;
    char *sql = (char *)aio.get_context();
    int len = *((int *)sql);
    sql += sizeof(int) + 1;
    std::string obuf;
    do {
        if (sqlite3_prepare_v2(sqlite3_handler, sql, len, &sql_stmt, 0) != SQLITE_OK) {
            err = 1;
            break;
        }
        obuf.clear();
        ncolumn = sqlite3_column_count(sql_stmt);
        sprintf_1024(obuf, "O%d", ncolumn);
        aio.cache_write_size_data(obuf.c_str(), obuf.size());

        obuf.clear();
        while ((status = sqlite3_step(sql_stmt)) != SQLITE_DONE) {
            if (status != SQLITE_ROW) {
                err = 1;
                break;
            }

            obuf.clear();
            obuf.push_back('*');
            for (coi=0;coi<ncolumn;coi++) {
                char *d = (char *)sqlite3_column_blob(sql_stmt, coi);
                int l = sqlite3_column_bytes(sql_stmt, coi);
                size_data_escape(obuf, d, l);
            }
            aio.cache_write_size_data(obuf.c_str(), obuf.size());
        }
        if (err) {
            break;
        }
    } while(0);

    obuf.clear();
    obuf.push_back(err?'E':'O');
    if (err) {
        obuf.append(sqlite3_errmsg(sqlite3_handler));
    }
    aio.cache_write_size_data(obuf.c_str(), obuf.size());
    if (sql_stmt) {
        sqlite3_finalize(sql_stmt);
    }
    aio.cache_flush(after_response, 0);
}

static void *pthread_proxy(void *arg)
{
    struct timespec timeout;
    async_io *aio;

    while(1) {
        if (var_proc_stop) {
            return 0;
        }
        zcc_pthread_lock(&proxy_mutex);
        while(proxy_list.empty()) {
            timeout.tv_sec = time(0) + 10;
            timeout.tv_nsec = 0;
            pthread_cond_timedwait(&proxy_cond, &proxy_mutex, &timeout);
            if (var_proc_stop) {
                return 0;
            }
        }
        aio = proxy_list.front();
        proxy_list.pop_front();
        zcc_pthread_unlock(&proxy_mutex);
        
        char *p = (char *)aio->get_context();
        zcc_pthread_lock(&sqlite3_mutex);
        if (p[sizeof(int)] == 'E') {
            proxy_exec(*aio);
        } else {
            proxy_query(*aio);
        }
        zcc_pthread_unlock(&sqlite3_mutex);
        free(p);
    }

    return arg;
}
/* }}} */

/* {{{ log */
static std::list<char *> log_list;
static std::list<char *> log_list_tmp;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t log_cond = PTHREAD_COND_INITIALIZER;

static void proxy_log()
{
    if (var_proc_stop) {
        return;
    }
    zcc_pthread_lock(&sqlite3_mutex);
    if (log_list_tmp.size() == 0) {
        zcc_pthread_unlock(&sqlite3_mutex);
        return;
    }

    char *err_sql = 0;
    int sok = 1;
    if (sok && (sqlite3_exec(sqlite3_handler, "BEGIN;", NULL, NULL, &err_sql) != SQLITE_OK)) {
        sok = 0;
    }
    std_list_walk_begin(log_list_tmp, log) {
        if (sok && (sqlite3_exec(sqlite3_handler, log + sizeof(int) + 1, NULL, NULL, &err_sql) != SQLITE_OK)) {
            sok = 0;
        }
        free(log);
    } std_list_walk_end;
    if (sok) {
        if (sok && (sqlite3_exec(sqlite3_handler, "COMMIT;", NULL, NULL, &err_sql) != SQLITE_OK)) {
            sok = 0;
        }
    }
    if (!sok) {
        if (sqlite3_exec(sqlite3_handler, "ROLLBACK;", NULL, NULL, &err_sql) != SQLITE_OK) {
            zcc_fatal("FATAL sqlite3 rollback");
        }
    }

    zcc_pthread_unlock(&sqlite3_mutex);
    log_list_tmp.clear();
    if ((!sok) && err_sql) {
        zcc_info("sqlite3 proxy log error: %s", err_sql);
    }
}

static void *pthread_log(void *arg)
{
    struct timespec timeout;
    size_t cache_size = 0;

    while(1) {
        if (var_proc_stop) {
            return 0;
        }
        zcc_pthread_lock(&log_mutex);
        long last_log = timeout_set(0);
        while(log_list.empty()) {
            timeout.tv_sec = time(0) + 1;
            timeout.tv_nsec = 0;
            pthread_cond_timedwait(&log_cond, &log_mutex, &timeout);
            if (var_proc_stop) {
                return 0;
            }
            if (timeout_set(0) - last_log > 1024) {
                last_log = timeout_set(0);
                proxy_log();
                cache_size = 0;
            }
        }
        char *data;
        data = log_list.front();
        log_list.pop_front();
        zcc_pthread_unlock(&log_mutex);

        log_list_tmp.push_back(data);
        int len = *((int *)data);
        cache_size += len;
        if (cache_size > 1024 * 1024) {
            last_log = timeout_set(0);
            proxy_log();
            cache_size = 0;
        }
    }

    return arg;
}
/* }}} */

/* {{{ server */
static void release_aio(async_io &aio)
{
    int fd = aio.get_fd();
    delete &aio;
    close(fd);
}

static void after_get_request(async_io &aio)
{
    int ret = aio.get_result();
    if (ret < 2) {
        release_aio(aio);
        return;
    }
    char *data = (char *)malloc(sizeof(int) + ret+1);
    *((int*)data) = ret - 1;
    char *ptr = data + sizeof(int);
    aio.fetch_rbuf(ptr, ret);
    ptr[ret] = 0;
    if ((ptr[0] =='E') || (ptr[0] == 'Q')) {
        aio.set_context(data);
        zcc_pthread_lock(&proxy_mutex);
        proxy_list.push_back(&aio);
        zcc_pthread_unlock(&proxy_mutex);
        pthread_cond_signal(&proxy_cond);
        return;
    } else if (ptr[0] == 'L') {
        zcc_pthread_lock(&log_mutex);
        log_list.push_back(data);
        zcc_pthread_unlock(&log_mutex);
        pthread_cond_signal(&log_cond);
        return;
    } else {
        free(data);
        release_aio(aio);
        return;
    }
    return;
}

static void after_response(async_io &aio)
{
    if (aio.get_result() < 1) {
        release_aio(aio);
        return;
    }
    aio.read_size_data(after_get_request, 0);
}

sqlite3_proxyd::sqlite3_proxyd()
{
}

sqlite3_proxyd::~sqlite3_proxyd()
{
}

void sqlite3_proxyd::before_service()
{
    do {
        sqlite3_proxy_filename = zcc::default_config.get_str("sqlite3_proxy_filename", "");
        if(empty(sqlite3_proxy_filename)) {
            zcc_fatal("FATAL must set sqlite3_proxy_filename'value");
        }
        sqlite3_fd = open(sqlite3_proxy_filename, O_CREAT|O_RDWR, 0666);
        if (sqlite3_fd == -1) {
            zcc_fatal("FATAL open %s(%m)", sqlite3_proxy_filename);
        }
        zcc::flock_exclusive(sqlite3_fd);
        if (SQLITE_OK != sqlite3_open(sqlite3_proxy_filename, &sqlite3_handler)) {
            zcc_fatal("FATAL dbopen %s(%m)", sqlite3_proxy_filename);
        }
        zcc_info("sqlite3_proxy open %s", sqlite3_proxy_filename);
    } while(0);

    do {
        pthread_t pth1, pth2;
        pthread_create(&pth1, 0, pthread_proxy, 0);
        pthread_create(&pth2, 0, pthread_log, 0);
    } while(0);
}

void sqlite3_proxyd::before_exit()
{
    zcc_pthread_lock(&sqlite3_mutex);
    if (sqlite3_handler) {
        if (sqlite3_close(sqlite3_handler) != SQLITE_OK) {
            zcc_fatal("FATAL close sqlite %s(%s)", sqlite3_proxy_filename, sqlite3_errmsg(sqlite3_handler));
        }
    }
    zcc_pthread_lock(&sqlite3_mutex);
}

void sqlite3_proxyd::simple_service(int fd)
{
    async_io *aio = new async_io();
    aio->bind(fd);
    aio->read_size_data(after_get_request, 0);
}

/* }}} */

}

/* Local variables:
* End:
* vim600: fdm=marker
*/
