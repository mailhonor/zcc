/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-08-11
 * ================================
 */

#pragma GCC diagnostic ignored "-Wstrict-aliasing"

#include "zcc.h"

namespace zcc
{

json::json()
{
    ___type = json_type_null;
    ___val.v = 0;
}

json::json(const char *val)
{
    ___type = json_type_string;
    new (___val.string) std::string(val);
}

json::json(const char *val, size_t size)
{
    ___type = json_type_string;
    new (___val.string) std::string(val, size);
}

json::json(const std::string &val)
{
    ___type = json_type_string;
    new (___val.string) std::string(val);
}

json::json(long val)
{
    ___type = json_type_long;
    ___val.number_long = val;
}

json::json(double val)
{
    ___type = json_type_double;
    ___val.number_double = val;
}

json::json(bool val)
{
    ___type = json_type_bool;
    ___val.b = val;
}

json::~json()
{
    clear_value();
}

json *json::used_for_bool()
{
    if (___type != json_type_bool) {
        reset();
        get_bool_value();
    }
    return this;
}

json *json::used_for_long()
{
    if (___type != json_type_long) {
        reset();
        get_long_value();
    }
    return this;
}

json *json::used_for_double()
{
    if (___type != json_type_double) {
        reset();
        get_double_value();
    }
    return this;
}

json *json::used_for_string()
{
    if (___type != json_type_string) {
        reset();
        get_string_value();
    }
    return this;
}

json *json::used_for_array()
{
    if (___type != json_type_array) {
        reset();
        ___type = json_type_array;
        ___val.v = new vector<json *>();
    }
    return this;
}

json *json::used_for_object()
{
    if (___type != json_type_object) {
        reset();
        ___type = json_type_object;
        ___val.m = new map<json *>();
    }
    return this;
}

std::string *json::get_string_value()
{
    if (___type == json_type_null) {
        new (___val.string) std::string();
        ___type = json_type_string;
    }
    if (___type != json_type_string) {
        return 0;
    }
    return (std::string *)(___val.string);
}

long *json::get_long_value()
{
    if (___type == json_type_null) {
        ___type = json_type_long;
    }
    if (___type != json_type_long) {
        return 0;
    }
    return &(___val.number_long);
}

double *json::get_double_value()
{
    if (___type == json_type_null) {
        ___type = json_type_double;
    }
    if (___type != json_type_double) {
        return 0;
    }
    return &(___val.number_double);
}

bool *json::get_bool_value()
{
    if (___type == json_type_null) {
        ___type = json_type_bool;
    }
    if (___type != json_type_bool) {
        return 0;
    }
    return &(___val.b);
}

json * json::get_array_element(size_t idx)
{
    if (___type != json_type_array) {
        return 0;
    }
    if (___val.v->size() <= idx) {
        return 0;
    }
    return (*___val.v)[idx];
}

json * json::get_object_element(const char *key)
{
    if (___type != json_type_object) {
        return 0;
    }
    json *r;
    if (___val.m->find(key, &r)) {
        return r;
    }
    return 0;
}

json_object_walker *json::get_object_first_walker()
{
    if (___type != json_type_object) {
        return 0;
    }
    return ___val.m->first_node();
}

json_object_walker *json::get_object_last_walker()
{
    if (___type != json_type_object) {
        return 0;
    }
    return ___val.m->last_node();
}

size_t json::get_array_size()
{
    if (___type != json_type_array) {
        return 0;
    }
    return ___val.v->size();
}

size_t json::get_object_size()
{
    if (___type != json_type_object) {
        return 0;
    }
    return ___val.m->size();
}

json *json::reset()
{
    clear_value();
    ___type = json_type_null;
    return this;
}

void json::push_back_array(json *j)
{
    if (___type == json_type_null) {
        ___type = json_type_array;
        ___val.v = new vector<json *>();
    }
    if (___type != json_type_array) {
        zcc_fatal("value's type of json is not array");
    }
    ___val.v->push_back(j);
}

void json::set_array_element(size_t idx, json *j, json **old)
{
    if (___type == json_type_null) {
        ___type = json_type_array;
        ___val.v = new vector<json *>();
    }
    if (___type != json_type_array) {
        zcc_fatal("value's type of json is not array");
    }
    if (old) {
        *old = 0;
    }
    if (idx < ___val.v->size()) {
        (*___val.v)[idx] = j;
    } else {
        ___val.v->resize(idx);
        ___val.v->push_back(j);
    }
}

void json::set_object_element(const char *key, json *j, json **old)
{
    if (___type == json_type_null) {
        ___type = json_type_object;
        ___val.m = new map<json *>();
    }
    if (___type != json_type_object) {
        zcc_fatal("value's type of json is not object");
    }
    if (old) {
        *old = 0;
    }
    ___val.m->update(key, j, old);
}

json * json::erase_array_key(size_t idx)
{
    if (___type != json_type_array) {
        return 0;
    }
    json *r = 0;
    if (idx < ___val.v->size()) {
        r = (*___val.v)[idx];
        ___val.v->erase(idx);
    } else {
        r = 0;
    }
    return r;
}

json * json::erase_object_key(const char *key)
{
    if (___type != json_type_object) {
        return 0;
    }
    json *r = 0;
    ___val.m->erase(key, &r);
    return r;
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
                jn = jn->get_array_element(idx);
            }
        } else if (jn->get_type() == json_type_object) {
            jn = jn->get_object_element(key);
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
                jn = jn->get_array_element(idx);
            }
        } else if (jn->get_type() == json_type_object) {
            jn = jn->get_object_element(key);
        } else {
            jn = 0;
        }
    }
    va_end(ap);
    return jn;
}

void json::clear_value()
{
    if (___type == json_type_string) {
        if (___val.string) {
            typedef std::string ___std_string;
            ((___std_string *)(___val.string))->~___std_string();
        }
    } else if (___type == json_type_array) {
        if (___val.v) {
            zcc_vector_walk_begin(*___val.v, jn) {
                delete (jn);
            } zcc_vector_walk_end;
            delete ___val.v;
        }
    } else if (___type == json_type_object) {
        map<json *> *mp = ___val.m;
        if (mp) {
            zcc_map_walk_begin(*mp, key, val) {
                delete val;
            } zcc_map_walk_end;
            delete mp;
        }
    }
    memset(&___val, 0, sizeof(___val));
}

}
