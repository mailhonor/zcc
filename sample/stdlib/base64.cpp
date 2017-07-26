/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-12-05
 * ================================
 */

#include "zcc.h"

static void ___usage()
{
    printf("USAGE: %s -encode/decode [ -mime ] -f filename\n", zcc::var_progname);
    exit(1);
}

int main(int argc, char **argv)
{
    char *fn = 0;
    bool encode_flag = true;
    bool mime_flag = false;

    zcc_main_parameter_begin() {
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
            ___usage();
        }
        if (!strcmp(optname, "-f")) {
            fn = optval;
            opti += 2;
            continue;
        }
    } zcc_main_parameter_end;

    if (zcc::empty(fn)) {
        ___usage();
    }
    std::string fcon, result;
    zcc::file_get_contents_sample(fn, fcon);
    if (encode_flag) {
        zcc::base64_encode(fcon.c_str(), fcon.length(), result, mime_flag);
    } else {
#if 0
        zcc::base64_decode(fcon.c_str(), fcon.length(), result);
#else
        zcc::base64_decoder decoder;
        std::string tmp;
        for (size_t i = 0; i < fcon.length() + 1; i++) {
            /* i == fcon.length , *(fcon.c_str()) == 0, means over */
            if (decoder.decode(fcon.c_str() + i, 1, tmp) < 0) {
                return 0;
            }
            result.append(tmp);
        }
#endif
    }
    puts(result.c_str());

    return 0;
}
