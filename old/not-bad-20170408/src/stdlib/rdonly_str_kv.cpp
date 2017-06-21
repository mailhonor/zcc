/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-12-07
 * ================================
 */

#include "zcc.h"

namespace zcc
{

rdonly_str_kv::rdonly_str_kv()
{
    _offset = (int *)(new string());
    _data = (char *)(new string());
    _count = 0;
    _over = false;
}

rdonly_str_kv::~rdonly_str_kv()
{
    if (_over) {
        if (_data) {
            free(_data);
        }
    } else {
        if (_data) {
            delete ((string *)_data);
            delete ((string *)_offset);
        }
    }
}

#define ___MY_CMP(s1, s2, result) { \
    char *c1 = (char *)(s1), *c2 = (char *)(s2); \
    if (*c1 == *c2) result=strcmp(c1+1, c2+1); \
    else if (*c1 < *c2) result=-1; \
    else result=1; \
} \
 
char *rdonly_str_kv::find(const char *key, const char *default_val)
{
    int count = _count;
    int *offset_list =  _offset;
    int left = 0, right = count-1, middle, token_len;
    int cmp_result;

    if (!_over) {
        return const_cast <char *> (default_val);
    }

    while(left <= right) {
        middle = (left + right)/2;
        ___MY_CMP(_data + offset_list[middle] + 2, key, cmp_result);
        if (cmp_result == 0) {
            token_len = ((_data[offset_list[middle]]) << 8) + _data[offset_list[middle]+1];
            return _data + offset_list[middle] + 2 + token_len +1;
        }
        if (cmp_result < 0) {
            left = middle + 1;
            continue;
        }
        right = middle - 1;
    }

    return const_cast <char *> (default_val);
}

rdonly_str_kv &rdonly_str_kv::add(const char *key, const char *value)
{
    string *str_data = (string *)_data;
    string *str_offset = (string *)_offset;
    int n_offset, key_len, key_len_add;

    key_len = strlen(key);
    if (key_len == 0) {
        return *this;
    }
    if (key_len > 65535) {
        log_fatal("rdonly_str_kv: the size of the key is too long");
    }

    n_offset = str_data->size();
    str_offset->append((char *)&n_offset, sizeof(int));
    _count++;

    key_len_add = ((key_len >> 8) & 0X00FF);
    str_data->push_back(key_len_add);
    key_len_add = (key_len & 0X00FF);
    str_data->push_back(key_len_add);

    str_data->append(key, key_len + 1);

    str_data->append(value);
    str_data->push_back(0);

    return *this;
}

rdonly_str_kv &rdonly_str_kv::over()
{
    string *str_data = (string *)_data;
    string *str_offset = (string *)_offset;
    int sum_len;

    if (_over) {
        return *this;
    }
    _over = true;

    sum_len = str_data->size() + str_offset->size();
    _data = (char *)malloc(sum_len);
    memcpy(_data, str_data->c_str(), str_data->size());
    memcpy(_data + str_data->size(), str_offset->c_str(), str_offset->size());
    _offset = (int *)(_data + str_data->size());

    delete str_data;
    delete str_offset;

    return *this;
}

}
