/*
 * ================================
 * eli960@163.com
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
    dict ___dict;
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
    if (zcc::empty(urlobj.get_destination())) {
        return false;
    }
    if (!load_dict(urlobj.get_destination())) {
        return false;
    }
    ___url = strdup(urlobj.get_destination());
    return true;
}
ssize_t flat_finder::find(const char *query, std::string &result, long timeout)
{
    char *v;
    
    v = ___dict.get_str(query, 0);
    if (!v) {
        return 0;
    }
    result = v;
    return 1;
}

bool flat_finder::load_dict(const char *fn)
{
    FILE *fp;
    char buf_raw[10240 + 10], *buf, *p;
    int len;

    fp = fopen(fn, "r");
    if (!fp) {
        zcc_info("finder: create %s, can not open(%m)", ___url);
        return false;
    }
    while ((!feof(fp)) && (!ferror(fp))) {
        if (!fgets(buf_raw, 10240, fp)) {
            break;
        }
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
    if (ferror(fp)) {
        zcc_info("finder: create %s, read error(%m)", ___url);
        return false;
    }

    fclose(fp);
    return true;
}

basic_finder *flat_finder_create()
{
    return new flat_finder();
}

}
