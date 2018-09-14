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

void mime_header_line_get_utf8_2231(const char *src_charset_def, const char *in_str, size_t in_len, std::string &dest, bool with_charset)
{
    int i, len, start_len, ch;
    char *in_src, *pend, *ps, *p, *start, *charset;
    char charset_buf[32];
   
    in_src = (char *)in_str;
    pend = in_src + in_len;
    std::string tmps;

    tmps.clear();
    dest.clear();
    if (with_charset) {
        p = (char *)memchr(in_src, '\'', in_len);
        if (!p) {
            goto err;
        }
        len = p - in_src + 1;
        if (len > 31) {
            len = 31;
        }
        memcpy(charset_buf, in_src, len);
        charset_buf[len] = 0;

        ps = p + 1;
        p = (char *)memchr(ps, '\'', pend - ps);
        if (!p) {
            goto err;
        }
        p++;

        {
            char *p;
            p = strchr(charset_buf, '*');
            if (p) {
                *p = 0;
            }
        }
        charset = charset_buf;
        start = p;
        start_len = pend - p;
    } else {
        charset = (char *)src_charset_def;
        start = (char *)in_src;
        start_len = in_len;
    }

    for(i=0;i<start_len;i++) {
        ch = start[i];
        if (ch!='%') {
            tmps.push_back(ch);
            continue;
        }
        if (i+1 > start_len) {
            break;
        }
        ch = (hex_to_dec_table[(int)(start[i+1])]<<4) | (hex_to_dec_table[(int)(start[i+2])]);
        tmps.push_back(ch);
        i += 2;
    }
    mime_iconv(charset, tmps.c_str(), tmps.size(), dest);
    return;
err:
    dest.append(in_src, in_len);
}

}
