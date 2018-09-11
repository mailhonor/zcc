/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2018-07-20
 * ================================
 */

#include "zcc.h"

int main(int argc, char **argv)
{
    zcc::main_parameter_run(argc, argv);
    zcc::fstream fp;
    if (!fp.open("./dd", "w+"))  {
        printf("ERR can not open dd(%m)\n");
        return 0;
    }
    for (int i=0;i<1000;i++) {
        fp.printf_1024("%5d XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n", i);
    }
    fp.close();

    if (!fp.open("./dd", "r"))  {
        printf("ERR can not open dd(%m) only read\n");
        return 0;
    }
    std::string linebuf, lastline;
    while(1) {
        linebuf.clear();
        if (fp.gets(linebuf) > 0) {
            lastline = linebuf;
        } else {
            break;
        }
    }
    printf("last line: %s\n", lastline.c_str());

    return 0;
}
