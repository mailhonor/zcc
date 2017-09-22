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
    string content;
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

static inline bool ___fetch_string(char *&ps, char *str_end, string &str)
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

static inline bool ___fetch_string2(char *&ps, char *str_end, string &str)
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
    string tmpkey(128, 0);
    char *ps = (char *)jstr, *str_end = ps + jsize;
    vector<json *> json_vec;
    json_vec.push_back(j);
    json *current_json, *new_json, *old_json;
    while(ps < str_end) {
        ___ignore_blank(ps, str_end);
        if (ps == str_end) {
            break;
        }
        if (!json_vec.pop(&current_json)) {
            break;
        }
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
            if ((current_json->get_object_size()>0) && (comma_count == 0)) {
                return false;
            }
            tmpkey.clear();
            if ((*ps == '"') || (*ps == '\'')) {
                ___fetch_string(ps, str_end, tmpkey);
            } else {
                ___fetch_string2(ps, str_end, tmpkey);
            }
            new_json = new json();
            current_json->set_object_element(tmpkey.c_str(), new_json, &old_json);
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
                    current_json->push_back_array(new_json);
                }
                ps ++;
                continue;
            }
            for (int i = 0; i < comma_count -1; i++) {
                new_json = new json();
                current_json->push_back_array(new_json);
            }
            if ((current_json->get_array_size() >0) && (comma_count < 1)){
                return false;
            }
            new_json = new json();
            current_json->push_back_array(new_json);
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
            string *val = current_json->get_string_value();
            if (val == 0) {
                return false;
            }
            *val = tmpkey;
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
                (*current_json->get_double_value()) = atof(tmpkey.c_str());
            } else {
                current_json->used_for_long();
                (*current_json->get_long_value()) = atol(tmpkey.c_str());
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
            (*current_json->used_for_bool()->get_bool_value()) = true;
            continue;
        }
        if (!strcmp(tmpkey.c_str(), "false")) {
            (*current_json->used_for_bool()->get_bool_value()) = false;
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

static inline void ___serialize_string(string &result, const char *data, size_t size)
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

static inline void ___serialize_string(string &result, const string &str)
{
    ___serialize_string(result, str.c_str(), str.size());
}

void json::serialize(string &result, int flag)
{
    json *current_json;
    size_t idx = 0;
    json_object_walker *rbnode = 0, *rbnext;
    vector<json *> json_vec;
    vector<size_t> idx_vec;
    vector<json_object_walker *> rbnode_vec;
    json_vec.push_back(this);
    idx_vec.push_back(0);
    rbnode_vec.push_back(0);

    while(1) {
        if (!json_vec.pop(&current_json)) {
            break;
        }
        idx_vec.pop(&idx);
        rbnode_vec.pop(&rbnode);
        if (current_json->is_null()) {
            current_json = 0;
            if (json_vec.size()>0) {
                current_json = json_vec[json_vec.size()-1];
            }
            if ((current_json==0) || (!current_json->is_array())) {
                result.append("null");
            } else {
                /* like [1,,3] == [1,null,3] */
            }
            continue;
        }
        if (current_json->is_string()) {
            ___serialize_string(result, *(current_json->get_string_value()));
            continue;
        }
        if (current_json->is_bool()) {
            result.append((*(current_json->get_bool_value()))?"true":"false");
            continue;
        }
        if (current_json->is_long()) {
            result.printf_1024("%ld", *(current_json->get_long_value()));
            continue;
        }
        if (current_json->is_double()) {
            long l = *(current_json->get_double_value());
            if ((l > 1000 * 1000 * 1000 * 1000L) || (l < -1000 * 1000 * 1000 * 1000L)){
                result.printf_1024("%e", l);
            } else {
                result.printf_1024("%ld", l);
            }
            continue;
        }
        if (current_json->is_array()) {
            size_t length = current_json->get_array_size();
            if (length == 0) {
                result.append("[]");
                continue;
            }
            if (idx == 0) {
                result.push_back('[');
                json_vec.push_back(current_json);
                idx_vec.push_back(idx+1);
                rbnode_vec.push_back(0);
                continue;
            }
            if (idx == length + 1) {
                result.push_back(']');
                continue;
            }
            if ((idx >1) && (idx < length+1)) {
                result.push_back(',');
            }
            json_vec.push_back(current_json);
            idx_vec.push_back(idx+1);
            rbnode_vec.push_back(0);
            
            json_vec.push_back(current_json->get_array_element(idx-1));
            idx_vec.push_back(0);
            rbnode_vec.push_back(0);
            continue;
        }
        if (current_json->is_object()) {
            if (current_json->get_object_size() == 0) {
                result.append("{}");
                continue;
            }
            if (rbnode == 0) {
                if (idx == 0) {
                    result.push_back('{');
                    rbnext = current_json->get_object_first_walker();
                    json_vec.push_back(current_json);
                    idx_vec.push_back(1);
                    rbnode_vec.push_back(rbnext);
                    continue;
                } else {
                    result.push_back('}');
                    continue;
                }
            } else {
                if (rbnode != current_json->get_object_first_walker()) {
                    result.push_back(',');
                }
                json_vec.push_back(current_json);
                idx_vec.push_back(1);
                rbnode_vec.push_back(rbnode->next());
                ___serialize_string(result, rbnode->key(), strlen(rbnode->key()));
                result.push_back(':');
                json_vec.push_back(rbnode->value());
                idx_vec.push_back(0);
                rbnode_vec.push_back(0);
                continue;
            }
            continue;
        }
        continue;
    }
}

}
