/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-12-26
 * ================================
 */

#include "zcc.h"

namespace zcc
{

void dict_debug(const std::map<std::string, std::string> &dict)
{
    std_map_walk_begin(dict, k, v) {
        debug_kv_show(k.c_str(), v.c_str());
    } std_map_walk_end;
}

bool dict_find(const std::map<std::string, std::string> &dict, const std::string &key, char **val)
{
    if (val) {
        *val = 0;
    }
    auto it = dict.find(key);
    if (it == dict.end()) {
        return false;
    }
    if (val) {
        *val = (char *)it->second.c_str();
    }
    return true;
}

bool dict_find(const std::map<std::string, std::string> &dict, const char *key, char **val)
{
    if (val) {
        *val = 0;
    }
    auto it = dict.find(key);
    if (it == dict.end()) {
        return false;
    }
    if (val) {
        *val = (char *)it->second.c_str();
    }
    return true;
}

char *dict_get_str(const std::map<std::string, std::string> &dict, const std::string &key, const char *def)
{
    char *v;
    if (dict_find(dict, key, &v)) {
        return v;
    }
    return const_cast<char *>(def);
}

char *dict_get_str(const std::map<std::string, std::string> &dict, const char *key, const char *def)
{
    char *v;
    if (dict_find(dict, key, &v)) {
        return v;
    }
    return const_cast<char *>(def);
}

}
