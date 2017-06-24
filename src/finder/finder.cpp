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

basic_finder *(*finder_create_extend_fn)(const char *method, const char *url) = 0;

basic_finder::basic_finder()
{
}

bool basic_finder::parse_url(const char *url, std::string &destination, zcc::dict &parameters)
{
    char buf[10240 + 1];
    char *ps, *p;
    strtok splitor;
    int len;

    len = strlen(url);
    if (len > 10240) {
        return false;
    }
    memcpy(buf, url, len);
    buf[len] = 0;

    ps = buf;
    p = strstr(ps, "://");
    if (!p) {
        return false;
    }
    ps = p+3;

    p = strchr(ps, '?');
    if (p) {
        *p++ = 0;
    }
    destination = ps;
    if (!p) {
        return true;
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
        parameters.update(ps, p);
    }
    return true;
}

/* @##################################################################@ */
finder::finder()
{
    ___fder = 0;
    ___uppercase = ___lowercase =false;
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
    ___finder_type_create(cdb);

    if ((!fder) && finder_create_extend_fn) {
        fder = finder_create_extend_fn(type, url);
    }
    if (!fder) {
        return false;
    }

    if (!fder->open(url)) {
        delete fder;
        return false;
    }
    ___fder = fder;
    return true;
}

ssize_t finder::find(const char *query, std::string &result, long timeout)
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

    return ___fder->find(nq, result, timeout);
}

void finder::close()
{
    if (___fder) {
        delete ___fder;
        ___fder = 0;
    }
}

/* ################################################## */
ssize_t finder_once(const char *url, const char *query, std::string &result, long timeout)
{
    finder fder;
    if (!fder.open(url)) {
        return -1;
    }
    return fder.find(query, result, timeout);
}

}
