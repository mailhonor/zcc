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
    char *cookie = 0;
    zcc::main_parameter_run(argc, argv);
    cookie = zcc::default_config.get_str("cookie");

    if (zcc::empty(cookie)) {
        printf("USAGE: %s -cookie http_cookie_string\n", argv[0]);
        exit(1);
    }
    zcc::dict dt;
    zcc::http_cookie_parse_request(dt, cookie);
    dt.debug_show();

    return 0;
}
