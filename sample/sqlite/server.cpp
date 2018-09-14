/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-08-02
 * ================================
 */

#include "zcc.h"

int main(int argc, char **argv)
{
    zcc::sqlite3_proxyd sqlite3db;
    sqlite3db.run(argc, argv);

    return 1;
}
