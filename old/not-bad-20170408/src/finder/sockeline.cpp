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

class socketline_finder: public basic_finder
{
public:
    socketline_finder();
    ~socketline_finder();
    inline bool pthread_safe() { return false; }
    bool open(const char *_url);
    ssize_t find(const char *query, string &result, long timeout);
    void disconnect();
    bool connect(long timeout);
private:
    const char *___prefix;
    const char *___suffix;
    iostream *___fp;
    int ___fd;
};

socketline_finder::socketline_finder()
{
    ___fp = 0;
    ___fd = -1;
}

socketline_finder::~socketline_finder()
{
    disconnect();
}

bool socketline_finder::open(const char *url)
{
    parse_url(url);
    ___prefix = kv_pairs.find("prefix", "");
    ___suffix = kv_pairs.find("suffix", "");
    if (!___destination) {
        return false;
    }
    return true;
}

ssize_t socketline_finder::find(const char *query, string &result, long timeout)
{
#define ___TRIM_RN(mystr) { \
    char *p = (mystr).c_str(); int len = (mystr).get_length(); \
    if ((len>0)&&(p[len-1] ='\n')) len--; \
    if ((len>0)&&(p[len-1] ='\r')) len--; \
    (mystr).truncate(len); (mystr).terminate(); \
}
    int i;
    long dtime = timeout_set(timeout);
    char error_buf[1024 + 1];

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
        ___fp->printf_1024("%s%s%s\r\n", ___prefix, query, ___suffix);
        ___fp->flush();
        if (___fp->is_error()) {
            snprintf(error_buf, 1024, "finder: %s : write error((%m)", ___url);
            result = error_buf;
            continue;
        }

        ___fp->gets(result);
        if (___fp->is_error()) {
            snprintf(error_buf, 1024, "finder: %s : read error", ___url);
            result = error_buf;
            disconnect();
            return -1;
        }
        return 1;
    }

    return -1;
}

void socketline_finder::disconnect()
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

bool socketline_finder::connect(long timeout)
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


basic_finder *socketline_finder_create()
{
    return new socketline_finder();
}

}
