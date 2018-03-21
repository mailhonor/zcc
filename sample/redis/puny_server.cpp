/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2018-01-23
 * ================================
 */

#include "zcc.h"

int main(int argc, char **argv)
{
    zcc::redis_puny_server rps;
    rps.run(argc, argv);

    return 1;
}
