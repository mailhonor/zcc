/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-12-08
 * ================================
 */

#include "zcc.h"

namespace zcc
{

static const int address_name_max_length = 10240;
static int parser_one(char **str, int *len, char **rname, char **raddress, char *tmp_cache)
{
    char *pstr = *str, c;
    int plen = *len, i, inquote = 0;
    char *name = 0, *mail = 0, last = 0;
    int tmp_cache_idx = 0;
#define  ___put(ch)  { if(tmp_cache_idx>address_name_max_length) return -1;tmp_cache[tmp_cache_idx++] = (ch);}

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
}

mime_address_parser::~mime_address_parser()
{
}

void mime_address_parser::parse(const char *line, size_t size)
{
    ___str = const_cast<char *>(line);
    ___len = size;
    ___over = false;
}

bool mime_address_parser::shift(string &name, string &address)
{
    int ret;
    char *n, *a;
    char tmp_cache[address_name_max_length + 10];
    name.clear();
    address.clear();
    if (___over) {
        return false;
    }
    ___over = true;
    while (1) {
        ret = parser_one(&___str, &___len, &n, &a, tmp_cache);
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

void mime_get_address_vector(const char *in_str, size_t in_len, vector<mime_address_t *> &vec, mem_pool &mpool)
{
    mime_address_t *addr;
    char *n, *a;
    char cache[address_name_max_length + 10], *str = const_cast<char *>(in_str);
    int len = (int)in_len, ret;

    while (1) {
        ret = parser_one(&str, &len, &n, &a, cache);
        if (ret == -1) {
            break;
        }
        if (ret == -2) {
            continue;
        }
        addr = (mime_address_t *)mpool.malloc(sizeof(mime_address_t));
        addr->name = (char *)mpool.strdup(n);
        addr->address = (char *)mpool.strdup(a);
        addr->name_utf8 = blank_buffer;
        vec.push_back(addr);
    }
}

void mime_get_address_utf8_vector(const char * in_charset_def, char *in_str, size_t in_len
        , vector<mime_address_t *> &vec, mem_pool &mpool)
{
    ZCC_STACK_STRING(dest, address_name_max_length * 3 + 10);
    mime_address_t *addr;
    char *n, *a;
    char cache[address_name_max_length + 10], *str = const_cast<char *>(in_str);
    int len = (int)in_len, ret;

    while (1) {
        ret = parser_one(&str, &len, &n, &a, cache);
        if (ret == -1) {
            break;
        }
        if (ret == -2) {
            continue;
        }
        addr = (mime_address_t *)mpool.malloc(sizeof(mime_address_t));
        addr->name = (char *)mpool.strdup(n);
        addr->address = (char *)mpool.strdup(a);
        vec.push_back(addr);
        dest.clear();
        mime_get_utf8(in_charset_def, addr->name, strlen(addr->name), dest);
        addr->name_utf8 =  mpool.memdupnull(dest.c_str(), dest.size());
    }
}

void mime_free_address(mime_address_t *addr, mem_pool &mpool)
{
    mpool.free(addr->address);
    mpool.free(addr->name);
    mpool.free(addr->name_utf8);
    mpool.free(addr);
}


}
