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
    bool open(const char *url);
    ssize_t find(const char *query, std::string &result, long timeout);
    void disconnect();
    bool connect(long timeout);
private:
    const char *___url;;
    const char *___destination;;
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
    ___url = 0;
}

redis_finder::~redis_finder()
{
    disconnect();
    free(___url);
}

bool redis_finder::open(const char *url)
{
    std::string dest;
    dict dt;
    if (!parse_url(url, dest, dt)) {
        return false;
    }
    if (dest.empty()) {
        return false;
    }
    stringsdup sdup;
    sdup.push_back(url);
    sdup.push_back(dest.c_str());
    sdup.push_back(dt.get_str("prefix", ""));
    sdup.push_back(dt.get_str("suffix", ""));
    sdup.push_back(dt.get_str("key", ""));

    ___url = sdup.dup();
    vector<size_t> &offsets = sdup.offsets();
    ___destination = ___url + offsets[1];
    ___prefix = ___url + offsets[2];
    ___suffix = ___url + offsets[3];
    ___key = ___url + offsets[4];
    return true;
}

ssize_t redis_finder::find(const char *query, std::string &result, long timeout)
{
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
            sprintf_1024(result, "finder: %s : connection error((%m)", ___url);
            continue;
        }
        ___fp->set_timeout(timeout_left(dtime));
        ___fp->printf_1024("hget %s %s%s%s\r\n", ___key, ___prefix, query, ___suffix);
        ___fp->flush();
        if (___fp->is_exception()) {
            sprintf_1024(result, "finder: %s : write error((%m)", ___url);
            continue;
        }

        mystr.clear();
        ___fp->gets(mystr);
        if (___fp->is_exception()) {
            sprintf_1024(result, "finder: %s : read error", ___url);
            disconnect();
            return -1;
        }
        ___TRIM_RN(mystr);
        if (!strcmp(mystr.c_str(), "$-1")) {
            return 0;
        }
        if (*(mystr.c_str()) != '$') {
            sprintf_1024(result, "finder: %s : read error, NEED $", ___url);
            disconnect();
            return -1;
        }

        len = atoi(mystr.c_str() + 1);
        if (len > 1024*1024) {
            sprintf_1024(result, "finder: %s : read error, line too long: %d", ___url, len);
            disconnect();
            return -1;
        }
        if (len > 0) {
            if (___fp->readn(result, len) != len) {
                result.clear();
                sprintf_1024(result, "finder: %s : read result error", ___url);
                return -1;
            }
        }
        mystr.clear();
        ___fp->gets(mystr);
        if (___fp->is_exception()) {
            result.clear();
            sprintf_1024(result, "finder: %s : read error", ___url);
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
        ___fd = zcc::connect(___destination);
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
