/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2016-11-14
 * ================================
 */

#include "zcc.h"

namespace zcc
{

config default_config;

bool config::load_by_filename(const char *filename)
{
    char *key, *val;

    fstream fp;
    if (!fp.open(filename, "r")) {
        return false;
    }
    char *buf = (char *)malloc(10240 + 1);
    int len;
    while((len=fp.gets(buf, 10240)) > 0) {
        buf[len] = 0;
        key = trim_left(buf);
        if ((!*key) || (*key == '#')) {
            continue;
        }
        val = key;
        while(*val) {
            if ((*val == '=') || (*val == '+')) {
                break;
            }
            val++;
        }

        if (*val == 0) {
            key = trim_right(key);
            (*this)[key] = "";
        } else if (*val == '=') {
            *val = 0;
            val ++;
            key = trim_right(key);
            val = trim(val);
            (*this)[key] = val;
        } else if (*val == '+') {
            *val = 0;
            val ++;
            key = trim_right(key);
            val = trim(val);
            config::iterator it = find(key);
            if (it == end()) {
                (*this)[key] = val;
            } else {
                it->second.append(val);
            }
        }
    }
    free(buf);

    return true;
}

void config::debug_show()
{
    std_map_walk_begin(*this, k, v) {
        debug_kv_show(k.c_str(), v.c_str());
    } std_map_walk_end;
}

bool config::find(const std::string &key, char **val)
{
    if (val) {
        *val = 0;
    }
    iterator it = find(key);
    if (it == end()) {
        return false;
    }
    if (val) {
        *val = (char *)it->second.c_str();
    }
    return true;
}

bool config::find(const char *key, char **val)
{
    if (val) {
        *val = 0;
    }
    iterator it = find(key);
    if (it == end()) {
        return false;
    }
    if (val) {
        *val = (char *)it->second.c_str();
    }
    return true;
}

char *config::get_str(const std::string &key, const char *def)
{
    char *v;
    if (find(key, &v)) {
        return v;
    }
    return const_cast<char *>(def);
}

char *config::get_str(const char *key, const char *def)
{
    char *v;
    if (find(key, &v)) {
        return v;
    }
    return const_cast<char *>(def);
}

bool config::get_bool(const std::string &key, bool def)
{
    return to_bool(get_str(key, ""), def);
}

bool config::get_bool(const char *key, bool def)
{
    return to_bool(get_str(key, ""), def);
}

int config::get_int(const std::string &key, int def, int min, int max)
{
    int r = atoi(get_str(key, ""));
    if ((r < min) || (r > max)) {
        return def;
    }
    return r;
}

int config::get_int(const char *key, int def, int min, int max)
{
    int r = atoi(get_str(key, ""));
    if ((r < min) || (r > max)) {
        return def;
    }
    return r;
}

long config::get_long(const std::string &key, long def, long min, long max)
{
    long r = atol(get_str(key, ""));
    if ((r < min) || (r > max)) {
        return def;
    }
    return r;
}

long config::get_long(const char *key, long def, long min, long max)
{
    long r = atol(get_str(key, ""));
    if ((r < min) || (r > max)) {
        return def;
    }
    return r;
}

long config::get_second(const std::string &key, long def, long min, long max)
{
    int r = to_second(get_str(key, ""), def);
    if ((r < min) || (r > max)) {
        return def;
    }
    return r;
}

long config::get_second(const char *key, long def, long min, long max)
{
    int r = to_second(get_str(key, ""), def);
    if ((r < min) || (r > max)) {
        return def;
    }
    return r;
}

long config::get_size(const std::string &key, long def, long min, long max)
{
    int r = to_size(get_str(key, ""), def);
    if ((r < min) || (r > max)) {
        return def;
    }
    return r;
}

long config::get_size(const char *key, long def, long min, long max)
{
    int r = to_size(get_str(key, ""), def);
    if ((r < min) || (r > max)) {
        return def;
    }
    return r;
}

void config::load_another(config &cf)
{
    for (config::iterator it = cf.begin(); it != cf.end(); it++) {
        (*this)[it->first] = it->second;
    }
}

void config::get_str_table(const config_str_table_t * table)
{
    while (table->name) {
        *(table->target) = get_str(table->name, table->defval);
        table++;
    }
}

void config::get_bool_table(const config_bool_table_t * table)
{
    while (table->name) {
        *(table->target) = get_bool(table->name, table->defval);
        table++;
    }
}

#define ___ZCC_CONFIG_GET_TABLE(ttype) \
    void config::get_## ttype ## _table(const config_ ## ttype ## _table_t * table) \
    { \
        while (table && table->name) \
        { \
            *(table->target) = get_ ## ttype(table->name, table->defval, table->min, table->max); \
            table++; \
        } \
    }
___ZCC_CONFIG_GET_TABLE(int);
___ZCC_CONFIG_GET_TABLE(long);
___ZCC_CONFIG_GET_TABLE(size);
___ZCC_CONFIG_GET_TABLE(second);


}
