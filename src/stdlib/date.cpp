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

char *build_rfc1123_date_string(long t, char *buf)
{
    struct tm tmbuf;
    gmtime_r((time_t *)(&t), &tmbuf);
    strftime(buf, 32, "%a, %d %b %Y %H:%M:%S GMT", &tmbuf);
    return buf;
}

}
