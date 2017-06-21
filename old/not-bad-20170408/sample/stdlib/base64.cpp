/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-12-05
 * ================================
 */

#include "zcc.h"

static void ___usage(char *arg)
{
    printf("USAGE: %s -encode/decode [ -mime ] -f filename\n", zcc::var_progname);
    exit(1);
}

int main(int argc, char **argv)
{
    char *fn = 0;
    bool encode_flag = true;
    bool mime_flag = false;

    zcc::var_progname = argv[0];
    ZCC_PARAMETER_BEGIN() {
        if (!strcmp(optname, "-encode")) {
            encode_flag = true;
            opti += 1;
            continue;
        }
        if (!strcmp(optname, "-decode")) {
            encode_flag = false;
            opti += 1;
            continue;
        }
        if (!strcmp(optname, "-mime")) {
            mime_flag = true;
            opti += 1;
            continue;
        }
        if (!optval) {
            ___usage(0);
        }
        if (!strcmp(optname, "-f")) {
            fn = optval;
            opti += 2;
            continue;
        }
    } ZCC_PARAMETER_END;

    if (zcc::empty(fn)) {
        ___usage(0);
    }
    zcc::string fcon, result;
    zcc::file_get_contents_sample(fn, fcon);
    if (encode_flag) {
        zcc::base64_encode(fcon.c_str(), fcon.length(), result, mime_flag);
    } else {
        zcc::base64_decode(fcon.c_str(), fcon.length(), result);
    }
    puts(result.c_str());

    return 0;
}
