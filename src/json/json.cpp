/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2017-08-11
 * ================================
 */

#pragma GCC diagnostic ignored "-Wstrict-aliasing"

#include "zcc.h"
#include <ctype.h>

namespace zcc
{

json::json()
{
    ___parent = 0;
    ___type = json_type_null;
    ___val.v = 0;
}

json::json(const char *val)
{
    ___parent = 0;
    ___type = json_type_string;
    new (___val.str) std::string(val?val:"");
}

json::json(const char *val, size_t size)
{
    ___parent = 0;
    ___type = json_type_string;
    if (!val || !size) {
        new (___val.str) std::string("");
    } else {
        new (___val.str) std::string(val, size);
    }
}

json::json(const std::string &val)
{
    ___parent = 0;
    ___type = json_type_string;
    if (val.empty()) {
        new (___val.str) std::string("");
    } else {
        new (___val.str) std::string(val);
    }
}

json::json(long val)
{
    ___parent = 0;
    ___type = json_type_long;
    ___val.number_long = val;
}

json::json(double val)
{
    ___parent = 0;
    ___type = json_type_double;
    ___val.number_double = val;
}

json::json(bool val)
{
    ___parent = 0;
    ___type = json_type_bool;
    ___val.b = val;
}

json::~json()
{
    if ((___type == json_type_array) || (___type == json_type_object)) {
        reset();
    } else if (___type == json_type_string) {
        if (___val.str) {
            typedef std::string ___std_string;
            ((___std_string *)(___val.str))->~___std_string();
        }
    }
}

json *json::used_for_bool()
{
    if (___type != json_type_bool) {
        get_bool_value();
    }
    return this;
}

json *json::used_for_long()
{
    if (___type != json_type_long) {
        get_long_value();
    }
    return this;
}

json *json::used_for_double()
{
    if (___type != json_type_double) {
        get_double_value();
    }
    return this;
}

json *json::used_for_string()
{
    if (___type != json_type_string) {
        get_string_value();
    }
    return this;
}

json *json::used_for_array()
{
    if (___type != json_type_array) {
        get_array_value();
    }
    return this;
}

json *json::used_for_object()
{
    if (___type != json_type_object) {
        get_object_value();
    }
    return this;
}

std::string &json::get_string_value()
{
    if (___type != json_type_string) {
        reset();
    }
    if (___type == json_type_null) {
        new (___val.str) std::string();
        ___type = json_type_string;
    }
    return *((std::string *)(___val.str));
}

long &json::get_long_value()
{
    if (___type != json_type_long) {
        reset();
        ___type = json_type_long;
    }
    return ___val.number_long;
}

double &json::get_double_value()
{
    if (___type != json_type_double) {
        reset();
        ___type = json_type_double;
    }
    return ___val.number_double;
}

bool &json::get_bool_value()
{
    if (___type != json_type_bool) {
        reset();
        ___type = json_type_bool;
    }
    return ___val.b;
}

const std::vector<json *> &json::get_array_value()
{
    if (___type != json_type_array) {
        reset();
        ___type = json_type_array;
        ___val.v = new std::vector<json *>();
    }
    return *(___val.v);
}

const std::map<std::string, json *> &json::get_object_value()
{
    if (___type != json_type_object) {
        reset();
        ___type = json_type_object;
        ___val.m = new std::map<std::string, json *>();
    }
    return *(___val.m);
}

json *json::set_string_value(const std::string &val)
{
    get_string_value() = val;
    return this;
}

json *json::set_string_value(const char *val)
{
    get_string_value() = val;
    return this;
}

json *json::set_string_value(const char *val, size_t size)
{
    std::string &s = get_string_value();
    s.clear();
    s.append(val, size);
    return this;
}

json *json::set_long_value(long val)
{
    get_long_value() = val;
    return this;
}

json *json::set_double_value(double val)
{
    get_double_value() = val;
    return this;
}

json *json::set_bool_value(bool val)
{
    get_bool_value() = val;
    return this;
}

json * json::array_get_element(size_t idx)
{
    if (___type != json_type_array) {
        return 0;
    }
    if (___val.v->size() <= idx) {
        return 0;
    }
    return (*___val.v)[idx];
}

json * json::object_get_element(const char *key)
{
    if (___type != json_type_object) {
        return 0;
    }
    std::map<std::string, json *>::iterator it;
    it = ___val.m->find(key);
    if (it == ___val.m->end()) {
        return 0;
    }
    return it->second;
}

size_t json::array_get_size()
{
    if (___type != json_type_array) {
        return 0;
    }
    return ___val.v->size();
}

size_t json::object_get_size()
{
    if (___type != json_type_object) {
        return 0;
    }
    return ___val.m->size();
}

json * json::array_add_element(json *j, bool return_child)
{
    j->___parent = this;
    if (___type == json_type_null) {
        ___type = json_type_array;
        ___val.v = new std::vector<json *>();
    }
    if (___type != json_type_array) {
        zcc_fatal("value's type of json is not array");
    }
    ___val.v->push_back(j);
    return (return_child?j:this);
}

json * json::array_add_element(size_t idx, json *j, bool return_child)
{
    json *d;
    j->___parent = this;
    if (___type == json_type_null) {
        ___type = json_type_array;
        ___val.v = new std::vector<json *>();
    }
    if (___type != json_type_array) {
        zcc_fatal("value's type of json is not array");
    }
    if (idx < ___val.v->size()) {
        d = (*___val.v)[idx];
        (*___val.v)[idx] = j;
        if (d) {
            delete d;
        }
    } else {
        ___val.v->resize(idx);
        ___val.v->push_back(j);
    }

    return (return_child?j:this);
}

json * json::object_add_element(const char *key, json *j, bool return_child)
{
    json *d;
    j->___parent = this;
    if (___type == json_type_null) {
        ___type = json_type_object;
        ___val.m = new std::map<std::string, json *>();
    }
    if (___type != json_type_object) {
        zcc_fatal("value's type of json is not object");
    }
    std_map_update(*___val.m, key, j, d);
    if (d) {
        delete d;
    }

    return (return_child?j:this);
}

json * json::array_add_element(size_t idx, json *j, json **old, bool return_child)
{
    json *d = 0;
    j->___parent = this;
    if (___type == json_type_null) {
        ___type = json_type_array;
        ___val.v = new std::vector<json *>();
    }
    if (___type != json_type_array) {
        zcc_fatal("value's type of json is not array");
    }
    if (old) {
        *old = 0;
    }
    if (idx < ___val.v->size()) {
        d = (*___val.v)[idx];
        (*___val.v)[idx] = j;
        if (old) {
            *old = d;
            d->___parent = 0;
        } else if (d) {
            delete d;
        }
    } else {
        ___val.v->resize(idx);
        ___val.v->push_back(j);
    }

    return (return_child?j:this);
}

json * json::object_add_element(const char *key, json *j, json **old, bool return_child)
{
    json *d = 0;
    j->___parent = this;
    if (___type == json_type_null) {
        ___type = json_type_object;
        ___val.m = new std::map<std::string, json *>();
    }
    if (___type != json_type_object) {
        zcc_fatal("value's type of json is not object");
    }
    std_map_update(*___val.m, key, j, d);
    if (old) {
        *old = d;
        if (d) {
            d->___parent = 0;
        }
    } else if (d) {
        delete d;
    }

    return (return_child?j:this);
}

json * json::array_detach_element(size_t idx)
{
    if (___type != json_type_array) {
        return 0;
    }

    if (idx >=  ___val.v->size()) {
        return 0;
    }

    std::vector<json *>::iterator it = ___val.v->begin() + idx;
    json *r = *it;
    ___val.v->erase(it);
    if (r) {
        r->___parent = 0;
    }

    return r;
}

json * json::object_detach_element(const char *key)
{
    if (___type != json_type_object) {
        return 0;
    }

    std::map<std::string, json *>::iterator it = ___val.m->find(key);
    if (it == ___val.m->end()) {
        return 0;
    }
    json *r = it->second;
    ___val.m->erase(it);
    if (r) {
        r->___parent = 0;
    }

    return r;
}

json * json::array_erase_element(size_t idx)
{
    if (___type != json_type_array) {
        return this;
    }

    if (idx >=  ___val.v->size()) {
        return this;
    }

    std::vector<json *>::iterator it = ___val.v->begin() + idx;
    json *r = *it;
    ___val.v->erase(it);
    if (r) {
        delete r;
    }

    return this;
}

json * json::object_erase_element(const char *key)
{
    if (___type != json_type_object) {
        return this;
    }

    std::map<std::string, json *>::iterator it = ___val.m->find(key);
    if (it == ___val.m->end()) {
        return this;
    }
    json *r = it->second;
    ___val.m->erase(it);
    if (r) {
        delete r;
    }

    return this;
}

json * json::get_element_by_path(const char *path)
{
    if (empty(path)) {
        return this;
    }
    std::string tmp(path);
    char *p, *key = (char *)(void *)tmp.c_str();
    json *jn = this;
    while (jn) {
        p = strchr(key, '/');
        if (p) {
            *p++ = 0;
        }
        if (jn->get_type() == json_type_array) {
            p = key;
            size_t idx = 0;
            while(*p) {
                if (!isdigit(*p)) {
                    idx = 1024 * 1024 + 10;
                    break;
                }
                idx = idx * 10 + (*p - '0');
                p++;
            }
            if (idx > 1024 * 1024) {
                jn = 0;
            } else {
                jn = jn->array_get_element(idx);
            }
        } else if (jn->get_type() == json_type_object) {
            jn = jn->object_get_element(key);
        } else {
            jn = 0;
        }
        if (!p) {
            break;
        }
        key = p;
    }

    return jn;
}

json * json::get_element_by_path_vec(const char *path0, ...)
{
    if (empty(path0)) {
        return 0;
    }
    json *jn = this;
    bool first = true;
    char *key, *p;
    va_list ap;
    va_start(ap, path0);
    while(jn) {
        if (first) {
            first = false;
            key = (char *)path0;
        } else {
            key = va_arg(ap, char *);
        }
        if (key == 0) {
            break;
        }
        if (jn->get_type() == json_type_array) {
            p = key;
            size_t idx = 0;
            while(*p) {
                if (!isdigit(*p)) {
                    idx = 1024 * 1024 + 10;
                    break;
                }
                idx = idx * 10 + (*p - '0');
                p++;
            }
            if (idx > 1024 * 1024) {
                jn = 0;
            } else {
                jn = jn->array_get_element(idx);
            }
        } else if (jn->get_type() == json_type_object) {
            jn = jn->object_get_element(key);
        } else {
            jn = 0;
        }
    }
    va_end(ap);
    return jn;
}

json * json::get_top()
{
    json *r = this;
    while (r->___parent) {
        r = r->___parent;
    }
    return r;
}

json *json::reset()
{
#if 0
    if (___type == json_type_string) {
        if (___val.str) {
            typedef std::string ___std_string;
            ((___std_string *)(___val.str))->~___std_string();
        }
    } else if (___type == json_type_array) {
        if (___val.v) {
            std_list_walk_begin(*___val.v, jn) {
                delete (jn);
            } std_list_walk_end;
            delete ___val.v;
        }
    } else if (___type == json_type_object) {
        std::map<std::string, json *> *mp = ___val.m;
        if (mp) {
            std_map_walk_begin(*mp, key, val) {
                delete val;
            } std_map_walk_end;
            delete mp;
        }
    }
    memset(&___val, 0, sizeof(___val));
    ___type = json_type_null;
    return this;
#else
    std::list<json *> for_deteled;
    for_deteled.push_back(this);
    while(!for_deteled.empty()) {
        json *js = for_deteled.front();
        for_deteled.pop_front();
        if (js->___type == json_type_string) {
            if (js->___val.str) {
                typedef std::string ___std_string;
                ((___std_string *)(js->___val.str))->~___std_string();
            }
        } else if (js->___type == json_type_array) {
            if (js->___val.v) {
                std_vector_walk_begin(*(js->___val.v), jn) {
                    if ((jn->___type == json_type_array) || (jn->___type == json_type_object)) {
                        for_deteled.push_back(jn);
                    } else {
                        delete jn;
                    }
                } std_vector_walk_end;
                delete js->___val.v;
            }
        } else if (js->___type == json_type_object) {
            if (js->___val.m) {
                std_map_walk_begin(*(js->___val.m), key, jn) {
                    if ((jn->___type == json_type_array) || (jn->___type == json_type_object)) {
                        for_deteled.push_back(jn);
                    } else {
                        delete jn;
                    }
                } std_map_walk_end;
                delete js->___val.m;
            }
        }
        js->___type = json_type_null;
        if (this != js) {
            delete js;
        }
    }
    memset(&___val, 0, sizeof(___val));
    ___type = json_type_null;
    return this;
#endif
}

}
