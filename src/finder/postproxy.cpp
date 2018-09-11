/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
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
    bool open(const char *url);
    ssize_t find(const char *query, std::string &result, long timeout);
    void disconnect();
    bool connect(long timeout);
private:
    const char *___url;
    const char *___destination;
    const char *___postfix_dict;
    const char *___prefix;
    const char *___suffix;
    stream ___fp;
    int ___fd;
};

postproxy_finder::postproxy_finder()
{
    ___fd = -1;
    ___url = 0;
}

postproxy_finder::~postproxy_finder()
{
    disconnect();
    free(___url);
}

bool postproxy_finder::open(const char *url)
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
    sdup.push_back(dict_get_str(urlobj.query_variates, "dictname"));

    ___url = sdup.dup();
    std::vector<size_t> &offsets = sdup.offsets();
    ___destination = ___url + offsets[1];
    ___prefix = ___url + offsets[2];
    ___suffix = ___url + offsets[3];
    ___postfix_dict = ___url + offsets[4];
    return true;
}

ssize_t postproxy_finder::find(const char *query, std::string &result, long timeout)
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
    int i, ret, status;
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
        ___fp.set_timeout(timeout_left(dtime));
        ___fp.puts("request");
        ___fp.put('\0');
        ___fp.puts("lookup");
        ___fp.put('\0');

        ___fp.puts("table");
        ___fp.put('\0');
        ___fp.puts(___postfix_dict);
        ___fp.put('\0');

        ___fp.puts("flags");
        ___fp.put('\0');
        ___fp.printf_1024("%d", (1 << 6));
        ___fp.put('\0');

        ___fp.puts("key");
        ___fp.put('\0');
        ___fp.puts(query);
        ___fp.put('\0');

        ___fp.put('\0');

        ___fp.flush();
        if (___fp.is_exception()) {
            result.clear();
            sprintf_1024(result, "finder: %s : write error(%m)", ___url);
            continue;
        }

        mystr.clear();
        ret = ___fp.gets(mystr, '\0');
        if ((ret != 7) || (strcmp(mystr.c_str(), "status"))) {
            result.clear();
            sprintf_1024(result, "finder: %s : read error, need status name", ___url);
            disconnect();
            return -1;
        }

        mystr.clear();
        ret = ___fp.gets(mystr, '\0');
        if ((ret != 2)) {
            sprintf_1024(result, "finder: %s : read error, need status value", ___url);
            disconnect();
            return -1;
        }
        status = atoi(mystr.c_str());

        mystr.clear();
        ret = ___fp.gets(mystr, '\0');
        if ((ret != 6) || (strcmp(mystr.c_str(), "value"))) {
            result.clear();
            sprintf_1024(result, "finder: %s : read error, need value name", ___url);
            disconnect();
            return -1;
        }
        mystr.clear();
        ret = ___fp.gets(result, '\0');
        if ((ret<0)) {
            result.clear();
            sprintf_1024(result, "finder: %s : read error, need status value", ___url);
            disconnect();
            return -1;
        }
        if (___fp.get() < 0) {
            result.clear();
            sprintf_1024(result, "finder: %s : read error, need end", ___url);
            disconnect();
            return -1;
        }
        if (status == 0) {
            return 1;
        }
        if (status == 1) {
            return 0;
        }
        result.clear();
        sprintf_1024(result, "finder: %s : read error, postproxy, return %d", ___url, status);
        disconnect();
        return -1;
    }

    return -1;
}

void postproxy_finder::disconnect()
{
    ___fp.close();
    if (___fd != -1) {
        close(___fd);
        ___fd = -1;
    }
}

bool postproxy_finder::connect(long timeout)
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


basic_finder *postproxy_finder_create()
{
    return new postproxy_finder();
}

}
