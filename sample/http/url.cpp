/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-07-23
 * ================================
 */

#include "zcc.h"

int main(int argc, char **argv)
{
    char *url = 0;
    zcc_main_parameter_begin() {
        if (!strcmp(optname, "-url")) {
            url = optval;
            opti += 2;
            continue;
        }
    } zcc_main_parameter_end;

    if (!url) {
        printf("USAGE: %s -url http_url_string\n", argv[0]);
        exit(1);
    }
    zcc::http_url parser(url);
    parser.debug_show();

    return 0;
}
