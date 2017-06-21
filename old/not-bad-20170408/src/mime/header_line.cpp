/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-12-09
 * ================================
 */

#include "zcc.h"

namespace zcc
{

void mime_unescape(const char *data, size_t size, string &dest)
{
    char ch, *src = (char *)(data);
    size_t i;

    dest.clear();
    for (i = 0; i < size; i++) {
        ch = src[i];
        if (ch == '\0') {
            continue;
        }
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            i++;
            if (i == size) {
                break;
            }
            ch = src[i];
            if (ch == '\t') {
                continue;
            }
            if (ch == ' ') {
                continue;
            }
            dest.push_back('\n');
        }
        dest.push_back(ch);
    }
}

size_t mime_unescape(const char *data, size_t size, char *dest)
{
    char ch, *src = (char *)(data);
    size_t i, rlen = 0;

    if (size > ZMAIL_HEADER_LINE_MAX_LENGTH){
        size = ZMAIL_HEADER_LINE_MAX_LENGTH;
    }
    for (i = 0; i < size; i++) {
        ch = src[i];
        if (ch == '\0') {
            continue;
        }
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            i++;
            if (i == size) {
                break;
            }
            ch = src[i];
            if (ch == '\t') {
                continue;
            }
            if (ch == ' ') {
                continue;
            }
            dest[rlen++] = '\n';
        }
        dest[rlen++] = ch;
    }
    dest[rlen] = 0;

    return rlen;
}

void mime_get_first_token(const char *line, size_t len, string &val)
{
    char *v;
    size_t l;
    mime_get_first_token(line, len, &v, &l);
    val.clear();
    val.append(v,l);
}

void mime_get_first_token(const char *line, size_t len, char **val, size_t *vlen)
{
    char *ps, *pend = (char *)line + len;
    size_t i;
    int ch;

    *val = (char *)line;
    *vlen = 0;
    for (i = 0; i < len; i++) {
        ch = line[i];
        if ((ch == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n') || (ch == '<')) {
            continue;
        }
        break;
    }
    if (i == len) {
        return;
    }
    ps = (char *)line + i;
    len = pend - ps;
    for (i = len - 1; i >= 0; i--) {
        ch = ps[i];
        if ((ch == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n') || (ch == '>')) {
            continue;
        }
        break;
    }
    if (i < 0) {
        return;
    }

    *val = ps;
    *vlen = i + 1;
    return;
}

size_t mime_get_elements(const char *in_src, size_t in_len, mime_element_t * mt_list, size_t mt_max_count)
{
    char *ps, *p1, *p2, *p3, *p, *pf, *pf_e, *pch, *pch_e, *pen, *pdata, *pdata_e;
    char *in_end = (char *)in_src + in_len;
    mime_element_t *mt;
    size_t mt_count = 0;
    int tmp_len;

    mt = mt_list;
    ps = (char *)in_src;
    while (in_end - ps > 0) {
        p = memstr(ps, "=?", in_end - ps);
        pf = ps;
        pf_e = p - 1;
        while (p) {
            pch = p + 2;
            p1 = memcasestr(pch, "?B?", in_end - pch);
            p2 = memcasestr(pch, "?Q?", in_end - pch);
            if (p1 && p2) {
                p3 = (p1 < p2 ? p1 : p2);
            } else if (p1 == 0) {
                p3 = p2;
            } else {
                p3 = p1;
            }
            if (!p3) {
                p = 0;
                break;
            }
            pch_e = p3 - 1;
            pen = p3 + 1;
            pdata = p3 + 3;
            p = memstr(pdata, "?=", in_end - pdata);
            if (!p) {
                break;
            }
            pdata_e = p - 1;
            ps = p + 2;
            mt->encode = 0;
            mt->data = pf;
            mt->size = pf_e - pf + 1;
            mt++;
            mt_count++;

            mt->encode = to_upper(*pen);
            tmp_len = pch_e - pch + 1;
            if (tmp_len > 31) {
                tmp_len = 31;
            }
            memcpy(mt->charset, pch, tmp_len);
            mt->charset[tmp_len] = 0;
            {
                /* rfc 2231 */
                char *p = strchr(mt->charset, '*');
                if (p) {
                    *p = 0;
                }
            }
            mt->data = pdata;
            mt->size = pdata_e - pdata + 1;
            mt++;
            mt_count++;
            p = (char *)in_src;
            break;
        }
        if (!p) {
            mt->encode = 0;
            mt->size = strlen(ps);
            mt->data = ps;
            mt++;
            mt_count++;
            break;
        }
        if (mt_count >= mt_max_count) {
            break;
        }
    }

    return mt_count;
}

void mime_get_utf8(const char *src_charset_def, const char *in_src, size_t in_len, string &dest)
{
    int ret, i, plen, mt_count;
    char *p;
    mime_element_t mt_list[ZMAIL_HEADER_LINE_MAX_ELEMENT + 1], *mt, *mtn;
    char bq_join_buf[ZMAIL_HEADER_LINE_MAX_LENGTH * 3 + 16];
    char zout_string_buf[ZMAIL_HEADER_LINE_MAX_LENGTH * 3 + 16];
    string bq_join, zout_string;
    bq_join.set_static_buf(bq_join_buf, ZMAIL_HEADER_LINE_MAX_LENGTH * 3 + 16);
    zout_string.set_static_buf(zout_string_buf, ZMAIL_HEADER_LINE_MAX_LENGTH * 3 + 16);

    dest.clear();
    if (in_len > ZMAIL_HEADER_LINE_MAX_LENGTH) {
        in_len = ZMAIL_HEADER_LINE_MAX_LENGTH;
    }

    mt_count = 0;
    memset(mt_list, 0, sizeof(mt_list));
    mt_count = mime_get_elements(in_src, in_len, mt_list, ZMAIL_HEADER_LINE_MAX_ELEMENT);

    for (i = 0; i < mt_count; i++) {
        mt = mt_list + i;
        if (mt->size == 0) {
            continue;
        }
        if ((mt->encode != 'B') && (mt->encode != 'Q')) {
            zout_string.clear();
            mime_iconv(src_charset_def, mt->data, mt->size, zout_string);
            dest += zout_string;
            continue;
        }
        bq_join.append(mt->data, mt->size);
        mtn = mt + 1;
        while (1) {
            if (i + 1 >= mt_count) {
                break;
            }
            if (mtn->encode == 0) {
                size_t j;
                char c;
                for (j = 0; j < mtn->size; j++) {
                    c = mtn->data[j];
                    if (c == ' ') {
                        continue;
                    }
                    break;
                }
                if (j == mtn->size) {
                    i++;
                    mtn++;
                    continue;
                }
                break;
            }
            if ((mt->encode == mtn->encode) && (*(mt->charset)) && (*(mtn->charset)) && (!strcasecmp(mt->charset, mtn->charset))) {
                bq_join.append(mtn->data, mtn->size);
                i++;
                mtn++;
                continue;
            }
            break;
        }
        p = bq_join.c_str();
        plen = bq_join.size();
        p[plen] = 0;
        ret = 0;
        zout_string.clear();
        if (mt->encode == 'B') {
            ret = base64_decode(p, plen, zout_string);
        } else if (mt->encode == 'Q') {
            ret = qp_decode_2047(p, plen, zout_string);
        }

        if (ret < 1) {
            continue;
        }
        bq_join = zout_string;
        zout_string.clear();
        mime_iconv(mt->charset, bq_join.c_str(), bq_join.size(), zout_string);
        dest += zout_string;
    }
}

}
