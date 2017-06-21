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

class postproxy_finder: public basic_finder
{
public:
    postproxy_finder();
    ~postproxy_finder();
    inline bool pthread_safe() { return false; }
    bool open(const char *url);
    ssize_t find(const char *query, string &result, long timeout);
    void disconnect();
    bool connect(long timeout);
private:
    const char *___postfix_dict;
    const char *___prefix;
    const char *___suffix;
    iostream *___fp;
    int ___fd;
};

postproxy_finder::postproxy_finder()
{
    ___fp = 0;
    ___fd = -1;
}

postproxy_finder::~postproxy_finder()
{
    disconnect();
}

bool postproxy_finder::open(const char *url)
{
    parse_url(url);
    ___postfix_dict = kv_pairs.find("dictname", "");
    if (empty(___postfix_dict)) {
        return false;
    }
    ___prefix = kv_pairs.find("prefix", "");
    ___suffix = kv_pairs.find("suffix", "");
    if (!___destination) {
        return false;
    }
    return true;
}

ssize_t postproxy_finder::find(const char *query, string &result, long timeout)
{
#define ___TRIM_RN(mystr) { \
    const char *p = (mystr).c_str(); size_t len = (mystr).size(); \
    if ((len>0)&&(p[len-1] =='\n')) len--; \
    if ((len>0)&&(p[len-1] =='\r')) len--; \
    mystr.resize(len); \
}
    int i, ret, status;
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
        ___fp->puts("request");
        ___fp->put('\0');
        ___fp->puts("lookup");
        ___fp->put('\0');

        ___fp->puts("table");
        ___fp->put('\0');
        ___fp->puts(___postfix_dict);
        ___fp->put('\0');

        ___fp->puts("flags");
        ___fp->put('\0');
        sprintf(error_buf, "%d", (1 << 6));
        ___fp->puts(error_buf);
        ___fp->put('\0');

        ___fp->puts("key");
        ___fp->put('\0');
        ___fp->puts(query);
        ___fp->put('\0');

        ___fp->put('\0');

        ___fp->flush();
        if (___fp->is_error()) {
            snprintf(error_buf, 1024, "finder: %s : write error((%m)", ___url);
            result = error_buf;
            continue;
        }

        mystr.clear();
        ret = ___fp->gets(&mystr, '\0');
        if ((ret != 7) || (strcmp(mystr.c_str(), "status"))) {
            snprintf(error_buf, 1024, "finder: %s : read error, need status name", ___url);
            result = error_buf;
            disconnect();
            return -1;
        }

        mystr.clear();
        ret = ___fp->gets(&mystr, '\0');
        if ((ret != 2)) {
            snprintf(error_buf, 1024, "finder: %s : read error, need status value", ___url);
            result = error_buf;
            disconnect();
            return -1;
        }
        status = atoi(mystr.c_str());

        mystr.clear();
        ret = ___fp->gets(&mystr, '\0');
        if ((ret != 6) || (strcmp(mystr.c_str(), "value"))) {
            snprintf(error_buf, 1024, "finder: %s : read error, need value name", ___url);
            result = error_buf;
            disconnect();
            return -1;
        }
        mystr.clear();
        ret = ___fp->gets(result, '\0');
        if ((ret<0)) {
            snprintf(error_buf, 1024, "finder: %s : read error, need status value", ___url);
            result = error_buf;
            disconnect();
            return -1;
        }
        if (___fp->get() < 0) {
            snprintf(error_buf, 1024, "finder: %s : read error, need end", ___url);
            result = error_buf;
            disconnect();
            return -1;
        }
        if (status == 0) {
            return 1;
        }
        if (status == 1) {
            return 0;
        }
        snprintf(error_buf, 1024, "finder: %s : read error, postproxy, return %d", ___url, status);
        result = error_buf;
        disconnect();
        return -1;
    }

    return -1;
}

void postproxy_finder::disconnect()
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

bool postproxy_finder::connect(long timeout)
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


basic_finder *postproxy_finder_create()
{
    return new postproxy_finder();
}

}
