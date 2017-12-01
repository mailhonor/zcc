/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-11-14
 * ================================
 */

#include "zcc.h"
#include <ctype.h>

namespace zcc
{

void dict::debug_show()
{
    std_map_walk_begin(*this, k, v) {
        debug_kv_show(k.c_str(), v.c_str());
    } std_map_walk_end;
}

bool dict::find(const std::string &key, char **val)
{
    if (val) {
        *val = 0;
    }
    iterator it = find(key);
    if (it == end()) {
        return false;
    }
    *val = (char *)it->second.c_str();
    return true;
}

bool dict::find(const char *key, char **val)
{
    if (val) {
        *val = 0;
    }
    iterator it = find(key);
    if (it == end()) {
        return false;
    }
    *val = (char *)it->second.c_str();
    return true;
}

char *dict::get_str(const std::string &key, const char *def)
{
    char *v;
    if (find(key, &v)) {
        return v;
    }
    return const_cast<char *>(def);
}

char *dict::get_str(const char *key, const char *def)
{
    char *v;
    if (find(key, &v)) {
        return v;
    }
    return const_cast<char *>(def);
}

bool dict::get_bool(const std::string &key, bool def)
{
    return to_bool(get_str(key, ""), def);
}

bool dict::get_bool(const char *key, bool def)
{
    return to_bool(get_str(key, ""), def);
}

int dict::get_int(const std::string &key, int def, int min, int max)
{
    int r = atoi(get_str(key, ""));
    if ((r < min) || (r > max)) {
        return def;
    }
    return r;
}

int dict::get_int(const char *key, int def, int min, int max)
{
    int r = atoi(get_str(key, ""));
    if ((r < min) || (r > max)) {
        return def;
    }
    return r;
}

long dict::get_long(const std::string &key, long def, long min, long max)
{
    long r = atol(get_str(key, ""));
    if ((r < min) || (r > max)) {
        return def;
    }
    return r;
}

long dict::get_long(const char *key, long def, long min, long max)
{
    long r = atol(get_str(key, ""));
    if ((r < min) || (r > max)) {
        return def;
    }
    return r;
}

long dict::get_second(const std::string &key, long def, long min, long max)
{
    int r = to_second(get_str(key, ""), def);
    if ((r < min) || (r > max)) {
        return def;
    }
    return r;
}

long dict::get_second(const char *key, long def, long min, long max)
{
    int r = to_second(get_str(key, ""), def);
    if ((r < min) || (r > max)) {
        return def;
    }
    return r;
}

long dict::get_size(const std::string &key, long def, long min, long max)
{
    int r = to_size(get_str(key, ""), def);
    if ((r < min) || (r > max)) {
        return def;
    }
    return r;
}

long dict::get_size(const char *key, long def, long min, long max)
{
    int r = to_size(get_str(key, ""), def);
    if ((r < min) || (r > max)) {
        return def;
    }
    return r;
}

void dict::parse_url_query(const char *query)
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
        (*this)[name] = value;
        if (*p == '\0') {
            break;
        }
        ps = p + 1;
    }
}

char *dict::build_url_query(std::string &query, bool strict)
{
    bool first = true;
    std_map_walk_begin(*this, key, val) {
        if (first) {
            first = false;
        } else {
            query.push_back('&');
        }
        query.append(key);
        query.push_back('&');
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
                query.push_back(ch>>4);
                query.push_back(ch&0X0F);
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
            query.push_back(ch>>4);
            query.push_back(ch&0X0F);
        }
    } std_map_walk_end;
    return (char *)(void *)query.c_str();
}

}
