/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-09-05
 * ================================
 */

#include "zcc.h"

namespace zcc
{

class redis_finder: public basic_finder
{
public:
    redis_finder();
    ~redis_finder();
    inline bool pthread_safe() { return false; }
    bool open(const char *url);
    ssize_t find(const char *query, string &result, long timeout);
    void disconnect();
    bool connect(long timeout);
private:
    const char *___key;
    const char *___prefix;
    const char *___suffix;
    iostream *___fp;
    int ___fd;
};

redis_finder::redis_finder()
{
    ___fp = 0;
    ___fd = -1;
}

redis_finder::~redis_finder()
{
    disconnect();
}

bool redis_finder::open(const char *url)
{
    parse_url(url);
    ___key = kv_pairs.find("key", "");
    if (empty(___key)) {
        return false;
    }
    ___prefix = kv_pairs.find("prefix", "");
    ___suffix = kv_pairs.find("suffix", "");
    if (!___destination) {
        return false;
    }
    return true;
}

ssize_t redis_finder::find(const char *query, string &result, long timeout)
{
#define ___TRIM_RN(mystr) { \
    const char *p = (mystr).c_str(); size_t len = (mystr).size(); \
    if ((len>0)&&(p[len-1] =='\n')) len--; \
    if ((len>0)&&(p[len-1] =='\r')) len--; \
    mystr.resize(len); \
}
    int i, len;
    long dtime = timeout_set(timeout);
    char error_buf[1024 + 1];
    string mystr;

    for (i = 0; i < 2; i++) {
        result.clear();
        if (i) {
            disconnect();
        }
        if (connect(timeout_left(dtime)) == false) {
            snprintf(error_buf, 1024, "finder: %s : connection error((%m)", ___url);
            result = error_buf;
            continue;
        }
        ___fp->set_timeout(timeout_left(dtime));
        ___fp->printf_1024("hget %s %s%s%s\r\n", ___key, ___prefix, query, ___suffix);
        ___fp->flush();
        if (___fp->is_error()) {
            snprintf(error_buf, 1024, "finder: %s : write error((%m)", ___url);
            result = error_buf;
            continue;
        }

        mystr.clear();
        ___fp->gets(mystr);
        if (___fp->is_error()) {
            snprintf(error_buf, 1024, "finder: %s : read error", ___url);
            result = error_buf;
            disconnect();
            return -1;
        }
        ___TRIM_RN(mystr);
        if (!strcmp(mystr.c_str(), "$-1")) {
            return 0;
        }
        if (*(mystr.c_str()) != '$') {
            snprintf(error_buf, 1024, "finder: %s : read error, NEED $", ___url);
            result = error_buf;
            disconnect();
            return -1;
        }

        len = atoi(mystr.c_str() + 1);
        if (len > 1024*1024) {
            snprintf(error_buf, 1024, "finder: %s : read error, line too long: %d", ___url, len);
            result = error_buf;
            disconnect();
            return -1;
        }
        if (len > 0) {
            if (___fp->readn(&result, len) != len) {
                snprintf(error_buf, 1024, "finder: %s : read result error", ___url);
                result = error_buf;
                return -1;
            }
        }
        mystr.clear();
        ___fp->gets(mystr);
        if (___fp->is_error()) {
            snprintf(error_buf, 1024, "finder: %s : read error", ___url);
            result = error_buf;
            return -1;
        }
        return 1;
    }

    return -1;
}

void redis_finder::disconnect()
{
    if (___fp) {
        delete ___fp;
        ___fp = 0;
    }
    if (___fd != -1) {
        close(___fd);
        ___fd = -1;
    }
}

bool redis_finder::connect(long timeout)
{
    if (___fp) {
        return true;
    }
    if (___fd <0) {
        ___fd = zcc::connect(___destination, timeout);
    }
    if (___fd < 0) {

        return false;
    }
    ___fp = new iostream(___fd);
    return true;
}


basic_finder *redis_finder_create()
{
    return new redis_finder();
}

}
