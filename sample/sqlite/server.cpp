/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
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
