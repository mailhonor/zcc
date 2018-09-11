/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
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
    bool open(const char *url);
    ssize_t find(const char *query, std::string &result, long timeout);
    void disconnect();
private:
    char *___destination;
    int ___len;
};

static_finder::static_finder()
{
    ___destination = 0;
}

static_finder::~static_finder()
{
    free(___destination);
}

ssize_t static_finder::find(const char *query, std::string &result, long timeout)
{
    result.clear();
    result.append(___destination, ___len);
    return 1;
}

bool static_finder::open(const char *url)
{
    http_url urlobj(url);
    if (urlobj.destination.empty()) {
        return false;
    }

    ___destination = strdup(urlobj.destination.c_str());
    ___len = strlen(___destination);
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
