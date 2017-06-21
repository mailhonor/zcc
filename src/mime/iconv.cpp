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

static inline void ___clear_null(const char *data, size_t size)
{
    char *p = const_cast<char *>(data);
    size_t i;
    for (i=0;i<size;i++) {
        if (p[i] == '\0') {
            p[i] = ' ';
        }
    }
}

void mime_iconv(const char *from_charset, const char *data, size_t size, std::string &dest)
{
    char f_charset_buf[64];
    const char *f_charset;
    bool detacted = false;

    dest.clear();
    if (size == 0) {
        return;
    }

    f_charset = from_charset;
    if (empty(f_charset)) {
        detacted = true;
        if (charset_detect_cjk(data, size, f_charset_buf)) {
            f_charset = f_charset_buf;
        } else  {
            f_charset = "GB18030";
        }
    } else {
        f_charset = charset_correct_charset(f_charset);
    }

    if (charset_iconv(f_charset, data, size
            , "UTF-8", dest
            , 0
            , -1, 0) > 0) {
        ___clear_null(dest.c_str(), dest.size());
        return;
    }

    if(detacted) {
        dest.append(data, size);
        ___clear_null(dest.c_str(), dest.size());
        return;
    } else {
        if (charset_detect_cjk(data, size, f_charset_buf)) {
            f_charset = f_charset_buf;
        } else  {
            f_charset = "GB18030";
        }
    }
    dest.clear();
    if (charset_iconv(f_charset, data, size
            , "UTF-8", dest
            , 0
            , -1, 0) > 0) {
        ___clear_null(dest.c_str(), dest.size());
        return;
    }

    dest.append(data, size);
    ___clear_null(dest.c_str(), dest.size());
    return;
}

size_t mime_iconv(const char *from_charset, const char *data, size_t size, char *dest, size_t dest_size)
{
    char f_charset_buf[64];
    const char *f_charset;
    bool detacted = false;
    size_t ret;

    if (size == 0) {
        return 0;
    }

    f_charset = from_charset;
    if (empty(f_charset)) {
        detacted = true;
        if (charset_detect_cjk(data, size, f_charset_buf)) {
            f_charset = f_charset_buf;
        } else  {
            f_charset = "GB18030";
        }
    } else {
        f_charset = charset_correct_charset(f_charset);
    }

    if ((ret = charset_iconv(f_charset, data, size
            , "UTF-8", dest, dest_size
            , 0
            , -1, 0)) > 0) {
        ___clear_null(dest, ret);
        return ret;
    }

    if(detacted) {
        ret = (size>dest_size?dest_size:size);
        memcpy(dest, data, ret);
        ___clear_null(dest, ret);
        return ret;
    } else {
        if (charset_detect_cjk(data, size, f_charset_buf)) {
            f_charset = f_charset_buf;
        } else  {
            f_charset = "GB18030";
        }
    }
    if ((ret = charset_iconv(f_charset, data, size
            , "UTF-8", dest, dest_size
            , 0
            , -1, 0)) > 0) {
        ___clear_null(dest, ret);
        return ret;
    }

    ret = (size>dest_size?dest_size:size);
    memcpy(dest, data, ret);
    ___clear_null(dest, ret);
    return ret;
}

}
