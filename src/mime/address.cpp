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

mime_address::mime_address()
{
    ___name = blank_buffer;
    ___address = blank_buffer;
    ___name_utf8 = blank_buffer;
}

mime_address::mime_address(const mime_address &_x)
{
    ___name = strdup(_x.___name);
    ___address = strdup(_x.___address);
    ___name_utf8 = strdup(_x.___name_utf8);
}

mime_address::~mime_address()
{
    if (!___do_not_free) {
        free(___name);
        free(___address);
        free(___name_utf8);
    }
}

mime_address &mime_address::update_name(const char *name)
{
    free(___name);
    ___name = strdup(name);
    return *this;
}

mime_address &mime_address::update_address(const char *address)
{
    free(___address);
    ___address = strdup(___address);
    return *this;
}

mime_address &mime_address::update_name_utf8(const char *name_utf8)
{
    free(___name_utf8);
    ___name_utf8 = strdup(name_utf8);
    return *this;
}

mime_address & mime_address::operator=(const mime_address &_x)
{
    if (!___do_not_free) {
        free(___name);
        free(___address);
        free(___name_utf8);
        ___do_not_free = false;
    }
    ___name = strdup(_x.name());
    ___address = strdup(_x.address());
    ___name_utf8 = strdup(_x.name_utf8());
    return *this;
}

void mime_address::set_values(const char *name, const char *address, const char *name_utf8)
{
    if (name) {
        ___name = const_cast<char *>(name);
    }
    if (address) {
        ___address = const_cast<char *>(address);
    }
    if (name_utf8) {
        ___name_utf8 = const_cast<char *>(name_utf8);
    }
    ___do_not_free = true;
}

static int parser_one(char **str, int *len, char **rname, char **raddress, char *tmp_cache)
{
    char *pstr = *str;
    int c;
    int plen = *len, i, inquote = 0;
    char *name = 0, *mail = 0, last = 0;
    size_t tmp_cache_idx = 0;
#define  ___put(ch)  { if(tmp_cache_idx>var_mime_address_name_max_length) return -1;tmp_cache[tmp_cache_idx++] = (ch);}

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

    to_lower(mail);
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

    name.clear();
    address.clear();

    if (!___cache) {
        ___cache = (char *)malloc(var_mime_address_name_max_length + 10);
    }

    while (1) {
        ret = parser_one(&___str, &___len, &n, &a, ___cache);
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
        ret = parser_one(&___str, &___len, name, address, ___cache);
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
void mime_header_line_get_address(const char *in_str, size_t in_len, std::vector<mime_address *> &rvec)
{
    mime_address *addr;
    char *n, *a, *str, *cache;
    int len = (int)in_len, ret;
    mime_parser_cache_magic mcm(in_str);

    str = mcm.true_data;
    cache = mcm.cache->line_cache;

    while (1) {
        ret = parser_one(&str, &len, &n, &a, cache);
        if (ret == -1) {
            break;
        }
        if (ret == -2) {
            continue;
        }
        if (mcm.gmp) {
            addr = new(mcm.gmp->calloc(1, sizeof(mime_address)))mime_address();
            addr->set_values((char*)mcm.gmp->strdup(n), (char*)mcm.gmp->strdup(a), 0);
        } else {
            addr = new mime_address();
            addr->update_name(n);
            addr->update_address(a);
        }
        rvec.push_back(addr);
    }
}

void mime_header_line_get_address_utf8(const char *src_charset_def , const char *in_str, size_t in_len
        , std::vector<mime_address *> &rvec)
{
    const char *name;
    mime_address *addr;
    mime_parser_cache_magic mcm(in_str);
    std::string &dest = mcm.require_string();

    mime_header_line_get_address((char *)&mcm, in_len, rvec);
    for (std::vector<mime_address *>::iterator it = rvec.begin(); it != rvec.end(); it++) {
        addr = *it;
        name = addr->name();
        if (*name) {
            dest.clear();
            mcm.true_data = const_cast<char *>(name);
            mime_header_line_get_utf8(src_charset_def, (char *)&mcm, strlen(name), dest);
            if (mcm.gmp) {
                addr->set_values(0, 0, mcm.gmp->memdupnull(dest.c_str(), dest.size()));
            } else {
                addr->update_name_utf8(dest.c_str());
            }
        }
    }
}

}
