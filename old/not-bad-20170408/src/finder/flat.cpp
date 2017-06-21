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
    inline bool pthread_safe() { return true; }
    bool open(const char *url);
    ssize_t find(const char *query, string &result, long timeout);
private:
    bool load_dict();
    str_dict ___dict;
};

flat_finder::flat_finder()
{
}

flat_finder::~flat_finder()
{
}

bool flat_finder::open(const char *url)
{
    if (!load_dict()) {
        return false;
    }
    if (!___destination) {
        return false;
    }
    return true;
}
ssize_t flat_finder::find(const char *query, string &result, long timeout)
{
    char *v;
    if (___dict.find(query, &v)) {
        result = v;
        return 1;
    } else {
        return 0;
    }
    return -1;
}

bool flat_finder::load_dict()
{
    const char *fn = ___destination;
    FILE *fp;
    char buf_raw[10240 + 10], *buf, *p;
    int len;

    fp = fopen(fn, "r");
    if (!fp) {
        log_info("finder: create %s, can not open(%m)", ___url);
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
        ___dict.update(buf, p);
    }
    if (ferror(fp)) {
        log_info("finder: create %s, read error(%m)", ___url);
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
