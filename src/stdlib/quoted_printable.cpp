/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2015-12-02
 * ================================
 */

#include "zcc.h"

namespace zcc
{

/* should check c1 and c2 are hex */
#define ___hex_val(ccc) { ccc = hex_to_dec_table[(unsigned char)ccc];}

#define ___get_next_ch(c0123)    while(1){ \
    if(src_pos >= src_size){ goto over; } \
    c0123 = src_c[src_pos++]; \
    break; \
}

ssize_t qp_decode_2045(const void *src, size_t src_size, std::string &str)
{
    unsigned char *src_c = (unsigned char *)src;
    size_t src_pos = 0;
    char c0, c1, c2;
    char addch;

    while (1) {
        ___get_next_ch(c0);
        if (c0 != '=') {
            addch = c0;
            goto append;
        }
        ___get_next_ch(c1);
        if (c1 == '\r' || c1 == '\n') {
            ___get_next_ch(c2);
            if (c2 != '\r' && c2 != '\n') {
                src_pos--;
            }
            continue;
        }
        ___get_next_ch(c2);
        ___hex_val(c1);
        ___hex_val(c2);
        addch = ((c1 << 4) | c2);
append:
        str.push_back(addch);
    }
over:

    return str.size();
}

ssize_t qp_decode_2047(const void *src, size_t src_size, std::string &str)
{
    unsigned char *src_c = (unsigned char *)src;
    size_t src_pos = 0;
    char c0, c1, c2;
    char addch;

    while (1) {
        ___get_next_ch(c0);
        if (c0 == '_') {
            addch = ' ';
        } else if (c0 != '=') {
            addch = c0;
        } else {
            ___get_next_ch(c1);
            ___get_next_ch(c2);
            ___hex_val(c1);
            ___hex_val(c2);
            addch = (c1 << 4 | c2);
        }
        str.push_back(addch);
    }

over:

    return str.size();
}

ssize_t qp_decode_get_valid_len(const void *src, size_t src_size)
{
    unsigned char *src_c = (unsigned char *)src;
    size_t i;
    unsigned char ch;

    for (i = 0; i < src_size; i++) {
        ch = src_c[i];
        if ((ch < 33) || (ch > 126)) {
            return i;
        }
    }

    return src_size;
}

}
