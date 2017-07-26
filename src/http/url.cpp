/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-07-22
 * ================================
 */

#include "zcc.h"

namespace zcc
{

class http_url_inner
{
public:
     http_url_inner();
    ~http_url_inner();
    char *scheme;
    char *destination;
    char *host;
    int port;
    char *path;
    char *query;
    dict query_variates;
    char *fragment;
};

http_url_inner::http_url_inner()
{
    scheme = blank_buffer;
    destination = blank_buffer;
    host = blank_buffer;
    path = blank_buffer;
    query = blank_buffer;
    fragment = blank_buffer;
    port = 0;
}

http_url_inner::~http_url_inner()
{
    free(scheme);
    free(destination);
    free(host);
    free(path);
    free(query);
    free(fragment);
}

http_url::http_url()
{
    memset(___data, 0, sizeof(___data));
    new(___data) http_url_inner();
}

http_url::http_url(const char *url)
{
    memset(___data, 0, sizeof(___data));
    new(___data) http_url_inner();
    parse(url);
}

http_url::~http_url()
{
    http_url_inner *urldata = (http_url_inner*)___data;
    urldata->~http_url_inner();
}

void http_url::clear()
{
    http_url_inner *urldata = (http_url_inner*)___data;
#define ___rfree(s) { free(urldata->s); urldata->s = blank_buffer; }
    ___rfree(scheme);
    ___rfree(destination);
    ___rfree(host);
    ___rfree(path);
    ___rfree(query);
#undef ___rfree
}

void http_url::parse(const char *url)
{
    http_url_inner *urldata = (http_url_inner*)___data;
    char *ps = const_cast<char *>(url);
    char *p;

    clear();
    urldata->query_variates.clear();
    urldata->port = 0;

    p = strstr(ps, "://");
    if (!p) {
        if ((ps[0] == '/') || (ps[1] == '/')) {
            urldata->scheme = strdup("http");
            ps = p + 2;
        } else {
            return;
        }
    } else {
        urldata->scheme = memdupnull(ps, p - ps);
        tolower(urldata->scheme);
        ps = p + 3;
    }
    if ((!strncmp(ps, "local:", 6))) {
        urldata->host = strdup("local");
        p = ps + 6;
        while(1) {
            if ((*p == '?')||(*p == '#') || (*p == '\0')) {
                urldata->path = memdupnull(ps + 6, p - (ps + 6));
                urldata->destination = memdupnull(ps, p - ps);
                break;
            }
            p ++;
        }
        ps = p ++;
        if (*p == '?') {
            goto query;
        }
        if (*p == '#') {
            goto fragment;
        }
        return;
    } 

    p = ps;
    urldata->destination = ps;
    while(1) {
        if ((*p == '?')||(*p == '#') || (*p == '/') || (*p == ':') || (*p == '\0')) {
            if (*p != ':') {
                urldata->destination = memdupnull(ps, p - ps);
            }
            urldata->host = memdupnull(ps, p - ps);
            break;
        }
        p ++;
    }
    ps = p + 1;
    if (*p == '?') {
        goto query;
    }
    if (*p == '#') {
        goto fragment;
    }
    if (*p == '/') {
        goto path;
    }
    if (*p == ':') {
        goto port;
    }
    return;

port:
    p = ps;
    while(1) {
        if ((*p == '?')||(*p == '#') || (*p == '/') || (*p == '\0')) {
            urldata->destination = memdupnull(urldata->destination, p - urldata->destination);
            char *tmp = memdupnull(ps, p - ps);
            urldata->port = atoi(tmp);
            free(tmp);
            break;
        }
        p ++;
    }
    ps = p + 1;
    if (*p == '?') {
        goto query;
    }
    if (*p == '#') {
        goto fragment;
    }
    if (*p == '/') {
        goto path;
    }
    return;

path:
    p = ps;
    while(1) {
        if ((*p == '?')||(*p == '#')|| (*p == '\0')) {
            urldata->path = memdupnull(ps, p - ps);
            break;
        }
        p ++;
    }
    ps = p + 1;
    if (*p == '?') {
        goto query;
    }
    if (*p == '#') {
        goto fragment;
    }
    return;

query:
    p = ps;
    while(1) {
        if ((*p == '#')|| (*p == '\0')) {
            urldata->query = memdupnull(ps, p - ps);
            urldata->query_variates.parse_url_query(urldata->query);
            break;
        }
        p ++;
    }
    ps = p + 1;
    if (*p == '#') {
        goto fragment;
    }
    return;

fragment:
    urldata->fragment = strdup(ps);
}

char *http_url::get_scheme(const char *def_val)
{
    http_url_inner *urldata = (http_url_inner*)___data;
    return urldata->scheme;
}

char * http_url::get_destination()
{
    http_url_inner *urldata = (http_url_inner*)___data;
    return urldata->destination;
}

char * http_url::get_host()
{
    http_url_inner *urldata = (http_url_inner*)___data;
    return urldata->host;
}

int http_url::get_port(int def_val)
{
    http_url_inner *urldata = (http_url_inner*)___data;
    return urldata->port;
}

char * http_url::get_path()
{
    http_url_inner *urldata = (http_url_inner*)___data;
    return urldata->path;
}

char * http_url::get_query()
{
    http_url_inner *urldata = (http_url_inner*)___data;
    return urldata->query;
}

char * http_url::get_query_variate(const char *name, const char *def_val)
{
    http_url_inner *urldata = (http_url_inner*)___data;
    return urldata->query_variates.get_str(name, def_val);
}

dict &http_url::get_query_variate()
{
    http_url_inner *urldata = (http_url_inner*)___data;
    return urldata->query_variates;
}

char * http_url::get_fragment()
{
    http_url_inner *urldata = (http_url_inner*)___data;
    return urldata->fragment;
}

void http_url::debug_show()
{
    debug_kv_show("scheme", get_scheme());
    debug_kv_show("host", get_host());
    debug_kv_show("port", get_port());
    debug_kv_show("destination", get_destination());
    debug_kv_show("path", get_path());
    debug_kv_show("query", get_query());
    debug_kv_show("fragment", get_fragment());
    printf("query variates\n");
    get_query_variate().debug_show();
}

}

#ifdef __ZCC_SIZEOF_PROBE__
int main()
{ 
    _ZCC_SIZEOF_DEBUG(zcc::http_url, zcc::http_url_inner);
    return 0;
}
#endif

