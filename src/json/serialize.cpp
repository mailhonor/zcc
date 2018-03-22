/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-08-11
 * ================================
 */

#pragma GCC diagnostic ignored "-Wstrict-aliasing"

#include "zcc.h"
#include <ctype.h>

namespace zcc
{

bool json::load_by_filename(const char *filename)
{
    std::string content;
    if (file_get_contents(filename, content) < 0) {
        return false;
    }
    return unserialize(content.c_str(), content.size());
}

static inline void ___ignore_blank(char * &ps, char *str_end)
{
    while(ps < str_end){
        if ((*ps != ' ') && (*ps != '\t') && (*ps != '\r') && (*ps != '\n')) {
            break;
        }
        ps++;
    }
}

static inline size_t ___ncr_decode(size_t ins, char *wchar)
{
    if (ins < 128) {
        *wchar = ins;
        return 1;
    }
    if (ins < 2048) {
        *wchar++ = (ins >> 6) + 192;
        *wchar++ = (ins & 63) + 128;
        return 2;
    }
    if (ins < 65536) {
        *wchar++ = (ins >> 12) + 224;
        *wchar++ = ((ins >> 6) & 63) + 128;
        *wchar++ = (ins & 63) + 128;
        return 3;
    }
    if (ins < 2097152) {
        *wchar++ = (ins >> 18) + 240;
        *wchar++ = ((ins >> 12) & 63) + 128;
        *wchar++ = ((ins >> 6) & 63) + 128;
        *wchar++ = (ins & 63) + 128;
        return 4;
    }

    return 0;
}

static inline bool ___fetch_string(char *&ps, char *str_end, std::string &str)
{
    char begin = *ps ++, ch, ch2, ch3;
    while (ps < str_end) {
        ch = *ps ++;
        if (ch =='"') {
            if (begin == '"') {
                return true;
            }
            str.push_back(ch);
            continue;
        }
        if (ch =='\'') {
            if (begin == '\'') {
                return true;
            }
            str.push_back(ch);
            continue;
        }
        if (ps == str_end) {
            return false;
        }
        if (ch == '\\') {
            ch2 = *ps ++;
            ch3 = 0;
            if (ch2 == '\\') {
                ch3 = '\\';
            } else if (ch2 == '/') {
                ch3 = '/';
            } else if (ch2 == '\"') {
                ch3 = '\"';
            } else if (ch2 == '\'') {
                ch3 = '\'';
            } else if (ch2 == '\b') {
                ch3 = '\b';
            } else if (ch2 == '\f') {
                ch3 = '\f';
            } else if (ch2 == '\r') {
                ch2 = '\r';
            } else if (ch2 == '\n') {
                ch3 = '\n';
            } else if (ch2 == '\t') {
                ch3 = '\t';
            } else if (ch2 == '\0') {
                ch3 = '\0';
            } else if (ch2 == 'u') {
                ch3 = 'u';
                if (ps + 4 > str_end) {
                    return false;
                }
                int uval = 0;
                for (size_t count = 4; count ;count --) {
                    int ch4 = hex_to_dec_table[(unsigned char)(*ps++)];
                    if (ch4 == -1) {
                        return false;
                    }
                    uval = (uval << 4) + ch4;
                }
                char buf[8];
                int len = ___ncr_decode((size_t)uval, buf);
                str.append(buf, len);
                continue;
            }
            if (ch3) {
                str.push_back(ch3);
            }
            continue;
        } else {
            str.push_back(ch);
        }
    }
    return false;
}

static inline bool ___fetch_string2(char *&ps, char *str_end, std::string &str)
{
    char ch;
    while (ps < str_end) {
        ch = *ps;
        if ((ch == '\r') || (ch=='\n') || (ch==' ') || (ch == '\t') || (ch == ':')) {
            break;
        }
        str.push_back(ch);
        ps++;
    }
    if (ps == str_end) {
        return false;
    }
    return true;
}

static bool json_unserialize(json *j, const char *jstr, size_t jsize)
{
    j->reset();
    std::string tmpkey(128, 0);
    char *ps = (char *)jstr, *str_end = ps + jsize;
    std::vector<json *> json_vec;
    json_vec.push_back(j);
    json *current_json, *new_json, *old_json;
    while(ps < str_end) {
        ___ignore_blank(ps, str_end);
        if (ps == str_end) {
            break;
        }
        if (json_vec.empty()) {
            break;
        }
        current_json = json_vec.back();
        json_vec.pop_back();
        if (current_json->is_object()) {
            int comma_count = 0;
            while(ps < str_end) {
                ___ignore_blank(ps, str_end);
                if (ps == str_end) {
                    return false;
                }
                if (*ps != ',') {
                    break;
                }
                comma_count ++;
                ps ++;
            }
            if (ps == str_end) {
                return false;
            }
            if (*ps == '}') {
                ps ++;
                continue;
            }
            if ((current_json->object_get_size()>0) && (comma_count == 0)) {
                return false;
            }
            tmpkey.clear();
            if ((*ps == '"') || (*ps == '\'')) {
                ___fetch_string(ps, str_end, tmpkey);
            } else {
                ___fetch_string2(ps, str_end, tmpkey);
            }
            new_json = new json();
            current_json->object_add_element(tmpkey.c_str(), new_json, &old_json);
            if (old_json) {
                delete old_json;
                return false;
            }
            json_vec.push_back(current_json);
            json_vec.push_back(new_json);
            ___ignore_blank(ps, str_end);
            if (ps == str_end) {
                return false;
            }
            if (*ps != ':') {
                return false;
            }
            ps ++;
            continue;
        }
        if (current_json->is_array()) {
            int comma_count = 0;
            while(ps < str_end) {
                ___ignore_blank(ps, str_end);
                if (ps == str_end) {
                    return false;
                }
                if (*ps != ',') {
                    break;
                }
                comma_count++;
                ps ++;
            }
            if (ps == str_end) {
                return false;
            }
            if (*ps == ']') {
                for (int i = 0; i < comma_count; i++) {
                    new_json = new json();
                    current_json->array_add_element(new_json);
                }
                ps ++;
                continue;
            }
            for (int i = 0; i < comma_count -1; i++) {
                new_json = new json();
                current_json->array_add_element(new_json);
            }
            if ((current_json->array_get_size() >0) && (comma_count < 1)){
                return false;
            }
            new_json = new json();
            current_json->array_add_element(new_json);
            json_vec.push_back(current_json);
            json_vec.push_back(new_json);
            continue;
        }
        if (*ps == '{') {
            json_vec.push_back(current_json);
            current_json->used_for_object();
            ps++;
            continue;
        }
        if (*ps == '[') {
            json_vec.push_back(current_json);
            current_json->used_for_array();
            ps++;
            continue;
        }
        if ((*ps == '"') || (*ps == '\'')) {
            tmpkey.clear();
            ___fetch_string(ps, str_end, tmpkey);
            if((!current_json->is_string()) && (!current_json->is_null())) {
                return false;
            }
            std::string &val = current_json->get_string_value();
            val = tmpkey;
            tmpkey.clear();
            continue;
        }
        if ((*ps == '-') || ((*ps >= '0') && (*ps <= '9'))) {
            tmpkey.clear();
            bool is_double = false;
            while(ps < str_end) {
                int ch = *ps;
                if ((ch!='-') && (ch!='+') && (ch!='.') && (ch!= 'E') &&(ch!='e') &&(!isdigit(ch))){
                    break;
                }
                if (ch == '.' || ch=='e' || ch == 'E'){
                    is_double = true;
                }
                tmpkey.push_back(ch);
                ps++;
            }
            if (is_double) {
                current_json->used_for_double();
                current_json->get_double_value() = atof(tmpkey.c_str());
            } else {
                current_json->used_for_long();
                current_json->get_long_value() = atol(tmpkey.c_str());
            }
            continue;
        }
        tmpkey.clear();
        while(ps < str_end) {
            int ch = *ps;
            if (!isalpha(ch)){
                break;
            }
            tmpkey.push_back(tolower(ch));
            ps++;
            if (tmpkey.size() > 10) {
                return false;
            }
        }

        if ((!strcmp(tmpkey.c_str(), "null")) || (!strcmp(tmpkey.c_str(), "undefined"))) {
            current_json->reset();
            continue;
        }
        if (!strcmp(tmpkey.c_str(), "true")) {
            current_json->used_for_bool()->get_bool_value() = true;
            continue;
        }
        if (!strcmp(tmpkey.c_str(), "false")) {
            current_json->used_for_bool()->get_bool_value() = false;
            continue;
        }
        return false;
    }

    return true;
}

bool json::unserialize(const char *jstr)
{
    bool r = json_unserialize(this, jstr, strlen(jstr));
    if (!r) {
        reset();
    }
    return r;
}

bool json::unserialize(const char *jstr, size_t jsize)
{
    bool r = json_unserialize(this, jstr, jsize);
    if (!r) {
        reset();
    }
    return r;
}

static void ___serialize_string(std::string &result, const char *data, size_t size)
{
    result.push_back('"');
    char *ps = (char *)data;
    for (size_t i = 0; i < size; i++) {
        unsigned char ch = ps[i];
        if (ch == '\\') {
            result.push_back('\\');
            result.push_back('\\');
        } else if (ch == '\"') {
            result.push_back('\\');
            result.push_back('"');
        } else if (ch == '\b') {
            result.push_back('\\');
            result.push_back('b');
        } else if (ch == '\f') {
            result.push_back('\\');
            result.push_back('f');
        } else if (ch == '\r') {
            result.push_back('\\');
            result.push_back('r');
        } else if (ch == '\n') {
            result.push_back('\\');
            result.push_back('n');
        } else if (ch == '\t') {
            result.push_back('\\');
            result.push_back('t');
        } else if (ch == '\0') {
            result.push_back('\\');
            result.push_back('\0');
        }else {
            result.push_back(ch);
        }
    }
    result.push_back('"');
}

static inline void ___serialize_string(std::string &result, const std::string &str)
{
    ___serialize_string(result, str.c_str(), str.size());
}

void json::serialize(std::string &result, int flag)
{
    json *current_json;
    size_t idx = 0;
    std::map<std::string, json *>::iterator map_it, map_it_next;
    std::vector<json *> json_vec;
    std::vector<size_t> array_vec;
    std::vector<std::map<std::string, json *>::iterator> object_vec;
    std::map<std::string, json*> virutal_object;
    json_vec.push_back(this);
    array_vec.push_back(0);
    object_vec.push_back(virutal_object.end());

    while(1) {
        if (json_vec.empty()) {
            break;
        }
        current_json = json_vec.back();
        json_vec.pop_back();
        idx = array_vec.back();
        array_vec.pop_back();
        map_it = object_vec.back();
        object_vec.pop_back();

        if (current_json->is_null()) {
            current_json = 0;
            if (!json_vec.empty()) {
                current_json = json_vec[json_vec.size()-1];
            }
            if ((current_json==0) || (!current_json->is_array())) {
                result.append("null");
            } else {
                result.append("null");
                /* like [1,,3] == [1,null,3] */
            }
            continue;
        }
        if (current_json->is_string()) {
            ___serialize_string(result, current_json->get_string_value());
            continue;
        }
        if (current_json->is_bool()) {
            result.append((current_json->get_bool_value())?"true":"false");
            continue;
        }
        if (current_json->is_long()) {
            sprintf_1024(result, "%ld", current_json->get_long_value());
            continue;
        }
        if (current_json->is_double()) {
            long l = current_json->get_double_value();
            if ((l > 1000 * 1000 * 1000 * 1000L) || (l < -1000 * 1000 * 1000 * 1000L)){
                sprintf_1024(result, "%e", (double)l);
            } else {
                sprintf_1024(result, "%ld", l);
            }
            continue;
        }
        if (current_json->is_array()) {
            size_t length = current_json->array_get_size();
            if (length == 0) {
                result.append("[]");
                continue;
            }
            if (idx == 0) {
                result.push_back('[');
                json_vec.push_back(current_json);
                idx++;
                array_vec.push_back(idx);
                object_vec.push_back(virutal_object.end());
                continue;
            }
            if (idx == length + 1) {
                result.push_back(']');
                continue;
            }
            if ((idx >1) && (idx < length + 1)) {
                result.push_back(',');
            }
            json_vec.push_back(current_json);
            array_vec.push_back(idx+1);
            object_vec.push_back(virutal_object.end());
            
            json_vec.push_back(current_json->array_get_element(idx-1));
            array_vec.push_back(0);
            object_vec.push_back(virutal_object.end());
            continue;
        }
        if (current_json->is_object()) {
            if (current_json->object_get_size() == 0) {
                result.append("{}");
                continue;
            }
            if ((map_it == virutal_object.end())||(map_it == current_json->___val.m->end())) {
                if (idx == 0) {
                    result.push_back('{');
                    map_it_next = current_json->___val.m->begin();
                    json_vec.push_back(current_json);
                    array_vec.push_back(1);
                    object_vec.push_back(map_it_next);
                    continue;
                } else {
                    result.push_back('}');
                    continue;
                }
            } else {
                if (map_it != current_json->___val.m->begin()) {
                    result.push_back(',');
                }
                json_vec.push_back(current_json);
                array_vec.push_back(1);
                map_it_next = map_it;
                map_it_next++;
                object_vec.push_back(map_it_next);
                ___serialize_string(result, map_it->first.c_str(), map_it->first.size());
                result.push_back(':');
                json_vec.push_back(map_it->second);
                array_vec.push_back(0);
                object_vec.push_back(virutal_object.end());
                continue;
            }
            continue;
        }
        continue;
    }
}

}
