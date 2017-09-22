/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-07-22
 * ================================
 */

#include "zcc.h"
#include <ctype.h>
#include <time.h>

namespace zcc
{

void http_cookie_parse_request(dict &result, const char *raw_cookie)
{
    char *q, *p, *ps = const_cast<char *>(raw_cookie);
    string name(32, 0), value(128, 0);
    while(1) {
        while(*ps) {
            if ((*ps == ' ') || (*ps == '\t')) {
                ps ++;
                continue;
            }
            break;
        }
        p = ps;
        while((*p != '\0') && (*p != ';')) {
            p++;
        }
        do {
            q = ps;
            while(q < p) {
                if (*q  == '=') {
                    break;
                }
                q ++ ;
            }
            if (q == p) {
                break;
            }
            name.clear();
            name.append(ps, q - ps);
            tolower(name.c_str());
            value.clear();
            q ++;
            url_hex_decode(q, p - q, value);
        } while(0);
        result.update(name.c_str(), value.c_str(), value.size());
        if (*p == '\0') {
            break;
        }
        ps = p + 1;
    }
}

void http_cookie_build(string &result, const char *name, const char *value, long expires, const char *path, const char *domain, bool secure, bool httponly)
{
    int ch;
    char *p;

    result.append(name);
    result.push_back('=');
    if (empty(value)) {
        result.append("deleted; expires=Thu, 01-Jan-1970 00:00:01 GMT; Max-Age=0;");
        return;
    }

    p = const_cast<char *>(value);
    while ((ch = *p++)) {
        if (ch == ' ') {
            result.push_back('+');
        } else if (isalnum(ch)) {
            result.push_back(ch);
        } else {
            result.push_back('%');
            result.push_back(ch<<4);
            result.push_back(ch&0X0F);
        }
    }

    if (expires > 0) {
        struct tm tmbuf;
        char timestringbuf[64 + 1];
        gmtime_r((time_t *)expires, &tmbuf);
        strftime(timestringbuf, 64, "%a, %d %b %Y %H:%M:%S GMT", &tmbuf);
        result.append("; expires=");
        result.append(timestringbuf);
        result.append("; Max-Age=");
        result.printf_1024("%ld", expires - time(0));
    }

    if (!empty(path)) {
        result.append("; path=");
        p = const_cast<char *>(path);
        while ((ch = *p++)) {
            if (ch == ' ') {
                result.push_back('+');
            } else if (isalnum(ch) || (ch == '/')) {
                result.push_back(ch);
            } else {
                result.push_back('%');
                result.push_back(ch<<4);
                result.push_back(ch&0X0F);
            }
        }
    }

    if (!empty(domain)) {
        result.append("; domain=");
        p = const_cast<char *>(domain);
        while ((ch = *p++)) {
            if (ch == ' ') {
                result.push_back('+');
            } else if (isalnum(ch)) {
                result.push_back(ch);
            } else {
                result.push_back('%');
                result.push_back(ch<<4);
                result.push_back(ch&0X0F);
            }
        }
    }

    if (secure) {
        result.append("; secure");
    }

    if (httponly) {
        result.append("; httponly");
    }
}

}
