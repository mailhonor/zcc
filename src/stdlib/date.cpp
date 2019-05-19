/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-07-24
 * ================================
 */

#include "zcc.h"
#include <time.h>

namespace zcc
{

char *build_rfc1123_date_string(long t, std::string &result)
{
    struct tm tmbuf;
    char buf[64];
    gmtime_r((time_t *)(&t), &tmbuf);
    strftime(buf, 32, "%a, %d %b %Y %H:%M:%S GMT", &tmbuf);
    result.append(buf);
    return (char *)result.c_str();
}

}
