/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2018-01-23
 * ================================
 */

#include "zcc.h"

namespace zcc {
    extern pthread_mutex_t *var_general_pthread_mutex;
}
int main(int argc, char **argv)
{
    zcc::redis_puny_server rps;
    rps.run(argc, argv);

    if (zcc::var_general_pthread_mutex) ;
    return 1;
}
