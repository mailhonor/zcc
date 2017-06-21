/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-12-09
 * ================================
 */

#include "zcc.h"

namespace zcc
{

class static_finder: public basic_finder
{
public:
    static_finder();
    ~static_finder();
    inline bool pthread_safe() { return true; }
    bool open(const char *url);
    ssize_t find(const char *query, string &result, long timeout);
    void disconnect();
    int len;
};

static_finder::static_finder()
{
}

static_finder::~static_finder()
{
}

ssize_t static_finder::find(const char *query, string &result, long timeout)
{
    result.clear();
    result.append(___destination, len);
    return 1;
}

bool static_finder::open(const char *url)
{
    parse_url(url);
    if (!___destination) {
        return false;
    }
    len = strlen(___destination);
    return true;
}

void static_finder::disconnect()
{
}

static_finder *static_finder_create()
{
    return new static_finder();
}

}
