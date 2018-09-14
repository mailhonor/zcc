/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2016-09-07
 * ================================
 */

#include "zcc.h"

namespace zcc
{

class flat_finder: public basic_finder
{
public:
    flat_finder();
    ~flat_finder();
    bool open(const char *url);
    ssize_t find(const char *query, std::string &result, long timeout);
private:
    bool load_dict(const char *fn);
    std::map<std::string, std::string> ___dict;
    const char *___url;
};

flat_finder::flat_finder()
{
    ___url = 0;
}

flat_finder::~flat_finder()
{
    free(___url);
}

bool flat_finder::open(const char *url)
{
    http_url urlobj(url);
    if (urlobj.destination.empty()) {
        return false;
    }
    if (!load_dict(urlobj.destination.c_str())) {
        return false;
    }
    ___url = strdup(urlobj.destination.c_str());
    return true;
}
ssize_t flat_finder::find(const char *query, std::string &result, long timeout)
{
    char *v;
    
    v = dict_get_str(___dict, query, 0);
    if (!v) {
        return 0;
    }
    result = v;
    return 1;
}

bool flat_finder::load_dict(const char *fn)
{
    char buf_raw[10240 + 10], *buf, *p;
    int len;

    fstream fp;
    if (!fp.open(fn, "r")) {
        zcc_info("finder: create %s, can not open(%m)", ___url);
        return false;
    }
    while ((len = fp.gets(buf_raw, 10240)) > 0) {
        buf_raw[len] = 0;
        buf = buf_raw;
        while (*buf) {
            if (*buf == ' ' || *buf == '\t') {
                p++;
                continue;
            }
            break;
        }
        if (buf[0] == '#') {
            continue;
        }
        if (buf[0] == 0) {
            continue;
        }
        p = strchr(buf, ' ');
        if (!p) {
            continue;
        }
        *p++ = 0;
        while (*p) {
            if (*p == ' ' || *p == '\t') {
                p++;
                continue;
            }
            break;
        }
        len = strlen(p);
        if ((len > 0) && (p[len - 1] == '\n')) {
            len--;
        }
        if ((len > 0) && (p[len - 1] == '\r')) {
            len--;
        }
        p[len] = 0;
        ___dict[buf] = p;
    }
    if (fp.is_error()) {
        zcc_info("finder: create %s, read error(%m)", ___url);
        return false;
    }

    return true;
}

basic_finder *flat_finder_create()
{
    return new flat_finder();
}

}
