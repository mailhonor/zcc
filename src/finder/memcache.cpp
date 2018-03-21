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

class memcache_finder: public basic_finder
{
public:
    memcache_finder();
    ~memcache_finder();
    bool open(const char *url);
    ssize_t find(const char *query, std::string &result, long timeout);
    void disconnect();
    bool connect(long timeout);
private:
    const char *___url;
    const char *___destination;
    const char *___prefix;
    const char *___suffix;
    stream ___fp;
    int ___fd;
};

memcache_finder::memcache_finder()
{
    ___fd = -1;
    ___url = 0;
}

memcache_finder::~memcache_finder()
{
    disconnect();
    free(___url);
}

bool memcache_finder::open(const char *url)
{
    http_url urlobj(url);
    if (urlobj.destination.empty()) {
        return false;
    }

    stringsdup sdup;
    sdup.push_back(url);
    sdup.push_back(urlobj.destination.c_str());
    sdup.push_back(dict_get_str(urlobj.query_variates, "prefix"));
    sdup.push_back(dict_get_str(urlobj.query_variates, "suffix"));

    ___url = sdup.dup();
    std::vector<size_t> &offsets = sdup.offsets();
    ___destination = ___url + offsets[1];
    ___prefix = ___url + offsets[2];
    ___suffix = ___url + offsets[3];
    return true;
}

ssize_t memcache_finder::find(const char *query, std::string &result, long timeout)
{
    if (timeout < 1) {
        timeout = var_max_timeout;
    }
#define ___TRIM_RN(mystr) { \
    const char *p = (mystr).c_str(); size_t len = (mystr).size(); \
    if ((len>0)&&(p[len-1] =='\n')) len--; \
    if ((len>0)&&(p[len-1] =='\r')) len--; \
    mystr.resize(len); \
}
    int i, len;
    long dtime = timeout_set(timeout);
    std::string mystr;

    for (i = 0; i < 2; i++) {
        result.clear();
        if (i) {
            disconnect();
        }
        if (connect(timeout_left(dtime)) == false) {
            result.clear();
            sprintf_1024(result, "finder: %s : connection error((%m)", ___url);
            continue;
        }
        ___fp.set_timeout(timeout_left(dtime));
        ___fp.printf_1024("get %s%s%s\r\n", ___prefix, query, ___suffix);
        ___fp.flush();
        if (___fp.is_exception()) {
            result.clear();
            sprintf_1024(result, "finder: %s : write error((%m)", ___url);
            continue;
        }

        mystr.clear();
        ___fp.gets(mystr);
        if (___fp.is_exception()) {
            result.clear();
            sprintf_1024(result, "finder: %s : read error", ___url);
            disconnect();
            return -1;
        }
        ___TRIM_RN(mystr);
        if (!strcmp(mystr.c_str(), "END")) {
            return 0;
        }

        if ((sscanf(mystr.c_str(), "VALUE %*s %*s %d", &len) < 1) || (len < 0)) {
            result.clear();
            sprintf_1024(result, "finder: %s : VALUE format error: %s", ___url, mystr.c_str());
            disconnect();
            return -1;
        }
        if (len > 1024*1024) {
            result.clear();
            sprintf_1024(result, "finder: %s : read error, line too long: %d", ___url, len);
            disconnect();
            return -1;
        }
        if (len > 0 ) {
            if (___fp.readn(result, len) != len) {
                result.clear();
                sprintf_1024(result, "finder: %s : read result error", ___url);
                disconnect();
                return -1;
            }
        }
        mystr.clear();
        ___fp.gets(mystr);
        if (___fp.is_exception()) {
            result.clear();
            sprintf_1024(result, "finder: %s : read error", ___url);
            return -1;
        }

        mystr.clear();
        ___fp.gets(mystr);
        ___TRIM_RN(mystr);
        if (strcmp(mystr.c_str(), "END")) {
            result.clear();
            sprintf_1024(result, "finder: %s : read error, neeed END", ___url);
            return -1;
        }
        return 1;
    }

    return -1;
}

void memcache_finder::disconnect()
{
    ___fp.close();
    if (___fd != -1) {
        close(___fd);
        ___fd = -1;
    }
}

bool memcache_finder::connect(long timeout)
{
    if (___fd != -1) {
        return true;
    }
    if (___fd ==-1) {
        ___fd = zcc::connect(___destination);
    }
    if (___fd < 0) {
        return false;
    }
    ___fp.open(___fd);
    return true;
}


basic_finder *memcache_finder_create()
{
    return new memcache_finder();
}

}
