/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2017-07-23
 * ================================
 */

#include "zcc.h"

int main(int argc, char **argv)
{
    char *url = 0;
    zcc::main_parameter_run(argc, argv);
    url = zcc::default_config.get_str("url");
    if (zcc::empty(url)) {
        printf("USAGE: %s -url http_url_string\n", argv[0]);
        exit(1);
    }
    zcc::http_url parser(url);
    parser.debug_show();

    return 0;
}
