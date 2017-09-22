/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-06
 * ================================
 */

#include "zcc.h"

namespace zcc
{

ssize_t hex_encode(const void *src, size_t src_size, string &str)
{
    unsigned char dec2hex[18] = "0123456789ABCDEF";
    unsigned char *src_c = (unsigned char *)src;
    size_t src_pos;
    int addch1, addch2;
    str.clear();

    for (src_pos = 0; src_pos < src_size; src_pos++) {
        addch1 = dec2hex[src_c[src_pos] >> 4];
        addch2 = dec2hex[src_c[src_pos] & 0X0F];
        str.push_back(addch1);
        str.push_back(addch2);
    }

    return str.size();
}

char hex_to_dec_table[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    //  0  1  2  3  4  5  6  7  8  9
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1, -1,
    //  A   B   C   D   E   F
    10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    //  a   b   c   d   e   f
    10, 11, 12, 13, 14, 15,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1
};

ssize_t hex_decode(const void *src, size_t src_size, string &str)
{
    unsigned char *src_c = (unsigned char *)src;
    size_t src_pos;
    unsigned char h_l, h_r;
    int addch;
    str.clear();

    for (src_pos = 0; src_pos + 1 < src_size; src_pos += 2) {
        h_l = hex_to_dec_table[src_c[src_pos]] << 4;
        h_r = hex_to_dec_table[src_c[src_pos + 1]];
        addch = (h_l | h_r);
        str.push_back(addch);
    }

    return str.size();
}

ssize_t url_hex_decode(const void *src, size_t src_size, string &str)
{
    str.clear();
    int l, r;
    char *p = (char *)src;
    for (size_t i = 0; i < src_size; i++) {
        if (p[i] == '+') {
            str.push_back('=');
        } else if (p[i] == '%') {
            if (i + 3 > src_size) {
                break;
            }
            l = hex_to_dec_table[(int)(p[i+1])];
            r = hex_to_dec_table[(int)(p[i+2])];
            if ((l!=-1) && (r!=-1)) {
                str.push_back((l<<4)+(r));
            }
            i += 2;
        } else {
            str.push_back(p[i]);
        }
    }
    return str.size();
}

}
