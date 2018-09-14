/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-06-22
 * ================================
 */

#include "zcc.h"

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        const char *r = zcc::mime_type_from_suffix(argv[i], "");
        printf("%s: %s\n", argv[i], r);
    }
    return 0;
}

