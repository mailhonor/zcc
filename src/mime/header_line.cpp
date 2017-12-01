/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-12-09
 * ================================
 */

#include "zcc.h"
#include "mime.h"

namespace zcc
{

void mime_header_line_unescape(const char *data, size_t size, std::string &dest)
{
    int ch;
    char *src = (char *)(data);
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

size_t mime_header_line_unescape(const char *data, size_t size, char *dest, size_t dest_size)
{
    int ch;
    char *src = const_cast<char *>(data);
    size_t i, rlen = 0;

    if (size > dest_size){
        size = dest_size;
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

    return rlen;
}

void mime_header_line_get_first_token(const char *line, size_t len, std::string &val)
{
    char *v;
    size_t l;
    l = mime_header_line_get_first_token(line, len, &v);
    if (l) {
        val.append(v,l);
    }
}

size_t mime_header_line_get_first_token(const char *line_raw, size_t len, char **val)
{
    char *line = const_cast<char *>(line_raw), *ps, *pend = (char *)line + len;
    size_t i, vlen;
    int ch;

    *val = line;
    vlen = 0;
    for (i = 0; i < len; i++) {
        ch = line[i];
        if ((ch == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n') || (ch == '<')) {
            continue;
        }
        break;
    }
    if (i == len) {
        return vlen;
    }
    ps = line + i;
    len = pend - ps;
    for (i = len - 1; i >= 0; i--) {
        ch = ps[i];
        if ((ch == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n') || (ch == '>')) {
            continue;
        }
        break;
    }
    if (i < 0) {
        return vlen;
    }

    *val = ps;
    vlen = i + 1;
    return vlen;
}

size_t mime_header_line_get_elements(const char *in_src, size_t in_len, mime_header_line_element_t * mt_list, size_t mt_max_count)
{
    char *ps, *p1, *p2, *p3, *p, *pf, *pf_e, *pch, *pch_e, *pen, *pdata, *pdata_e;
    char *in_end = (char *)in_src + in_len;
    mime_header_line_element_t *mt;
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
            mt->charset[0] = 0;
            mt->encode = var_encode_none;
            mt->data = pf;
            mt->size = pf_e - pf + 1;
            mt++;
            mt_count++;
            {
                char c = toupper(*pen);
                if (c == 'B') {
                    mt->encode = var_encode_base64;
                } else if ( c == 'Q') {
                    mt->encode = var_encode_qp;
                } else  {
                    mt->encode = var_encode_none;
                }
            }
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
            mt->charset[0] = 0;
            mt->encode = var_encode_none;
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

void mime_header_line_get_utf8(const char *src_charset_def, const char *in_src, size_t in_len, std::string &dest)
{
    int ret, i, plen, mt_count;
    char *p;
    mime_header_line_element_t *mt_list, *mt, *mtn;
    mime_parser_cache_magic mcm(in_src);
   
    in_src = mcm.true_data;
    std::string &bq_join = mcm.require_string();
    std::string &out_string = mcm.require_string();

    if (in_len > var_mime_header_line_max_length) {
        in_len = var_mime_header_line_max_length;
    }

    mt_list = (mime_header_line_element_t *)(mcm.cache->line_cache);

    bq_join.clear();
    out_string.clear();

    mt_count = mime_header_line_get_elements(in_src, in_len, mt_list, var_mime_header_line_max_element);
    for (i = 0; i < mt_count; i++) {
        mt = mt_list + i;
        if (mt->size == 0) {
            continue;
        }
        if ((mt->encode != var_encode_base64) && (mt->encode != var_encode_qp)) {
            out_string.clear();
            mime_iconv(src_charset_def, mt->data, mt->size, out_string);
            dest += out_string;
            continue;
        }
        bq_join.append(mt->data, mt->size);
        mtn = mt + 1;
        while (1) {
            if (i + 1 >= mt_count) {
                break;
            }
            if (mtn->encode == var_encode_none) {
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
        p = const_cast<char *>(bq_join.c_str());
        plen = bq_join.size();
        p[plen] = 0;
        ret = 0;
        out_string.clear();
        if (mt->encode == var_encode_base64) {
            ret = base64_decode(p, plen, out_string);
        } else if (mt->encode == var_encode_qp) {
            ret = qp_decode_2047(p, plen, out_string);
        }

        if (ret < 1) {
            continue;
        }
        bq_join = out_string;
        out_string.clear();
        mime_iconv(mt->charset, bq_join.c_str(), bq_join.size(), out_string);
        dest += out_string;
    }
}

}
