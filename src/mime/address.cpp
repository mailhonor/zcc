/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-12-08
 * ================================
 */

#include "zcc.h"
#include "mime.h"

namespace zcc
{

static int parser_one(char **str, int *len, char **rname, char **raddress, char *tmp_cache, int tmp_cache_size)
{
    char *pstr = *str;
    int c;
    int plen = *len, i, inquote = 0;
    char *name = 0, *mail = 0, last = 0;
    int tmp_cache_idx = 0;
#define  ___put(ch)  { if(tmp_cache_idx>tmp_cache_size) return -1;tmp_cache[tmp_cache_idx++] = (ch);}

    if (plen <= 0) {
        return -1;
    }
    for (i = 0; i < plen; i++) {
        c = *(pstr++);
        if (last == '\\') {
            ___put(c);
            last = '\0';
            continue;
        }
        if (c == '\\') {
            last = c;
            continue;
        }
        if (c == '"') {
            if (inquote) {
                inquote = 0;
                ___put(c);
            } else {
                inquote = 1;
            }
            continue;
        }
        if (inquote) {
            ___put(c);
            continue;
        }
        if (c == ',') {
            break;
        }
        ___put(c);
    }
    *len = *len - (pstr - *str);
    *str = pstr;

    tmp_cache[tmp_cache_idx] = 0;
    pstr = tmp_cache;
    plen = tmp_cache_idx;
    if (plen < 1) {
        return -2;
    }
    while (1) {
        pstr = trim(pstr);
        plen = strlen(pstr);
        if (plen < 1) {
            return -2;
        }
        if (pstr[plen - 1] == '>') {
            pstr[plen - 1] = ' ';
            continue;
        }
        break;
    }
    unsigned char ch;
    int findi = -1;
    for (i = plen - 1; i >= 0; i--) {
        ch = pstr[i];
        if ((ch == '<') || (ch == ' ') || (ch == '"') || (ch & 0X80)) {
            pstr[i] = 0;
            findi = i;
            break;
        }
    }
    if (findi > -1) {
        name = pstr;
        mail = trim(pstr + findi + 1);
    } else {
        name = 0;
        mail = pstr;
    }

    tolower(mail);
    *raddress = mail;

    char *name_bak = name;
    pstr = name;
    while (name && *name) {
        if (*name != '"') {
            *pstr++ = *name++;
        } else {
            *pstr++ = ' ';
            name++;
        }
    }
    if (pstr) {
        *pstr = 0;
    }
    if (name_bak) {
        int slen = skip(name_bak, strlen(name_bak), " \t\"'\r\n", 0, rname); 
        if (slen > 0) {
         (*rname)[slen] = 0;
        } else {
            *rname = trim(name_bak);
        }
    } else {
        *rname = blank_buffer;
    }

#undef ___put
    return 0;
}

mime_address_parser::mime_address_parser()
{
    ___str = 0;
    ___len = 0;
    ___over = false;
    ___cache = 0;
}

mime_address_parser::~mime_address_parser()
{
    free(___cache);
}

void mime_address_parser::parse(const char *line, size_t size)
{
    ___str = const_cast<char *>(line);
    ___len = size;
    ___over = false;
}

bool mime_address_parser::shift(std::string &name, std::string &address)
{
    int ret;
    char *n, *a;

    if (___over) {
        return false;
    }
    ___over = true;

    if (!___cache) {
        ___cache = (char *)malloc(var_mime_address_name_max_length + 10);
    }

    while (1) {
        ret = parser_one(&___str, &___len, &n, &a, ___cache, var_mime_address_name_max_length);
        if (ret == -1) {
            return false;
        }
        if (ret == -2) {
            continue;
        }
        name.append(n);
        address.append(a);
        ___over = false;
        return true;
    }
    return false;
}

bool mime_address_parser::shift(char **name, char **address)
{
    int ret;

    if (___over) {
        return false;
    }
    ___over = true;

    if (!___cache) {
        ___cache = (char *)malloc(var_mime_address_name_max_length + 10);
    }

    while (1) {
        ret = parser_one(&___str, &___len, name, address, ___cache, var_mime_address_name_max_length);
        if (ret == -1) {
            return false;
        }
        if (ret == -2) {
            continue;
        }
        ___over = false;
        return true;
    }
    return false;
}

std::list<mime_address> mime_header_line_get_address(const char *in_str, size_t in_len)
{
    std::list<mime_address> r;
    mime_header_line_get_address(in_str, in_len, r);
    return r;
}

void mime_header_line_get_address(const char *in_str, size_t in_len, std::list<mime_address> &rvec)
{
    char *n, *a, *str, *cache;
    int len = (int)in_len, ret;

    str = (char *)in_str;
    cache = (char *)malloc(in_len + 1024);

    while (1) {
        ret = parser_one(&str, &len, &n, &a, cache, in_len + 1000);
        if (ret == -1) {
            break;
        }
        if (ret == -2) {
            continue;
        }
        mime_address addr;
        addr.name = n;
        addr.address = a;
        rvec.push_back(addr);
    }

    free(cache);
}

std::list<mime_address> mime_header_line_get_address_utf8(const char *src_charset_def,
        const char *in_str, size_t in_len)
{
    std::list<mime_address> r;
    mime_header_line_get_address_utf8(src_charset_def, in_str, in_len, r);
    return r;
}

void mime_header_line_get_address_utf8(const char *src_charset_def, const char *in_str, size_t in_len
        , std::list<mime_address> &rvec)
{
    char *n, *a, *str, *cache;
    int len = (int)in_len, ret;

    str = (char *)in_str;
    cache = (char *)malloc(in_len + 1024);

    while (1) {
        ret = parser_one(&str, &len, &n, &a, cache, in_len + 1000);
        if (ret == -1) {
            break;
        }
        if (ret == -2) {
            continue;
        }
        mime_address addr;
        addr.name = n;
        addr.address = a;
        if (!addr.name.empty()) {
            mime_header_line_get_utf8(src_charset_def, addr.name.c_str(), addr.name.size(), addr.name_utf8);
        }
        rvec.push_back(addr);
    }

    free(cache);
}

}
