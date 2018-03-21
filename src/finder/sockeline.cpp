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
    bool open(const char *_url);
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

socketline_finder::socketline_finder()
{
    ___fd = -1;
    ___url = 0;
}

socketline_finder::~socketline_finder()
{
    disconnect();
    free(___url);
}

bool socketline_finder::open(const char *url)
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

ssize_t socketline_finder::find(const char *query, std::string &result, long timeout)
{
    if (timeout < 1) {
        timeout = var_max_timeout;
    }
    int i;
    long dtime = timeout_set(timeout);

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
        ___fp.printf_1024("%s%s%s\r\n", ___prefix, query, ___suffix);
        ___fp.flush();
        if (___fp.is_exception()) {
            result.clear();
            sprintf_1024(result, "finder: %s : write error((%m)", ___url);
            continue;
        }

        ___fp.gets(result);
        if (___fp.is_exception()) {
            result.clear();
            sprintf_1024(result, "finder: %s : read error", ___url);
            disconnect();
            return -1;
        }
        return 1;
    }

    return -1;
}

void socketline_finder::disconnect()
{
    ___fp.close();
    if (___fd != -1) {
        close(___fd);
        ___fd = -1;
    }
}

bool socketline_finder::connect(long timeout)
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


basic_finder *socketline_finder_create()
{
    return new socketline_finder();
}

}
