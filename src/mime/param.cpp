/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2015-12-09
 * ================================
 */

#include "zcc.h"

namespace zcc
{

static char *ignore_chs(char *p, int plen, const char *chs_raw, int len, int flag)
{
    char *chs = const_cast<char *>(chs_raw);
    int i = 0, j;
    if (plen < 1)
        return (NULL);
    if (flag < 0)
        p += plen - 1;
    for (i = 0; i < plen; i++) {
        for (j = 0; j < len; j++) {
            if (*(p) == chs[j])
                break;
        }
        if (j == len)
            return (p);
        p += flag;
    }
    return (NULL);
}

static char *find_delim(char *p, int plen, const char *chs_raw, int len)
{
    char *chs = const_cast<char *>(chs_raw);
    int i, j;
    for (i = 0; i < plen; i++) {
        for (j = 0; j < len; j++) {
            if (p[i] == chs[j])
                return (p + i);
        }
    }
    return (NULL);
}

/* ################################################################## */
static int find_next_kv(char *buf, int len, char **key, int *key_len, char **value, int *value_len, char **nbuf, int *nlen)
{
    char *p = buf, *p1, *p2, *pe, *pn = 0;
    int find;

    p = ignore_chs(p, len, ";\t \r\n", 4, 1);
    if (!p) {
        return -1;
    }
    *nbuf = 0;
    *key = p;

    pe = (char *)memchr(p, '=', len - (p - buf));
    if (!pe) {
        return -1;
    }
    *pe = 0;

    p1 = find_delim(p, pe - p, "\t \r\n", 4);
    if (p1) {
        *p1 = 0;
    }
    *key_len = strlen(p);

    p = pe + 1;
    p = ignore_chs(p, len - (p - buf), "\t \r\n", 4, 1);
    if (!p) {
        return -1;
    }
    find = 0;
    if (*p == '"') {
        p++;
        find = 1;
    }
    *value = p;
    if (find) {
        p2 = find_delim(p, len - (p - buf), "\"\r\n", 3);
    } else {
        p2 = find_delim(p, len - (p - buf), "\t ;\r\n", 5);
    }
    if (p2) {
        *value_len = p2 - p;
        pn = p2 + 1;
    } else {
        *value_len = strlen(p);
    }

    if (pn) {
        *nbuf = pn;
        *nlen = len - (pn - buf);
    }

    return 0;
}

static int find_value(char *buf, int len, char **value, int *value_len, char **nbuf, int *nlen)
{
    char *p = buf, *p1;

    p = ignore_chs(p, len, "\t \"", 3, 1);
    if (!p) {
        return -1;
    }
    *nbuf = 0;
    *value = p;
    p1 = find_delim(p, len - (p - buf), ";\t \"\r\n", 6);
    if (p1) {
        *value_len = p1 - p;
        *nbuf = p1 + 1;
        *nlen = len - (p1 + 1 - buf);
    } else {
        *value_len = len - (p - buf);
    }

    return 0;
}

void mime_header_line_get_params(const char *data, size_t len, std::string &val, std::map<std::string, std::string> &params)
{
    char *value, *nbuf;
    int value_len, nlen;

    if (find_value((char *)data, (int)len, &value, &value_len, &nbuf, &nlen)) {
        return;
    }
    if (value_len) {
        val.append(value, value_len);
    }

    if (nbuf == 0) {
        return;
    }

    char *start, *key;
    int start_len, key_len;
    std::string kbuf(128, 0), vbuf(128, 0);

    start = nbuf;
    start_len = nlen;

    while (1) {
        if (start_len < 2) {
            break;
        }
        if (find_next_kv(start, start_len, &key, &key_len, &value, &value_len, &nbuf, &nlen) ) {
            break;
        }
        if (key_len < 1) {
            continue;
        }
        kbuf.clear();
        kbuf.append(key, key_len);
        vbuf.clear();
        vbuf.append(value, value_len);
        params[kbuf] = vbuf;

        if (nbuf == 0) {
            break;
        }
        start = nbuf;
        start_len = nlen;
    }
}

void mime_header_line_decode_content_type(const char *data, size_t len
        , std::string &_value, std::string &boundary, std::string &charset, std::string &name)
{
    char *value, *nbuf;
    int value_len, nlen;
    _value.clear();
    boundary.clear();
    charset.clear();
    name.clear();

    if (find_value((char *)data, (int)len, &value, &value_len, &nbuf, &nlen)) {
        return;
    }
    if (value_len) {
        _value.append(value, value_len);
    }

    if (nbuf == 0) {
        return;
    }

    char *start, *key;
    int start_len, key_len;

    start = nbuf;
    start_len = nlen;

    while (1) {
        if (start_len < 2) {
            break;
        }
        if (find_next_kv(start, start_len, &key, &key_len, &value, &value_len, &nbuf, &nlen) ) {
            break;
        }
        if (zcc_str_n_case_eq(key, "boundary", key_len)) {
            boundary.clear();
            boundary.append(value, value_len);
        } else if (zcc_str_n_case_eq(key, "charset", key_len)) {
            charset.clear();
            charset.append(value, value_len);
        } else if (zcc_str_n_case_eq(key, "name", key_len)) {
            name.clear();
            name.append(value, value_len);
        }
        if (nbuf == 0) {
            break;
        }
        start = nbuf;
        start_len = nlen;
    }
}

void mime_header_line_decode_content_disposition(const char *data, size_t len
        , std::string &_value, std::string &filename, std::string &filename_2231, bool *filename_2231_with_charset)
{
    char *value, *nbuf;
    int value_len, nlen;

    _value.clear();
    filename.clear();
    filename_2231.clear();

    if (find_value((char *)data, (int)len, &value, &value_len, &nbuf, &nlen)) {
        return;
    }
    if (value_len) {
        _value.append(value, value_len);
    }

    if (nbuf == 0) {
        return;
    }

    char *start, *key;
    int start_len, key_len;
    int flag_2231 = 0;
    bool charset_2231 = false;

    start = nbuf;
    start_len = nlen;

    while (1) {
        if (start_len < 2) {
            break;
        }
        if (find_next_kv(start, start_len, &key, &key_len, &value, &value_len, &nbuf, &nlen) ) {
            break;
        }
        if (zcc_str_n_case_eq(key, "filename", key_len)) {
            filename.clear();
            filename.append(value, value_len);
        } else if ((key_len > 8) && (zcc_str_n_case_eq(key, "filename*", 9))) {
            filename_2231.append(value, value_len);
            if (!flag_2231) {
                flag_2231 = 1;
                if (key[9] != '0') {
                    charset_2231 = true;
                } else if (key_len > 9) {
                    if (key[10] =='*') {
                        charset_2231 = true;
                    }
                }
            }
        }

        if (nbuf == 0) {
            break;
        }
        start = nbuf;
        start_len = nlen;
    }
    *filename_2231_with_charset = charset_2231;
}

void mime_header_line_decode_content_transfer_encoding(const char *data, size_t len, std::string &_value)
{
    char *value, *nbuf;
    int value_len, nlen;

    _value.clear();

    if (find_value((char *)data, (int)len, &value, &value_len, &nbuf, &nlen)) {
        return;
    }
    if (value_len) {
        _value.append(value, value_len);
    }
}

}
