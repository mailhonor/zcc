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

http_url::http_url()
{
    port = 0;
}

http_url::http_url(const char *url)
{
    port = 0;
    parse(url);
}

http_url::~http_url()
{
}

void http_url::clear()
{
    scheme.clear();
    destination.clear();
    host.clear();
    path.clear();
    query.clear();
    port = 0;
    query_variates.clear();
}

void http_url::parse(const char *url)
{
    char *ps = const_cast<char *>(url);
    char *p;

    clear();

    p = strstr(ps, "://");
    if (!p) {
        if ((ps[0] == '/') || (ps[1] == '/')) {
            scheme = "http";
            ps = p + 2;
        } else {
            return;
        }
    } else {
        scheme.append(ps, p - ps);
        tolower(scheme);
        ps = p + 3;
    }
    char *tmp_destination = ps;

    if ((!strncmp(ps, "local:", 6))) {
        host = "local";
        p = ps + 6;
        while(1) {
            if ((*p == '?')||(*p == '#') || (*p == '\0')) {
                path.append(ps + 6, p - (ps + 6));
                destination.append(ps, p - ps);
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
    } 

    p = ps;
    while(1) {
        if ((*p == '?')||(*p == '#') || (*p == '/') || (*p == ':') || (*p == '\0')) {
            if (*p != ':') {
                destination.append(ps, p - ps);
            }
            host.append(ps, p - ps);
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
            destination.append(tmp_destination, p - tmp_destination);
            port = atoi(ps);
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
            path.append(ps, p - ps);
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
            query.append(ps, p - ps);
            http_url_parse_query(query_variates, query.c_str());
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
    fragment = ps;
}

void http_url::debug_show()
{
    debug_kv_show("scheme", scheme.c_str());
    debug_kv_show("host", host.c_str());
    debug_kv_show("port", port);
    debug_kv_show("destination", destination.c_str());
    debug_kv_show("path", path.c_str());
    debug_kv_show("query", query.c_str());
    debug_kv_show("fragment", fragment.c_str());
    printf("query variates\n");
    std_map_walk_begin(query_variates, k, v) {
        debug_kv_show(k.c_str(), v.c_str());
    } std_map_walk_end;
}

void http_url_parse_query(std::map<std::string, std::string> &result, const char *query)
{
    char *q, *p, *ps = const_cast<char *>(query);
    std::string name(32, 0), value(128, 0);
    while(1) {
        p = ps;
        while((*p != '\0') && (*p != '&')) {
            p++;
        }
        do {
            q = ps;
            while(q < p) {
                if (*q  == '=') {
                    break;
                }
                q ++ ;
            }
            if (q == p) {
                break;
            }
            name.clear();
            name.append(ps, q - ps);
            tolower(name.c_str());
            value.clear();
            q ++;
            url_hex_decode(q, p - q, value);
        } while(0);
        result[name] = value;
        if (*p == '\0') {
            break;
        }
        ps = p + 1;
    }
}


char *http_url_build_query(std::string &query, std::map<std::string, std::string> &dict, bool strict)
{
    unsigned char dec2hex[18] = "0123456789ABCDEF";
    bool first = true;
    std_map_walk_begin(dict, key, val) {
        if (first) {
            first = false;
        } else {
            query.push_back('&');
        }
        query.append(key);
        query.push_back('=');
        size_t i, len = val.size();
        char *v = (char *)val.c_str();
        for (i = 0; i < len; i++) {
            unsigned char ch = v[i];
            if (ch == ' ') {
                query.push_back('+');
                continue;
            }
            if (isalnum(ch)) {
                query.push_back(ch);
                continue;
            }
            if (strict) {
                query.push_back('%');
                query.push_back(dec2hex[ch>>4]);
                query.push_back(dec2hex[ch&0X0F]);
                continue;
            } 
            if (ch > 127) {
                query.push_back(ch);
                continue;
            }
            if (strchr("._-", ch)) {
                query.push_back(ch);
                continue;
            }
            query.push_back('%');
            query.push_back(dec2hex[ch>>4]);
            query.push_back(dec2hex[ch&0X0F]);
        }
    } std_map_walk_end;
    return (char *)(void *)query.c_str();
}

}
