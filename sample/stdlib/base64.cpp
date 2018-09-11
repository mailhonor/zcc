/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2016-12-05
 * ================================
 */

#include "zcc.h"

static void ___usage()
{
    printf("USAGE: %s --encode/decode [ --mime ] -f filename\n", zcc::var_progname);
    exit(1);
}

int main(int argc, char **argv)
{
    char *fn = 0;
    bool encode_flag = true;
    bool mime_flag = false;
    zcc::main_parameter_run(argc, argv);
    encode_flag = zcc::default_config.get_bool("encode", false);
    mime_flag = zcc::default_config.get_bool("mime", false);
    fn = zcc::default_config.get_str("f");

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
