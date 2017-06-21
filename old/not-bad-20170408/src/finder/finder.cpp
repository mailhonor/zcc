/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-12-05
 * ================================
 */

#include "zcc.h"
#include <pthread.h>

namespace zcc
{

basic_finder *(*finder_create_extend_fn)(const char *method, char *url) = 0;

basic_finder::basic_finder()
{
    ___url = ___destination = 0;
    ___flock = false;
}

void basic_finder::parse_url(const char *url)
{
    char buf[4096 + 1];
    str_dict kvp;
    strtok splitor;
    config::node *cnode;
    char *ps, *p;

    kvp.update("#u", url);
    snprintf(buf, 4096, "%s", url);
    ps = buf;
    p = strchr(ps, '?');
    if (p) {
        *p++ = 0;
    }
    kvp.update("#d", ps);
    if (!p) {
        goto ok_dict;
    }
    ps = p;

    splitor.set_str(ps);
    while (1) {
        if(!splitor.tok("&")) {
            break;
        }

        ps = splitor.ptr();
        ps[splitor.size()] = 0;
        p = strchr(ps, '=');
        if(p) {
            *p++ = 0;
        }
        kvp.update(ps, p);
    }

ok_dict:
    while((cnode = kvp.first_node())) {
        kv_pairs.add(cnode->key(), cnode->value());
        kvp.erase(cnode);
    }
    kv_pairs.over();

    ___url = kv_pairs.find("#u", 0);
    ___destination = kv_pairs.find("#d", 0);
}

/* @##################################################################@ */
finder::finder()
{
    ___fder = 0;
    ___flock = ___plock = ___uppercase = ___lowercase =false;
}

finder::~finder()
{
    close();
}

bool finder::open(const char *url_raw)
{
    char *url = const_cast <char *>(url_raw);
    char type[56], *p;
    basic_finder *fder = 0;
    int len;

    if (empty(url)) {
        return false;
    }

    p = strstr(url, "://");
    if (!p) {
        return false;
    }
    len = p - url;
    if ((len <1) || (len > 50)) {
        return false;
    }
    memcpy(type, url, len);
    type[len] = 0;
    url += len + 3;

#define ___finder_type_create(tn)  if(!fder) { \
    if(!strcmp(type, #tn)) { \
        basic_finder * tn ## _finder_create(); \
        fder = tn ## _finder_create(); \
    } }
    ___finder_type_create(static);
    ___finder_type_create(flat);
    ___finder_type_create(memcache);
    ___finder_type_create(redis);
    ___finder_type_create(socketline);
    ___finder_type_create(postproxy);

    if ((!fder) && finder_create_extend_fn) {
        fder = finder_create_extend_fn(type, url);
    }
    if (!fder) {
        return false;
    }
    fder->___flock = ___flock;

    if (!fder->open(url)) {
        delete fder;
        return false;
    }
    ___fder = fder;
    if (___plock && (!fder->pthread_safe())) {
        ___mutex = malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init((pthread_mutex_t *)___mutex, 0);
    }
    return true;
}

ssize_t finder::find(const char *query, string &result, long timeout)
{
    result.clear();
    if (!___fder) {
        return -1;
    }

    char *nq = const_cast<char *>(query);
    char buf[1024 + 2];
    size_t qlen = strlen(query);
    if (qlen > 1024) {
        return  -1;
    }
    if (___uppercase || ___lowercase) {
        memcpy(buf, query, qlen);
        buf[qlen] = 0;
        if (___lowercase) {
            to_lower(buf);
        } else if (___uppercase) {
            to_upper(buf);
        }
        nq = buf;
    }

    zcc_pthread_lock(___mutex);
    ssize_t ret = ___fder->find(nq, result, timeout);
    zcc_pthread_unlock(___mutex);

    return ret;
}

finder &finder::close()
{
    if (___fder) {
        delete ___fder;
        ___fder = 0;
    }
    if (___mutex) {
        pthread_mutex_destroy((pthread_mutex_t *)___mutex);
        free(___mutex);
        ___mutex = 0;
    }

    return *this;
}

/* ################################################## */
ssize_t finder_once(const char *url, const char *query, string &result, long timeout)
{
    finder fder;
    if (!fder.open(url)) {
        return -1;
    }
    return fder.find(query, result, timeout);
}

}

