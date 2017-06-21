/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-01-12
 * ================================
 */

#include "zcc.h"

namespace zcc
{

static inline void ___clear_null(const char *data, size_t size, string &dest)
{
    char ch, *p = (char *)data;
    size_t i;

    dest.clear();
    for (i=0;i<size;i++) {
        ch = p[i];
        if (ch == '\0') {
            continue;
        }
        dest.push_back(ch);
    }
}

void mime_iconv(const char *from_charset, const char *data, size_t size, string &dest)
{
    char f_charset_buf[64];
    const char *f_charset;
    string tmpstr;
    size_t ret;
    bool detact = false;
    dest.clear();

    f_charset = from_charset;
    if (empty(f_charset)) {
        detact = true;
        if (charset_detect_cjk(data, size, f_charset_buf)) {
            f_charset = f_charset_buf;
        } else  {
            f_charset = "GB18030";
        }
    }
    f_charset = charset_correct_charset(f_charset);
    tmpstr.clear();
    ret = charset_iconv(f_charset, data, size
            , "UTF-8", tmpstr
            , 0
            , -1, 0);

    if (ret > 0 ) {
        ___clear_null(tmpstr.c_str(), tmpstr.size(), dest);
        return;
    }

    if(detact) {
        ___clear_null(data, size, dest);
        return;
    } else {
        if (charset_detect_cjk(data, size, f_charset_buf)) {
            f_charset = f_charset_buf;
        } else  {
            f_charset = "GB18030";
        }
    }
    tmpstr.clear();
    ret = charset_iconv(f_charset, data, size
            , "UTF-8", tmpstr
            , 0
            , -1, 0);
    if (ret > 0 ) {
        ___clear_null(tmpstr.c_str(), tmpstr.size(), dest);
        return;
    }

    ___clear_null(data, size, dest);
}

}
